#include "mod.hpp"
#include "wadmakequeue.hpp"
#include <algorithm>
#include <numeric>
#include <miniz.h>

using namespace LCS;

Mod::Mod(fs::path path) : path_(fs::absolute(path)) {
    filename_ = path_.filename().generic_string();
    if (fs::exists(path_ / "META" / "info.json")) {
        std::ifstream infile;
        infile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        infile.open(path_ / "META" / "info.json", std::ifstream::binary);
        auto beg = infile.tellg();
        infile.seekg(0, std::ios::end);
        auto end = infile.tellg();
        infile.seekg(0, std::ios::beg);
        info_.resize((size_t)(end - beg));
        infile.read(info_.data(), (std::streamsize)info_.size());
    } else {
        throw std::ifstream("Missing META/info.json");
    }

    if (fs::exists(path_ / "META" / "image.png")) {
        image_ = path_ / "META" / "image.png";
    } else {
        image_ = "";
    }

    for (auto file : fs::directory_iterator(path_ / "WAD")) {
        if (file.is_regular_file()) {
            if (auto filepath = file.path(); filepath.extension() == ".client") {
                auto wad = new Wad { std::move(filepath) };
                wads_.insert_or_assign(wad->name(),  std::unique_ptr<Wad>{wad});
            }
        }
    }
}

void Mod::write_zip(std::filesystem::path path, ProgressMulti& progress) const {
    size_t sizeTotal = std::accumulate(wads_.begin(), wads_.end(), size_t{0},
                                       [](size_t old, auto const& kvp) -> size_t {
        return old + kvp.second->size();
    });

    progress.startMulti(wads_.size(), sizeTotal);
    if (fs::exists(path)) {
        fs::remove(path);
    }
    auto pathString = path.generic_string();

    mz_zip_archive zip = {};

    if (!mz_zip_writer_init_file(&zip, pathString.c_str(), 0)) {
        throw std::runtime_error("Failed to open .zip for writing!");
    }

    for(auto const& [name, wad]: wads_) {
        std::string dstPath = "WAD\\" + name;
        auto const& wadPath = wad->path();
        std::string srcPath = wadPath.generic_string();
        progress.startItem(wadPath, wad->size());
        if (!mz_zip_writer_add_file(&zip, dstPath.c_str(), srcPath.c_str(),
                                    nullptr, 0, (mz_uint)MZ_DEFAULT_COMPRESSION)) {
            throw std::runtime_error("Failed to add file to .zip");
        }
        progress.consumeData(wad->size());
        progress.finishItem();
    }

    if (fs::exists(image_)) {
        std::string dstPath = "META\\image.png";
        std::string srcPath = image_.generic_string();
        mz_zip_writer_add_file(&zip, dstPath.c_str(), srcPath.c_str(),
                               nullptr, 0, (mz_uint)MZ_DEFAULT_COMPRESSION);
    }

    {
        std::string dstPath = "META\\info.json";
        if (!mz_zip_writer_add_mem(&zip, dstPath.c_str(), info_.data(), info_.size(),
                                   (mz_uint)MZ_DEFAULT_COMPRESSION)) {
            throw std::runtime_error("Failed to add info.json to .zip");
        }
    }


    if (!mz_zip_writer_finalize_archive(&zip)) {
        throw std::runtime_error("Failed to finalize the .zip file!");
    }

    if (!mz_zip_writer_end(&zip)) {
        throw std::runtime_error("Failed to close the .zip!");
    }

    progress.finishMulti();
}

void Mod::remove_wad(std::string const& name) {
    if (auto i = wads_.find(name); i != wads_.end()) {
        auto path = i->second->path();
        wads_.erase(i);
        fs::remove(path);
    } else {
        throw std::runtime_error("Wad name doesn't exist!");
    }
}

void Mod::change_info(std::string const& infoData) {
    std::ofstream outfile;
    outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    outfile.open(path_ / "META" / "info.json", std::ofstream::binary);
    outfile.write(infoData.data(), (std::streamsize)infoData.size());
    info_ = infoData;
}

void Mod::change_image(fs::path const& path) {
    fs::copy_file(path, path_ / "META" / "image.png", fs::copy_options::overwrite_existing);
    image_ = path_ / "META" / "image.png";
}

void Mod::remove_image() {
    fs::remove(path_ / "META" / "image.png");
    image_ = "";
}

std::vector<Wad const*> Mod::add_wads(WadMakeQueue& wads, ProgressMulti& progress, Conflict conflict) {
    std::vector<std::string> removeExisting;
    std::vector<std::string> skipNew;
    std::vector<Wad const*> added;
    added.reserve(wads.size());

    // Handle conflicts
    for (auto const& [name, item]: wads.items()) {
        if (auto w = wads_.find(name); w != wads_.end()) {
            auto const& orgpath = w->second->path();
            auto const& newpath = std::visit([](auto const& item) -> fs::path const& {
                return item.path();
            }, item);
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
