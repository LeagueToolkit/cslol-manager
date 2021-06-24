#include "wadmake.hpp"
#include "error.hpp"
#include "progress.hpp"
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
WadMakeCopy::WadMakeCopy(fs::path const& path)
    : path_(fs::absolute(path)),
      name_(path_.filename())
{
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    auto wad = Wad(path_);
    entries_ = wad.entries();
    size_ = (size_t)fs::file_size(path_);
    is_oldchecksum_ = wad.is_oldchecksum();
}

void WadMakeCopy::write(fs::path const& dstpath, Progress& progress) const {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(dstpath)
                );
    std::uint64_t sizeData = size();
    progress.startItem(dstpath, sizeData);
    fs::create_directories(dstpath.parent_path());
    if (!is_oldchecksum_) {
        fs::copy_file(path_, dstpath);
        progress.consumeData(sizeData);
    } else {
        Wad::Header header{
            { 'R', 'W', '\x03', '\x01' },
            {},
            {},
            static_cast<uint32_t>(entries_.size())
        };
        auto entries = std::vector<Wad::Entry> {};
        InFile infile(path_);
        OutFile outfile(dstpath);
        std::vector<char> buffer;
        for(auto entry: entries_) {
            buffer.reserve(entry.sizeCompressed);
            infile.seek(entry.dataOffset, SEEK_SET);
            infile.read(buffer.data(), entry.sizeCompressed);
            if (entry.type != Wad::Entry::Type::FileRedirection) {
                entry.checksum = XXH3_64bits(buffer.data(), entry.sizeCompressed);
            }
            outfile.seek(entry.dataOffset, SEEK_SET);
            outfile.write(buffer.data(), entry.sizeCompressed);
            progress.consumeData(entry.sizeCompressed);
            entries.push_back(entry);
        }
        outfile.seek(0, SEEK_SET);
        outfile.write(&header, sizeof(header));
        progress.consumeData(sizeof(header));
        outfile.write(entries.data(), entries.size() * sizeof(Wad::Entry));
        progress.consumeData(entries.size() * sizeof(Wad::Entry));
    }
    progress.finishItem();
}

/// Makes a .wad from folder on a filesystem
WadMake::WadMake(fs::path const& path)
    : path_(fs::absolute(path)),
      name_(path_.filename()) {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    lcs_assert(fs::is_directory(path_));
    for(auto const& entry: fs::recursive_directory_iterator(path_)) {
        if(entry.is_regular_file()) {
            auto xxhash = pathhash(fs::relative(entry.path(), path_));
            entries_.insert_or_assign(xxhash, entry.path());
        }
    }
    size_ = std::accumulate(entries_.begin(), entries_.end(), std::uint64_t{0},
                            [](std::uint64_t old, auto const& kvp) -> std::uint64_t {
        return old + fs::file_size(kvp.second);
    });
}

