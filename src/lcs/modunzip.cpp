#include "modunzip.hpp"
#include "wad.hpp"
#include <fstream>
#include <xxhash.h>
#include <picosha2.hpp>
#include <charconv>
#include <zstd.h>

using namespace LCS;

ModUnZip::ModUnZip(fs::path path)
    : path_(fs::absolute(path)), zip_archive{}
{
    auto paths = path_.generic_string();
    if (auto status = mz_zip_reader_init_file(&zip_archive, paths.c_str(), 0); !status){
        throw std::runtime_error("failed to open zip file\n");
    }

    constexpr auto pathhash = [](fs::path path) -> uint64_t {
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
    };

    constexpr auto pathmerge = [](auto beg, auto end) -> fs::path {
        fs::path result = *beg;
        for(beg++; beg != end; beg++) {
            result /= *beg;
        }
        return result;
    };

    mz_uint numFiles = mz_zip_reader_get_num_files(&zip_archive);
    mz_zip_archive_file_stat stat = {};
    for (mz_uint i = 0; i != numFiles; i++) {
        mz_zip_reader_file_stat(&zip_archive, i, &stat);
        if (stat.m_is_directory || !stat.m_is_supported) {
            continue;
        }
        auto fpath = fs::path(stat.m_filename).lexically_normal();
        auto piter = fpath.begin();
        auto pend = fpath.end();
        if (piter == pend) {
            continue;
        } else if (*piter == "META") {
            piter++;
            if (piter != pend) {
                auto relpath = pathmerge(piter, pend);
                metafile_.push_back(Entry { relpath, i, stat.m_uncomp_size });
                itemCount++;
                dataCount += stat.m_uncomp_size;
            }
        } else if(*piter == "RAW") {
            piter++;
            if (piter != pend) {
                auto relpath = pathmerge(piter, pend);
                auto h = pathhash(relpath);
                wadfolder_["RAW"][h] = Entry { relpath, i, stat.m_uncomp_size};
                itemCount++;
                dataCount += stat.m_uncomp_size;
            }
        } else if(*piter == "WAD") {
            piter++;
            if (piter == pend) {
                continue;
            }
            auto beg = piter;
            piter++;
            auto name = pathmerge(beg, piter);
            if (name.extension() != ".client") {
                continue;
            } else if (piter == pend) {
                wadfile_.push_back(Entry { name, i, stat.m_uncomp_size });
                itemCount++;
                dataCount += stat.m_uncomp_size;
            } else {
                auto relpath = pathmerge(piter, pend);
                auto h = pathhash(relpath);
                wadfolder_[name.generic_string()][h] = Entry { relpath, i, stat.m_uncomp_size };
                itemCount++;
                dataCount += stat.m_uncomp_size;
            }
        }
    }
}

ModUnZip::~ModUnZip() {
    mz_zip_reader_end(&zip_archive);
}

void ModUnZip::extract_meta_files(fs::path const& dest, ProgressMulti& progress) {
    for (auto const& meta: metafile_) {
        auto outpath = dest / "META" / meta.path;
        progress.startItem(outpath, meta.size);
        fs::create_directories(outpath.parent_path());
        auto strpath = outpath.generic_string();
        if (!mz_zip_reader_extract_to_file(&zip_archive, meta.index, strpath.c_str(), 0)) {
            throw std::ofstream::failure("Failed to uncompress the file!");
        }
        progress.consumeData(meta.size);
        progress.finishItem();
    }
}

void ModUnZip::extract_wad_files(fs::path const& dest, ProgressMulti& progress) {
    for (auto const& wad: wadfile_) {
        auto outpath = dest / "WAD" / wad.path;
        progress.startItem(outpath, wad.size);
        fs::create_directories(outpath.parent_path());
        auto strpath = outpath.generic_string();
        if (!mz_zip_reader_extract_to_file(&zip_archive, wad.index, strpath.c_str(), 0)) {
            throw std::ofstream::failure("Failed to uncompress the file!");
        }
        progress.consumeData(wad.size);
        progress.finishItem();
    }
}

void ModUnZip::extract_wad_folders(fs::path const& dest, ProgressMulti& progress) {
    std::vector<char> uncompressedBuffer;
    std::vector<char> compressedBuffer;
    for (auto const& [name, files]: wadfolder_) {
        auto outpath = dest / "WAD" / name;
        fs::create_directories(outpath.parent_path());
        std::ofstream outfile;
        outfile.exceptions(std::ofstream::failbit | std::ifstream::badbit);
        outfile.open(path_, std::ios::binary);

        Wad::Header header{
            { 'R', 'W', '\x03', '\x00' },
            {},
            {},
            static_cast<uint32_t>(files.size())
        };
        outfile.write((char const*)&header, sizeof(Wad::Header));

        std::vector<Wad::Entry> entries = {};
        entries.reserve(files.size());
        outfile.write((char const*)entries.data(), (std::streamsize)(files.size() * sizeof(Wad::Entry)));

        picosha2::hash256_one_by_one sha256;
        for(auto const& [xxhash, file]: files) {
            progress.startItem(file.path, file.size);
            uncompressedBuffer.reserve(file.size);
            if (!mz_zip_reader_extract_to_mem(&zip_archive, file.index, uncompressedBuffer.data(), file.size, 0)) {
                throw std::ofstream::failure("Failed to uncompress the file!");
            }
            auto dataOffset = outfile.tellp();
            if (file.path.extension() == ".wpk") {
                outfile.write(uncompressedBuffer.data(), (std::streamsize)file.size);
                sha256.init();
                sha256.process(uncompressedBuffer.begin(), uncompressedBuffer.end());
                sha256.finish();
                std::array<uint8_t, 8> hash;
                sha256.get_hash_bytes(hash.begin(), hash.end());
                entries.push_back(Wad::Entry {
                                      xxhash,
                                      (uint32_t)dataOffset,
                                      (uint32_t)file.size,
                                      (uint32_t)file.size,
                                      Wad::Entry::Uncompressed,
                                      {},
                                      {},
                                      hash
                                  });
            } else {
                size_t compSizeEst = ZSTD_compressBound(uncompressedBuffer.size());
                compressedBuffer.reserve(compSizeEst);
                size_t compSize = ZSTD_compress(compressedBuffer.data(), compSizeEst,
                                                uncompressedBuffer.data(), file.size, 3);

                outfile.write(compressedBuffer.data(), (std::streamsize)compressedBuffer.size());
                sha256.init();
                sha256.process(compressedBuffer.begin(), compressedBuffer.end());
                sha256.finish();
                std::array<uint8_t, 8> hash;
                sha256.get_hash_bytes(hash.begin(), hash.end());
                entries.push_back(Wad::Entry {
                                      xxhash,
                                      (uint32_t)dataOffset,
                                      (uint32_t)file.size,
                                      (uint32_t)(compSize),
                                      Wad::Entry::ZStandardCompressed,
                                      {},
                                      {},
                                      hash
                                  });
            }
            progress.consumeData(file.size);
            progress.finishItem();
        }

        outfile.seekp((std::streamoff)sizeof(Wad::Header));
        outfile.write((char const*)entries.data(), (std::streamsize)(entries.size() * sizeof(Wad::Entry)));
    }
}

void ModUnZip::extract(fs::path const& dest, ProgressMulti& progress) {
    progress.startMulti(itemCount, dataCount);
    extract_wad_files(dest, progress);
    extract_meta_files(dest, progress);
    extract_wad_folders(dest, progress);
    progress.finishMulti();
}
