#include "wadmake.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "utility.hpp"
#include <charconv>
#include <numeric>
#include <xxhash.h>
#include <picosha2.hpp>
#include <zstd.h>
#include <miniz.h>

using namespace LCS;

WadMakeBase::~WadMakeBase() noexcept {

}

static uint64_t pathhash(fs::path const& path) noexcept {
    uint64_t h;
    fs::path noextension = path;
    noextension.replace_extension();
    auto name = noextension.generic_u8string();
    auto const start = reinterpret_cast<char const*>(name.data());
    auto const end = start + name.size();
    auto const result = std::from_chars(start, end, h, 16);
    if (result.ptr == end && result.ec == std::errc{}) {
        return h;
    }
    name = path.generic_u8string();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return XXH64(name.data(), name.size(), 0);
}

/// Copies a .wad from filesystem
WadMakeCopy::WadMakeCopy(fs::path const& path, WadIndex const* index,
                         bool removeUnknownNames, bool removeUnchangedEntries)
    : path_(fs::absolute(path)),
      name_(path_.filename()),
      index_(index),
      remove_unknown_names_(removeUnknownNames),
      remove_unchanged_entries_(removeUnchangedEntries)
{
    lcs_trace_func(
                lcs_trace_var(path),
                lcs_trace_var(removeUnknownNames),
                lcs_trace_var(removeUnchangedEntries)
                );
    if (removeUnknownNames || removeUnchangedEntries) {
        lcs_assert(index);
    }
    auto wad = Wad(path_);
    entries_ = wad.entries();
    is_oldchecksum_ = wad.is_oldchecksum();
    if (removeUnknownNames) {
        std::erase_if(entries_, [&checksums = index->checksums()] (auto const& entry) -> bool {
            return !checksums.contains(entry.xxhash);
        });
    }
    for (auto const& entry: entries_) {
        size_ += entry.sizeCompressed;
    }
}

void WadMakeCopy::write(fs::path const& dstpath, Progress& progress) const {
    lcs_trace_func(
                lcs_trace_var(dstpath)
                );
    progress.startItem(dstpath, size_);
    InFile infile(path_);
    fs::create_directories(dstpath.parent_path());
    OutFile outfile(dstpath);
    std::vector<char> buffer;
    std::vector<Wad::Entry> entries = {};
    entries.reserve(entries_.size());
    std::uint32_t dataOffset = sizeof(Wad::Header) + entries_.size() * sizeof(Wad::Entry);
    outfile.seek(dataOffset, SEEK_SET);
    for(auto entry: entries_) {
        buffer.reserve(entry.sizeCompressed);
        infile.seek(entry.dataOffset, SEEK_SET);
        infile.read(buffer.data(), entry.sizeCompressed);
        if (entry.type != Wad::Entry::Type::FileRedirection) {
            if (is_oldchecksum_) {
                entry.checksum = XXH3_64bits(buffer.data(), entry.sizeCompressed);
            }
            if (remove_unchanged_entries_) {
                auto const i = index_->checksums().find(entry.xxhash);
                if (i != index_->checksums().end() && i->second == entry.checksum) {
                    progress.consumeData(entry.sizeCompressed);
                    continue;
                }
            }
        }
        entry.dataOffset = dataOffset;
        dataOffset += entry.sizeCompressed;
        outfile.write(buffer.data(), entry.sizeCompressed);
        entries.push_back(entry);
        progress.consumeData(entry.sizeCompressed);
    }
    Wad::Header header{
        { 'R', 'W', '\x03', '\x01' },
        {},
        {},
        static_cast<uint32_t>(entries.size())
    };
    outfile.seek(0, SEEK_SET);
    outfile.write(&header, sizeof(Wad::Header));
    progress.consumeData(sizeof(header));
    outfile.write(entries.data(), entries.size() * sizeof(Wad::Entry));
    progress.consumeData(entries.size() * sizeof(Wad::Entry));

    progress.finishItem();
}