void WadMake::write(fs::path const& dstpath, Progress& progress) const {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(dstpath)
                );
    std::uint64_t sizeData = size();
    progress.startItem(dstpath, sizeData);
    fs::create_directories(dstpath.parent_path());
    OutFile outfile(dstpath);
    outfile.seek(sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size(), SEEK_SET);
    std::vector<Wad::Entry> entries;
    entries.reserve(entries_.size());
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    uint64_t dataOffset = (uint64_t)(outfile.tell());

    for(auto const& [xxhash, path]: entries_) {
        std::uint64_t uncompressedSize = fs::file_size(path);
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
        if (path.extension() == ".wpk") {
            entry.sizeCompressed = (uint32_t)uncompressedSize;
            entry.type = Wad::Entry::Uncompressed;
            entry.checksum = XXH3_64bits(uncompressedBuffer.data(), uncompressedSize);
            outfile.write((char const*)uncompressedBuffer.data(), uncompressedSize);
        } else {
            size_t estimate = ZSTD_compressBound(uncompressedSize);
            compressedBuffer.reserve(estimate);
            size_t compressedSize = ZSTD_compress(compressedBuffer.data(), estimate,
                                                  uncompressedBuffer.data(), uncompressedSize,
                                                  3);
            entry.sizeCompressed = (uint32_t)compressedSize;
            entry.type = Wad::Entry::ZStandardCompressed;
            entry.checksum = XXH3_64bits(compressedBuffer.data(), compressedSize);
            outfile.write((char const*)compressedBuffer.data(), compressedSize);
        }
        entry.dataOffset = static_cast<uint32_t>(dataOffset);
        dataOffset += entry.sizeCompressed;
        constexpr auto GB = static_cast<std::uint64_t>(1024 * 1024 * 1024);
        lcs_assert(dataOffset <= 2 * GB);
        entries.push_back(entry);
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

/// Makes a .wad from folder inside a .zip archive
WadMakeUnZip::WadMakeUnZip(fs::path const& path, void* archive)
    : path_(path),
      name_(path_.filename()),
      archive_(archive)
{
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
}

std::uint64_t WadMakeUnZip::size() const noexcept {
    if (!sizeCalculated_) {
        size_ = std::accumulate(entries_.begin(), entries_.end(), std::uint64_t{0},
                                [](std::uint64_t old, auto const& kvp) -> std::uint64_t{
            return old + kvp.second.size;
        });
        sizeCalculated_ = true;
    }
    return true;
}

void WadMakeUnZip::add(fs::path const& srcpath, unsigned int zipEntry, std::uint64_t size) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(srcpath)
                );
    auto xxhash = pathhash(srcpath);
    entries_.insert_or_assign(xxhash, FileEntry{ srcpath, zipEntry, size });
    sizeCalculated_ = false;
}

void WadMakeUnZip::write(fs::path const& dstpath, Progress& progress) const {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(dstpath)
                );
    auto archive = (mz_zip_archive*)archive_;
    std::uint64_t sizeData = size();
    progress.startItem(dstpath, sizeData);
    fs::create_directories(dstpath.parent_path());
    auto outfile = OutFile(dstpath);
    outfile.seek(sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size(), SEEK_SET);
    std::vector<Wad::Entry> entries = {};
    entries.reserve(entries.size());
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    auto dataOffset = (std::uint64_t)(outfile.tell());

    for(auto const& [xxhash, fileEntry]: entries_) {
        std::uint64_t uncompressedSize = fileEntry.size;
        uncompressedBuffer.reserve(uncompressedSize);
        lcs_assert(mz_zip_reader_extract_to_mem(archive, fileEntry.index,
                                                uncompressedBuffer.data(), fileEntry.size, 0));
        Wad::Entry entry = {
            xxhash,
            static_cast<std::uint32_t>(dataOffset),
            {},
            (uint32_t)uncompressedSize,
            {},
            false,
            {},
            {}
        };
        if (fileEntry.path.extension() == ".wpk") {
            entry.sizeCompressed = (uint32_t)uncompressedSize;
            entry.type = Wad::Entry::Uncompressed;
            entry.checksum = XXH3_64bits(uncompressedBuffer.data(), uncompressedSize);
            outfile.write((char const*)uncompressedBuffer.data(), uncompressedSize);
        } else {
            size_t estimate = ZSTD_compressBound(uncompressedSize);
            compressedBuffer.reserve(estimate);
            size_t compressedSize = ZSTD_compress(compressedBuffer.data(), estimate,
                                                  uncompressedBuffer.data(), uncompressedSize,
                                                  3);
            entry.sizeCompressed = (uint32_t)compressedSize;
            entry.type = Wad::Entry::ZStandardCompressed;
            entry.checksum = XXH3_64bits(compressedBuffer.data(), compressedSize);
            outfile.write((char const*)compressedBuffer.data(), compressedSize);
        }
        entry.dataOffset = static_cast<uint32_t>(dataOffset);
        dataOffset += entry.sizeCompressed;
        constexpr auto GB = static_cast<std::uint64_t>(1024 * 1024 * 1024);
        lcs_assert(dataOffset <= 2 * GB);
        entries.push_back(entry);
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

