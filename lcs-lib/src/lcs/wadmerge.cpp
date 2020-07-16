#include "wadmerge.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "conflict.hpp"
#include <numeric>
#include <utility>
#include <unordered_set>
#include <picosha2.hpp>

using namespace LCS;

template<size_t S>
static void copyStream(std::istream& source, std::ofstream& dest,
                       size_t size, char (&buffer)[S],
                       Progress& progress) {
    for(size_t i = 0; i != size;) {
        size_t count = std::min(S, size - i);
        source.read(buffer, count);
        dest.write(buffer, count);
        // progress.consumeData(count);
        i += count;
    }
    progress.consumeData(size);
}

WadMerge::WadMerge(fs::path const& path, Wad const* original)
 : path_(fs::absolute(path)), original_(original) {
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    lcs_assert(original_);
    lcs_trace("original: ", original->name());
    fs::create_directories(path_.parent_path());
    // TODO: potentially optimize by using the fact entries are sorted
    for(auto const& entry: original->entries()) {
        entries_.insert_or_assign(entry.xxhash, Entry { entry, original, EntryKind::Original });
        orgsha256_.insert_or_assign(entry.xxhash, entry.sha256);
    }
}

void WadMerge::addWad(const Wad* source, Conflict conflict) {
    lcs_trace_func();
    lcs_assert(source);
    lcs_trace("source: ", source->path());
    for(auto const& entry: source->entries()) {
        addWadEntry(entry, source, conflict, EntryKind::Full);
    }
}

void WadMerge::addExtraEntry(const Wad::Entry& entry, const Wad* source, Conflict conflict) {
    lcs_trace_func();
    lcs_assert(source);
    lcs_trace("source: ", source->path());
    addWadEntry(entry, source, conflict, EntryKind::Extra);
}

void WadMerge::addWadEntry(Wad::Entry const& entry, Wad const* source,
                           Conflict conflict, EntryKind kind) {
    lcs_trace_func();
    lcs_assert(source);
    lcs_trace("source: ", source->path());
    if (auto o = orgsha256_.find(entry.xxhash); o != orgsha256_.end() && o->second == entry.sha256) {
        return;
    }
    if (auto i = entries_.find(entry.xxhash); i != entries_.end()) {
        if (i->second.sha256 != entry.sha256 && i->second.kind_ != EntryKind::Original) {
            if (conflict == Conflict::Skip) {
                return;
            } else if(conflict == Conflict::Abort) {
                throw ConflictError(entry.xxhash, i->second.wad_->path(), source->path());
            }
        }
        i->second = Entry { entry, source, kind };
        sizeCalculated_ = false;
        sizeFastCalculated_ = false;
    } else {
        entries_.insert(i, std::pair {entry.xxhash, Entry { entry, source, kind } });
        sizeCalculated_ = false;
        sizeFastCalculated_ = false;
    }
}

size_t WadMerge::size() const noexcept {
    if(!sizeCalculated_) {
        size_ = std::accumulate(entries_.begin(), entries_.end(), size_t{0},
                    [](size_t old, auto const& kvp) -> size_t {
            return old + kvp.second.sizeCompressed;
        });
        sizeCalculated_ = true;
    }
    return size_;
}

void WadMerge::write(Progress& progress) const {
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    size_t totalSize = size();
    progress.startItem(path_, totalSize);
    Wad::Header newHeader = original_->header();
    newHeader.filecount = (uint32_t)(entries_.size());
    picosha2::hash256_one_by_one sha256_hasher;
    for(auto const& [xxhash, entry]: entries_) {
        auto const beg = (char const*)(&entry);
        auto const end = beg + sizeof(Wad::Entry);
        sha256_hasher.process(beg, end);
    }
    sha256_hasher.finish();
    sha256_hasher.get_hash_bytes(newHeader.signature.begin(), newHeader.signature.end());
    if (std::ifstream infile(path_, std::ios::binary); infile) {
        Wad::Header oldHeader = {};
        infile.read((char*)&oldHeader, sizeof(Wad::Header));
        if (memcmp((char const*)&oldHeader, (char const*)&newHeader, sizeof(Wad::Header)) == 0) {
            progress.consumeData(totalSize);
            progress.finishItem();
            return;
        }
    }
    fs::create_directories(path_.parent_path());
    std::ofstream outfile;
    outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    outfile.open(path_, std::ios::binary);
    std::vector<Wad::Entry> newEntries;
    newEntries.resize(entries_.size());
    uint32_t dataOffset = (uint32_t)(sizeof(Wad::Header) + entries_.size() * sizeof(Wad::Entry));
    std::transform(entries_.begin(), entries_.end(), newEntries.begin(),
                   [&dataOffset](std::pair<uint64_t,Wad::Entry> kvp) {
        kvp.second.dataOffset = dataOffset;
        dataOffset += kvp.second.sizeCompressed;
        return kvp.second;
    });
    outfile.write((char const*)&newHeader, (std::streamsize)sizeof(Wad::Header));
    outfile.write((char const*)newEntries.data(), (std::streamsize)(newEntries.size() * sizeof(Wad::Entry)));
    char buffer[64 * 1024];
    for(auto const& [xxhash, entry]: entries_) {
        auto& source = entry.wad_->file();
        auto sourceOffset = entry.dataOffset;
        auto& dest = outfile;
        auto size = entry.sizeCompressed;
        source.seekg((std::streamoff)sourceOffset, std::ifstream::beg);
        copyStream(source, dest, size, buffer, progress);
    }
    progress.finishItem();
}