/// Makes a .wad from folder on a filesystem
WadMake::WadMake(fs::path const& path, WadIndex const* index,
                 bool removeUnknownNames, bool removeUnchangedEntries)
    : path_(fs::absolute(path)),
      name_(path_.filename()),
      index_(index),
      remove_unknown_names_(removeUnknownNames),
      remove_unchanged_entries_(removeUnchangedEntries) {
    lcs_trace_func(
                lcs_trace_var(path),
                lcs_trace_var(removeUnknownNames),
                lcs_trace_var(removeUnchangedEntries)
                );
    lcs_assert(fs::is_directory(path_));
    if (removeUnknownNames || removeUnchangedEntries) {
        lcs_assert(index);
    }
    for(auto const& entry: fs::recursive_directory_iterator(path_)) {
        if(entry.is_regular_file()) {
            auto xxhash = pathhash(fs::relative(entry.path(), path_));
            if (removeUnknownNames && !index->checksums().contains(xxhash)) {
                continue;
            }
            entries_.insert_or_assign(xxhash, entry.path());
        }
    }
    size_ = 0;
    for (auto const& kvp: entries_) {
        size_ += fs::file_size(kvp.second);
    }
}

void WadMake::write(fs::path const& dstpath, Progress& progress) const {
    lcs_trace_func(
                lcs_trace_var(dstpath)
                );
    progress.startItem(dstpath, size_);
    fs::create_directories(dstpath.parent_path());
    OutFile outfile(dstpath);
    std::vector<Wad::Entry> entries;
    entries.reserve(entries_.size());
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    uint64_t dataOffset = sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size();
    outfile.seek(dataOffset, SEEK_SET);
    for(auto const& [xxhash, path]: entries_) {
        std::uint64_t uncompressedSize = fs::file_size(path);
        std::uint64_t compressedSize = {};
        uncompressedBuffer.reserve(uncompressedSize);
        {
            InFile infile(path);
            infile.read(uncompressedBuffer.data(), uncompressedSize);
        }
        Wad::Entry entry = {
            xxhash,
            static_cast<uint32_t>(dataOffset),
            {},
            (uint32_t)uncompressedSize,
            {},
            false,
            {},
            {}
        };
        std::u8string extension = path.extension().generic_u8string();
        if (extension.empty()) {
            extension = u8"." + ScanExtension(uncompressedBuffer.data(), (size_t)uncompressedSize);
        }
        if (extension == u8".wpk") {
            entry.type = Wad::Entry::Uncompressed;
            compressedSize = uncompressedSize;
            compressedBuffer.reserve((size_t)compressedSize);
            memcpy(compressedBuffer.data(), uncompressedBuffer.data(), (size_t)compressedSize);
        } else {
            entry.type = Wad::Entry::ZStandardCompressed;
            size_t const estimate = ZSTD_compressBound(uncompressedSize);
            compressedBuffer.reserve(estimate);
            compressedSize = ZSTD_compress(compressedBuffer.data(), estimate,
                                           uncompressedBuffer.data(), uncompressedSize,
                                           3);
        }
        entry.checksum = XXH3_64bits(compressedBuffer.data(), compressedSize);
        if (remove_unchanged_entries_) {
            auto const i = index_->checksums().find(entry.xxhash);
            if (i != index_->checksums().end() && i->second == entry.checksum) {
                progress.consumeData(uncompressedSize);
                continue;
            }
        }
        entry.sizeCompressed = (uint32_t)compressedSize;
        entry.dataOffset = static_cast<uint32_t>(dataOffset);
        dataOffset += entry.sizeCompressed;
        constexpr auto GB = static_cast<std::uint64_t>(1024 * 1024 * 1024);
        lcs_assert(dataOffset <= 2 * GB);
        entries.push_back(entry);
        outfile.write((char const*)compressedBuffer.data(), compressedSize);
        progress.consumeData(uncompressedSize);
    }
    outfile.seek(0, SEEK_SET);
    Wad::Header header{
        { 'R', 'W', '\x03', '\x01' },
        {},
        {},
        static_cast<uint32_t>(entries.size())
    };
    outfile.write((char const*)&header, sizeof(Wad::Header));
    outfile.write((char const*)entries.data(), sizeof(Wad::Entry) * entries.size());
    progress.finishItem();
}
