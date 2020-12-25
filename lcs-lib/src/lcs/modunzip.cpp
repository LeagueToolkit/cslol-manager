#include "modunzip.hpp"
#include "error.hpp"
#include "progress.hpp"
#include <numeric>
#include <utility>

using namespace LCS;

constexpr auto pathmerge = [](auto beg, auto end) -> fs::path {
    fs::path result = *beg;
    for(beg++; beg != end; beg++) {
        result /= *beg;
    }
    return result;
};

ModUnZip::ModUnZip(fs::path path)
    : path_(fs::absolute(path)),
      zip_archive{}
{
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    auto paths = path_.generic_string();
    lcs_assert(mz_zip_reader_init_file(&zip_archive, paths.c_str(), 0));
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
    size_ += std::accumulate(metafile_.begin(), metafile_.end(), std::uint64_t{0},
                            [](std::uint64_t old, auto const& copy) -> std::uint64_t {
                                return old + copy.size;
                            });
    size_ += std::accumulate(wadfile_.begin(), wadfile_.end(), std::uint64_t{0},
                            [](std::uint64_t old, auto const& copy) -> std::uint64_t {
                                return old + copy.size;
                            });
    size_ += std::accumulate(wadfolder_.begin(), wadfolder_.end(), std::uint64_t{0},
                            [](std::uint64_t old, auto const& kvp) -> std::uint64_t {
                                return old + kvp.second->size();
                            });
}

ModUnZip::~ModUnZip() {
    mz_zip_reader_end(&zip_archive);
}

void ModUnZip::extract(fs::path const& dest, ProgressMulti& progress) {
    lcs_trace_func();
    lcs_trace("dest: ", dest);
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
        wadfolder->write(outpath, progress);
    }
    progress.finishMulti();
}

WadMakeUnZip& ModUnZip::addFolder(std::string name) {
    lcs_trace_func();
    lcs_trace("name: ", name);
    auto wad = wadfolder_.find(name);
    if (wad == wadfolder_.end()) {
        auto result = wadfolder_.emplace_hint(wad, name, std::make_unique<WadMakeUnZip>(name, &zip_archive));
        return *result->second;
    } else {
        return *wad->second;
    }
}

void ModUnZip::extractFile(fs::path const& outpath, CopyFile const& file, Progress& progress) {
    lcs_trace_func();
    lcs_trace("outpath: ", outpath);
    progress.startItem(outpath, file.size);
    fs::create_directories(outpath.parent_path());
    auto strpath = outpath.generic_string();
    lcs_assert(mz_zip_reader_extract_to_file(&zip_archive, file.index, strpath.c_str(), 0));
    progress.consumeData(file.size);
    progress.finishItem();
}