size_t WadMerge::size_whole() const noexcept {
    if(!sizeFastCalculated_) {
        size_t sum = 0;
        std::unordered_set<Wad const*> fullWads;
        for(auto const& [xxhash, entry]: entries_) {
            if (entry.kind_ == EntryKind::Extra) {
                sum += entry.sizeCompressed;
            } else if (auto full = fullWads.find(entry.wad_); full == fullWads.end()) {
                sum += (uint32_t)(entry.wad_->dataSize());
                fullWads.insert(full, entry.wad_);
            }
        }
        sizeFast_ = sum;
        sizeFastCalculated_ = true;
    }
    return sizeFast_;
}

void WadMerge::write_whole(Progress& progress) const {
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    size_t totalSize = size_whole();
    progress.startItem(path_, totalSize);
    Wad::Header newHeader = original_->header();
    newHeader.filecount = (uint32_t)(entries_.size());
    picosha2::hash256_one_by_one sha256_hasher;
    for(auto const& [xxhash, entry]: entries_) {
        auto const beg = (char const*)(&entry);
        auto const end = beg + sizeof(Wad::Entry);
        sha256_hasher.process(beg, end);
    }
    sha256_hasher.finish();
    sha256_hasher.get_hash_bytes(newHeader.signature.begin(), newHeader.signature.end());
    if (std::ifstream infile(path_, std::ios::binary); infile) {
        Wad::Header oldHeader = {};
        infile.read((char*)&oldHeader, sizeof(Wad::Header));
        if (memcmp((char const*)&oldHeader, (char const*)&newHeader, sizeof(Wad::Header)) == 0) {
            progress.consumeData(totalSize);
            progress.finishItem();
            return;
        }
    }
    fs::create_directories(path_.parent_path());
    std::ofstream outfile;
    outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    outfile.open(path_, std::ios::binary);
    uint32_t dataOffset = (uint32_t)(sizeof(Wad::Header) + entries_.size() * sizeof(Wad::Entry));
    outfile.seekp((std::streamoff)dataOffset);
    std::unordered_map<Wad const*, uint32_t> fullWads;
    std::vector<Wad::Entry> newEntries;
    char buffer[64 * 1024];
    for(auto const& [xxhash, entry]: entries_) {
        auto wad = entry.wad_;
        if (entry.kind_ == EntryKind::Extra) {
            auto& source = wad->file();
            auto sourceOffset = entry.dataOffset;
            auto& dest = outfile;
            auto size = entry.sizeCompressed;
            source.seekg((std::streamoff)sourceOffset, std::ifstream::beg);
            copyStream(source, dest, size, buffer, progress);
            auto& copy = newEntries.emplace_back(entry);
            copy.dataOffset = dataOffset;
            dataOffset += size;
        } else {
            uint32_t offAdd;
            if (auto full = fullWads.find(wad); full != fullWads.end()) {
                offAdd = full->second;
            } else {
                fullWads.insert(full, std::pair{ wad, dataOffset });
                auto& source = wad->file();
                auto sourceOffset = wad->dataBegin();
                auto& dest = outfile;
                auto size = wad->dataSize();
                source.seekg((std::streamoff)sourceOffset, std::ifstream::beg);
                copyStream(source, dest, size, buffer, progress);
                offAdd = dataOffset;
                dataOffset += (uint32_t)size;
            }
            auto& copy = newEntries.emplace_back(entry);
            copy.dataOffset -= (uint32_t)(wad->dataBegin());
            copy.dataOffset += offAdd;
        }
    }
    outfile.seekp(0);
    outfile.write((char const*)&newHeader, sizeof(Wad::Header));
    outfile.write((char const*)newEntries.data(), (std::streamsize)(newEntries.size() * sizeof(Wad::Entry)));
    progress.finishItem();
}
