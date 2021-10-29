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
#include <span>

using namespace LCS;

inline constexpr auto GB = static_cast<std::uint64_t>(1024 * 1024 * 1024);

WadMakeBase::~WadMakeBase() noexcept {

}

static uint64_t pathhash(fs::path const& path) noexcept {
    uint64_t h;
    fs::path noextension = path;
    noextension.replace_extension();
    auto name = noextension.generic_u8string();
    if (name.size() == 16) {
        auto const start = reinterpret_cast<char const*>(name.data());
        auto const end = start + name.size();
        auto const result = std::from_chars(start, end, h, 16);
        if (result.ptr == end && result.ec == std::errc{}) {
            return h;
        }
    }
    name = path.generic_u8string();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return XXH64(name.data(), name.size(), 0);
}

/// Copies a .wad from filesystem
WadMakeCopy::WadMakeCopy(fs::path const& path, WadIndex const* index, bool removeUnknownNames)
    : path_(fs::absolute(path)),
      name_(path_.filename()),
      index_(index)
{
    lcs_trace_func(
                lcs_trace_var(path),
                lcs_trace_var(removeUnknownNames)
                );
    if (removeUnknownNames) {
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
    can_copy_ = !is_oldchecksum_ && (entries_.size() == wad.entries().size());
}

void WadMakeCopy::write(fs::path const& dstpath, Progress& progress) const {
    lcs_trace_func(
                lcs_trace_var(dstpath)
                );
    progress.startItem(dstpath, size_);
    fs::create_directories(dstpath.parent_path());
    if (can_copy_) {
        fs::copy_file(path_, dstpath, fs::copy_options::overwrite_existing);
        progress.consumeData(size_);
        progress.finishItem();
        return;
    }
    OutFile outfile(dstpath);
    std::vector<char> buffer;
    std::vector<Wad::Entry> entries = {};
    entries.reserve(entries_.size());
    std::uint32_t dataOffset = sizeof(Wad::Header) + entries_.size() * sizeof(Wad::Entry);
    InFile infile(path_);
    outfile.seek(dataOffset, SEEK_SET);
    for(auto entry: entries_) {
        buffer.reserve(entry.sizeCompressed);
        infile.seek(entry.dataOffset, SEEK_SET);
        infile.read(buffer.data(), entry.sizeCompressed);
        if (entry.type != Wad::Entry::Type::FileRedirection) {
            if (is_oldchecksum_) {
                entry.checksum = XXH3_64bits(buffer.data(), entry.sizeCompressed);
            }
        }
        entry.dataOffset = dataOffset;
        dataOffset += entry.sizeCompressed;
        outfile.write(buffer.data(), entry.sizeCompressed);
        entries.push_back(entry);
        progress.consumeData(entry.sizeCompressed);
    }
    Wad::Header header{
        { 'R', 'W', },
        0x03,
        0x02,
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
WadMake::WadMake(fs::path const& path, WadIndex const* index, bool removeUnknownNames)
    : path_(fs::absolute(path)),
      name_(path_.filename()),
      index_(index) {
    lcs_trace_func(
                lcs_trace_var(path),
                lcs_trace_var(removeUnknownNames)
                );
    lcs_assert(fs::is_directory(path_));
    if (removeUnknownNames) {
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
    std::vector<char> inbuffer;
    std::vector<char> outbuffer;
    uint64_t dataOffset = sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size();
    outfile.seek(dataOffset, SEEK_SET);
    for(auto const& [xxhash, path]: entries_) {
        InFile infile(path);
        std::uint64_t uncompressedSize = infile.size();
        lcs_assert(uncompressedSize < 2 * GB);
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
        inbuffer.clear();
        inbuffer.resize((size_t)uncompressedSize);
        infile.read(inbuffer.data(), inbuffer.size());
        std::u8string extension = path.extension().generic_u8string();
        if (extension.empty()) {
            extension = u8"." + ScanExtension(inbuffer.data(), inbuffer.size());
        }
        if (extension == u8".wpk" || extension == u8".bnk") {
            entry.type = Wad::Entry::Uncompressed;
            outbuffer = inbuffer;
        } else {
            entry.type = Wad::Entry::ZStandardCompressed;
            outbuffer.clear();
            outbuffer.resize(ZSTD_compressBound(inbuffer.size()));
            size_t zstd_out_size = ZSTD_compress(outbuffer.data(), outbuffer.size(),
                                                 inbuffer.data(), inbuffer.size(), 0);
            lcs_assert(!ZSTD_isError(zstd_out_size));
            outbuffer.resize(zstd_out_size);
        }
        entry.checksum = XXH3_64bits(outbuffer.data(), outbuffer.size());
        outfile.write(outbuffer.data(), outbuffer.size());
        entry.sizeCompressed = (uint32_t)outbuffer.size();
        entry.dataOffset = static_cast<uint32_t>(dataOffset);
        dataOffset += entry.sizeCompressed;
        lcs_assert(dataOffset <= 2 * GB);
        entries.push_back(entry);
        progress.consumeData(uncompressedSize);
    }
    outfile.seek(0, SEEK_SET);
    Wad::Header header{
        { 'R', 'W', },
        0x03,
        0x02,
        {},
        {},
        static_cast<uint32_t>(entries.size())
    };
    outfile.write((char const*)&header, sizeof(Wad::Header));
    outfile.write((char const*)entries.data(), sizeof(Wad::Entry) * entries.size());
    progress.finishItem();
}
