#include "mod.hpp"
#include "wadmakequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "conflict.hpp"
#include "iofile.hpp"
#include <miniz.h>
#include <numeric>

using namespace LCS;

Mod::Mod(fs::path path) : path_(fs::absolute(path)), filename_(path_.filename().generic_string()) {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );

    lcs_assert(fs::exists(path_ / "META" / "info.json"));
    InFile infile(path_ / "META" / "info.json");
    info_.resize((std::size_t)(infile.size()));
    infile.read(info_.data(), info_.size());

    if (fs::exists(path_ / "META" / "image.png")) {
        image_ = path_ / "META" / "image.png";
    } else {
        image_ = "";
    }

    for (auto const& file : fs::directory_iterator(path_ / "WAD")) {
        if (file.is_regular_file()) {
            if (auto filepath = file.path(); filepath.extension() == ".client") {
                auto wad = new Wad { filepath };
                wads_.insert_or_assign(wad->name(),  std::unique_ptr<Wad>{wad});
            }
        }
    }
}

void Mod::write_zip(fs::path dstpath, ProgressMulti& progress) const {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(dstpath)
                );
    std::uint64_t sizeTotal = std::accumulate(wads_.begin(), wads_.end(), std::uint64_t{0},
                                       [](std::uint64_t old, auto const& kvp) -> std::uint64_t {
        return old + kvp.second->size();
    });
    progress.startMulti(wads_.size(), sizeTotal);
    if (fs::exists(dstpath)) {
        fs::remove(dstpath);
    }
    auto pathString = dstpath.generic_string();
    mz_zip_archive zip = {};
    lcs_assert(mz_zip_writer_init_file(&zip, pathString.c_str(), 0));
    for (auto const& [name, wad]: wads_) {
        std::string dstPath = "WAD\\" + name;
        fs::path const& wadPath = wad->path();
        std::string srcPath = wadPath.generic_string();
        lcs_trace_func(
                    lcs_trace_var(dstPath),
                    lcs_trace_var(wadPath),
                    lcs_trace_var(srcPath)
                    );
        progress.startItem(wadPath, wad->size());
        lcs_assert(mz_zip_writer_add_file(&zip, dstPath.c_str(), srcPath.c_str(),
                                          nullptr, 0, (mz_uint)MZ_DEFAULT_COMPRESSION));
        progress.consumeData(wad->size());
        progress.finishItem();
    }
    if (fs::exists(image_)) {
        std::string dstPath = "META\\image.png";
        std::string srcPath = image_.generic_string();
        mz_zip_writer_add_file(&zip, dstPath.c_str(), srcPath.c_str(),
                               nullptr, 0, (mz_uint)MZ_DEFAULT_COMPRESSION);
    }
    std::string dstPath = "META\\info.json";
    lcs_assert(mz_zip_writer_add_mem(&zip, dstPath.c_str(), info_.data(), info_.size(),
                                     (mz_uint)MZ_DEFAULT_COMPRESSION));
    lcs_assert(mz_zip_writer_finalize_archive(&zip));
    lcs_assert(mz_zip_writer_end(&zip));

    progress.finishMulti();
}

void Mod::remove_wad(std::string const& name) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(name)
                );
    auto i = wads_.find(name);
    lcs_assert(i != wads_.end());
    auto path = i->second->path();
    wads_.erase(i);
    fs::remove(path);
}

void Mod::change_info(std::string const& infoData) {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    auto outfile = OutFile(path_ / "META" / "info.json");
    outfile.write(infoData.data(), infoData.size());
    info_ = infoData;
}

void Mod::change_image(fs::path const& srcpath) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(srcpath)
                );
    fs::copy_file(srcpath, path_ / "META" / "image.png", fs::copy_options::overwrite_existing);
    image_ = path_ / "META" / "image.png";
}

void Mod::remove_image() {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    fs::remove(path_ / "META" / "image.png");
    image_ = "";
}

std::vector<Wad const*> Mod::add_wads(WadMakeQueue& wads, ProgressMulti& progress, Conflict conflict) {
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    std::vector<std::string> removeExisting;
    std::vector<std::string> skipNew;
    std::vector<Wad const*> added;
    added.reserve(wads.items().size());
    // Handle conflicts
    for (auto const& [name, item]: wads.items()) {
        if (auto w = wads_.find(name); w != wads_.end()) {
            auto const& orgpath = w->second->path();
            auto const& newpath = item->path();
            if (conflict == Conflict::Skip) {
                skipNew.push_back(name);
                continue;
            } else if(conflict == Conflict::Overwrite) {
                removeExisting.push_back(name);
                continue;
            } else  if(conflict == Conflict::Abort) {
                throw ConflictError(name, orgpath, newpath);
            }
        }
    }
    for (auto const& name: skipNew) {
        wads.items().erase(name);
    }
    for (auto const& name: removeExisting) {
        remove_wad(name);
    }
    wads.write(path_ / "WAD", progress);
    for(auto const& [name, item]: wads.items()) {
        auto wad = new Wad { path_ / "WAD" / name };
        wads_.insert_or_assign(wad->name(),  std::unique_ptr<Wad>{wad});
        added.push_back(wad);
    }
    return added;
}
