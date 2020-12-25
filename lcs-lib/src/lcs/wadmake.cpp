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
    auto name = noextension.generic_string();
    auto result = std::from_chars(name.data(), name.data() + name.size(), h, 16);
    if (result.ptr == name.data() + name.size() && result.ec == std::errc{}) {
        return h;
    }
    name = path.generic_string();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    return XXH64(name.data(), name.size(), 0);
}

/// Copies a .wad from filesystem
WadMakeCopy::WadMakeCopy(fs::path const& path)
    : path_(fs::absolute(path)),
      name_(path_.filename().generic_string())
{
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    {
        Wad wad(path);
        entries_ = wad.entries();
    }
    size_ = (size_t)fs::file_size(path_);
}

void WadMakeCopy::write(fs::path const& path, Progress& progress) const {
    lcs_trace_func();
    lcs_trace("path: ", path);
    std::uint64_t sizeData = size();
    progress.startItem(path, sizeData);
    fs::create_directories(path.parent_path());
    fs::copy_file(path_, path);
    progress.consumeData(sizeData);
    progress.finishItem();
}

/// Makes a .wad from folder on a filesystem
WadMake::WadMake(fs::path const& path)
    : path_(fs::absolute(path)),
      name_(path_.filename().generic_string()) {
    lcs_trace_func();
    lcs_trace("path_: ", path_);
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

void WadMake::write(fs::path const& path, Progress& progress) const {
    lcs_trace_func();
    lcs_trace("path: ", path);
    std::uint64_t sizeData = size();
    progress.startItem(path, sizeData);
    fs::create_directories(path.parent_path());
    OutFile outfile(path);
    outfile.seek(sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size(), SEEK_SET);
    std::vector<Wad::Entry> entries;
    entries.reserve(entries_.size());
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    uint64_t dataOffset = (uint64_t)(outfile.tell());
    picosha2::hash256_one_by_one sha256;
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
            sha256.init();
            sha256.process(uncompressedBuffer.data(), uncompressedBuffer.data() + uncompressedSize);
            sha256.finish();
            sha256.get_hash_bytes(entry.sha256.data(), entry.sha256.data() + entry.sha256.size());
            outfile.write((char const*)uncompressedBuffer.data(), uncompressedSize);
        } else {
            size_t estimate = ZSTD_compressBound(uncompressedSize);
            compressedBuffer.reserve(estimate);
            size_t compressedSize = ZSTD_compress(compressedBuffer.data(), estimate,
                                                  uncompressedBuffer.data(), uncompressedSize,
                                                  3);
            entry.sizeCompressed = (uint32_t)compressedSize;
            entry.type = Wad::Entry::ZStandardCompressed;
            sha256.init();
            sha256.process(compressedBuffer.data(), compressedBuffer.data() + compressedSize);
            sha256.finish();
            sha256.get_hash_bytes(entry.sha256.data(), entry.sha256.data() + entry.sha256.size());
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
        { 'R', 'W', '\x03', '\x00' },
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
      name_(path_.filename().generic_string()),
      archive_(archive)
{
    lcs_trace_func();
    lcs_trace("path_: ", path_);
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

void WadMakeUnZip::add(fs::path const& path, unsigned int zipEntry, std::uint64_t size) {
    lcs_trace_func();
    lcs_trace("path: ", path);
    auto xxhash = pathhash(path);
    entries_.insert_or_assign(xxhash, FileEntry{ path, zipEntry, size });
    sizeCalculated_ = false;
}

void WadMakeUnZip::write(fs::path const& path, Progress& progress) const {
    lcs_trace_func();
    lcs_trace("path: ", path);
    auto archive = (mz_zip_archive*)archive_;
    std::uint64_t sizeData = size();
    progress.startItem(path, sizeData);
    fs::create_directories(path.parent_path());
    auto outfile = OutFile(path);
    outfile.seek(sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size(), SEEK_SET);
    std::vector<Wad::Entry> entries = {};
    entries.reserve(entries.size());
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    auto dataOffset = (std::uint64_t)(outfile.tell());
    picosha2::hash256_one_by_one sha256;
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
            sha256.init();
            sha256.process(uncompressedBuffer.data(), uncompressedBuffer.data() + uncompressedSize);
            sha256.finish();
            sha256.get_hash_bytes(entry.sha256.data(), entry.sha256.data() + entry.sha256.size());
            outfile.write((char const*)uncompressedBuffer.data(), uncompressedSize);
        } else {
            size_t estimate = ZSTD_compressBound(uncompressedSize);
            compressedBuffer.reserve(estimate);
            size_t compressedSize = ZSTD_compress(compressedBuffer.data(), estimate,
                                                  uncompressedBuffer.data(), uncompressedSize,
                                                  3);
            entry.sizeCompressed = (uint32_t)compressedSize;
            entry.type = Wad::Entry::ZStandardCompressed;
            sha256.init();
            sha256.process(compressedBuffer.data(), compressedBuffer.data() + compressedSize);
            sha256.finish();
            sha256.get_hash_bytes(entry.sha256.data(), entry.sha256.data() + entry.sha256.size());
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
        { 'R', 'W', '\x03', '\x00' },
        {},
        {},
        static_cast<uint32_t>(entries.size())
    };
    outfile.write((char const*)&header, sizeof(Wad::Header));
    outfile.write((char const*)entries.data(), sizeof(Wad::Entry) * entries.size());
    progress.finishItem();
}

