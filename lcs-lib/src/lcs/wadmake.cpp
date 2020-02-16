#include "wadmake.hpp"
#include <cstring>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <xxhash.h>
#include <picosha2.hpp>
#include <charconv>
#include <zstd.h>
#include <miniz.h>

using namespace LCS;

static uint64_t pathhash(fs::path const& path) {
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
    {
        Wad wad(path);
        entries_ = wad.entries();
    }
    size_ = (size_t)fs::file_size(path_);
}

void WadMakeCopy::write(fs::path const& path, Progress& progress) const {
    size_t sizeData = size();
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
    if (!fs::is_directory(path_)) {
        throw std::runtime_error("Not a directory!");
    }
    for(auto const& entry: fs::recursive_directory_iterator(path_)) {
        if(entry.is_regular_file()) {
            auto xxhash = pathhash(fs::relative(entry.path(), path_));
            entries_.insert_or_assign(xxhash, entry.path());
        }
    }
    size_ = std::accumulate(entries_.begin(), entries_.end(), size_t{0},
                            [](size_t old, auto const& kvp) -> size_t {
        return old + fs::file_size(kvp.second);
    });
}

void WadMake::write(fs::path const& path, Progress& progress) const {
    size_t sizeData = size();
    progress.startItem(path, sizeData);

    fs::create_directories(path.parent_path());
    std::ofstream outfile;
    outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    outfile.open(path, std::ios::binary);
    outfile.seekp((std::streamoff)(sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size()));

    std::vector<Wad::Entry> entries;
    entries.reserve(entries_.size());

    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    uint32_t dataOffset = (uint32_t)(outfile.tellp());

    picosha2::hash256_one_by_one sha256;
    for(auto const& [xxhash, path]: entries_) {
        size_t uncompressedSize = fs::file_size(path);

        uncompressedBuffer.reserve(uncompressedSize);
        {
            std::ifstream infile;
            infile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            infile.open(path, std::ios::binary);
            infile.read(uncompressedBuffer.data(), (std::streamsize)uncompressedSize);
        }
        Wad::Entry entry = {
            xxhash,
            dataOffset,
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
            outfile.write((char const*)uncompressedBuffer.data(), (std::streamsize)uncompressedSize);
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
            outfile.write((char const*)compressedBuffer.data(), (std::streamsize)compressedSize);
        }
        entry.dataOffset = dataOffset;
        dataOffset += entry.sizeCompressed;
        entries.push_back(entry);
        progress.consumeData(uncompressedSize);
    }

    outfile.seekp(0);
    Wad::Header header{
        { 'R', 'W', '\x03', '\x00' },
        {},
        {},
        static_cast<uint32_t>(entries.size())
    };
    outfile.write((char const*)&header, (std::streamsize)(sizeof(Wad::Header)));
    outfile.write((char const*)entries.data(), (std::streamsize)(sizeof(Wad::Entry) * entries.size()));
    progress.finishItem();
}

/// Makes a .wad from folder inside a .zip archive
WadMakeUnZip::WadMakeUnZip(fs::path const& path, void* archive)
    : path_(path),
      name_(path_.filename().generic_string()),
      archive_(archive)
{}

size_t WadMakeUnZip::size() const noexcept {
    if (!sizeCalculated_) {
        size_ = std::accumulate(entries_.begin(), entries_.end(), size_t{0},
                                [](size_t old, auto const& kvp) -> size_t{
            return old + kvp.second.size;
        });
        sizeCalculated_ = true;
    }
    return true;
}

void WadMakeUnZip::add(fs::path const& path, unsigned int zipEntry, size_t size) {
    auto xxhash = pathhash(path);
    entries_.insert_or_assign(xxhash, FileEntry{ path, zipEntry, size });
    sizeCalculated_ = false;
}

void WadMakeUnZip::write(fs::path const& path, Progress& progress) const {
    auto archive = (mz_zip_archive*)archive_;
    size_t sizeData = size();
    progress.startItem(path, sizeData);

    fs::create_directories(path.parent_path());
    std::ofstream outfile;
    outfile.exceptions(std::ofstream::failbit | std::ifstream::badbit);
    outfile.open(path, std::ios::binary);
    outfile.seekp((std::streamoff)(sizeof(Wad::Header) + sizeof(Wad::Entry) * entries_.size()));

    std::vector<Wad::Entry> entries = {};
    entries.reserve(entries.size());

    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    uint32_t dataOffset = (uint32_t)(outfile.tellp());

    picosha2::hash256_one_by_one sha256;
    for(auto const& [xxhash, fileEntry]: entries_) {
        size_t uncompressedSize = fileEntry.size;

        uncompressedBuffer.reserve(uncompressedSize);
        if (!mz_zip_reader_extract_to_mem(archive, fileEntry.index,
                                          uncompressedBuffer.data(), fileEntry.size, 0)) {
            throw std::ofstream::failure("Failed to uncompress the file!");
        }
        Wad::Entry entry = {
            xxhash,
            dataOffset,
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
            outfile.write((char const*)uncompressedBuffer.data(), (std::streamsize)uncompressedSize);
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
            outfile.write((char const*)compressedBuffer.data(), (std::streamsize)compressedSize);
        }
        entry.dataOffset = dataOffset;
        dataOffset += entry.sizeCompressed;
        entries.push_back(entry);
        progress.consumeData(uncompressedSize);
    }

    outfile.seekp(0);
    Wad::Header header{
        { 'R', 'W', '\x03', '\x00' },
        {},
        {},
        static_cast<uint32_t>(entries.size())
    };
    outfile.write((char const*)&header, (std::streamsize)(sizeof(Wad::Header)));
    outfile.write((char const*)entries.data(), (std::streamsize)(sizeof(Wad::Entry) * entries.size()));
    progress.finishItem();
}
