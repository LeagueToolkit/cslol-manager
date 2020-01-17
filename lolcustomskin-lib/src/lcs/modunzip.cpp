#include "modunzip.hpp"
#include <fstream>
#include <numeric>
#include <algorithm>

using namespace LCS;

ModUnZip::ModUnZip(fs::path path)
    : path_(fs::absolute(path)),
      zip_archive{}
{
    auto paths = path_.generic_string();
    if (auto status = mz_zip_reader_init_file(&zip_archive, paths.c_str(), 0); !status){
        throw std::runtime_error("failed to open zip file\n");
    }

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
                metafile_.push_back(CopyFile { relpath, i, stat.m_uncomp_size });
            }
        } else if(*piter == "RAW") {
            piter++;
            if (piter != pend) {
                auto relpath = pathmerge(piter, pend);
                addFolder("RAW.wad.client").add(relpath, i, stat.m_uncomp_size);
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
                wadfile_.push_back(CopyFile { name, i, stat.m_uncomp_size });
            } else {
                auto relpath = pathmerge(piter, pend);
                addFolder(name.generic_string()).add(relpath, i, stat.m_uncomp_size);
            }
        }
    }

    items_ += metafile_.size();
    items_ += wadfile_.size();
    items_ += wadfolder_.size();

    size_ += std::accumulate(metafile_.begin(), metafile_.end(), size_t{0},
                            [](size_t old, auto const& copy) -> size_t {
                                return old + copy.size;
                            });
    size_ += std::accumulate(wadfile_.begin(), wadfile_.end(), size_t{0},
                            [](size_t old, auto const& copy) -> size_t {
                                return old + copy.size;
                            });
    size_ += std::accumulate(wadfolder_.begin(), wadfolder_.end(), size_t{0},
                            [](size_t old, auto const& kvp) -> size_t {
                                return old + kvp.second.size();
                            });
}

ModUnZip::~ModUnZip() {
    mz_zip_reader_end(&zip_archive);
}

void ModUnZip::extract(fs::path const& dest, ProgressMulti& progress) {
    progress.startMulti(items_, size_);
    for (auto const& file: metafile_) {
        auto outpath = dest / "META" / file.path;
        extractFile(outpath, file, progress);
    }
    for (auto const& file: wadfile_) {
        auto outpath = dest / "WAD" / file.path;
        extractFile(outpath, file, progress);
    }
    for (auto const& [name, wadfolder]: wadfolder_) {
        auto outpath = dest / "WAD" / name;
        wadfolder.write(outpath, progress);
    }
    progress.finishMulti();
}

WadMakeUnZip& ModUnZip::addFolder(std::string name) {
    if (auto wad = wadfolder_.find(name); wad == wadfolder_.end()) {
        return wadfolder_.insert(wad, std::pair {
                                    name, WadMakeUnZip(name, &zip_archive)
                                })->second;
    } else {
        return wad->second;
    }
}

void ModUnZip::extractFile(fs::path const& outpath, CopyFile const& file, Progress& progress) {
    progress.startItem(outpath, file.size);
    fs::create_directories(outpath.parent_path());
    auto strpath = outpath.generic_string();
    if (!mz_zip_reader_extract_to_file(&zip_archive, file.index, strpath.c_str(), 0)) {
        throw std::ofstream::failure("Failed to uncompress the file!");
    }
    progress.consumeData(file.size);
    progress.finishItem();
}
