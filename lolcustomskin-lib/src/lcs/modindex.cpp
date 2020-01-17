#include "modindex.hpp"
#include "modunzip.hpp"
#include "wadmakequeue.hpp"
#include <miniz.h>
#include <json.hpp>

using namespace LCS;

ModIndex::ModIndex(fs::path path)
    : path_(fs::absolute(path))
{
    fs::create_directories(path_);
    for (auto file : fs::directory_iterator(path)) {
        auto dirpath = file.path();
        if (file.is_directory() && dirpath.filename() != "tmp") {
            auto mod = new Mod { std::move(dirpath) };
            mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
        }
    }
    if (fs::exists(path_ / "tmp")) {
        fs::remove_all(path_ / "tmp");
    }
}

bool ModIndex::remove(std::string const& filename) noexcept {
    if (auto i = mods_.find(filename); i != mods_.end()) {
        mods_.erase(i);
        try {
            fs::remove_all(path_ / filename);
        } catch(std::exception const&) {
        }
        return true;
    }
    return false;
}

bool ModIndex::refresh() noexcept {
    bool found = false;
    for (auto file : fs::directory_iterator(path_)) {
        auto dirpath = file.path();
        if (file.is_directory()) {
            auto paths = dirpath.filename().generic_string();
            if (auto i = mods_.find(paths); i == mods_.end()) {
                auto mod = new Mod { std::move(dirpath) };
                mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
                found = true;
            }
        }
    }
    return found;
}

Mod* ModIndex::install_from_zip(fs::path path, ProgressMulti& progress) {
    fs::path filename = path.filename();
    filename.replace_extension();
    fs::path dest = path_ / filename;
    if (fs::exists(dest)) {
        throw std::runtime_error("Mod already exists!");
    }
    fs::path tmp = path_ / "tmp";
    if (fs::exists(tmp)) {
        fs::remove_all(tmp);
    }
    {
        ModUnZip zip(path);
        zip.extract(tmp, progress);
    }
    fs::rename(tmp, dest);
    auto mod = new Mod { std::move(dest) };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

Mod* ModIndex::make(const std::string_view& fileName,
                    const std::string_view& info,
                    const std::string_view& image,
                    WadMakeQueue const& queue,
                    ProgressMulti& progress)
{
    fs::path dest = path_ / fileName;
    if (fs::exists(dest)) {
        throw std::runtime_error("Mod already exists!");
    }
    fs::path tmp = path_ / "tmp";
    if (fs::exists(tmp)) {
        fs::remove_all(tmp);
    }
    fs::create_directories(tmp / "META");
    {
        std::ofstream outfile;
        outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        outfile.open(tmp / "META" / "info.json", std::ios::binary);
        outfile.write(info.data(), (std::streamsize)info.size());
    }
    if (!image.empty() && fs::exists(image)) {
        std::error_code error;
        fs::copy_file(image, tmp / "META" / "image.png", error);
    }
    fs::create_directories(tmp / "WAD");
    queue.write(tmp / "WAD", progress);

    fs::rename(tmp, dest);
    auto mod = new Mod { std::move(dest) };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

void ModIndex::export_zip(const std::string& filename, std::filesystem::path path, ProgressMulti& progress) {
    if (auto i = mods_.find(filename); i != mods_.end()) {
        i->second->write_zip(path, progress);
    } else {
        throw std::runtime_error("Mod not found!");
    }
}

void ModIndex::remove_mod_wad(const std::string& modFileName, const std::string& wadName) {
    if (auto i = mods_.find(modFileName); i != mods_.end()) {
        i->second->remove_wad(wadName);
    } else {
        throw std::runtime_error("Mod not found!");
    }
}

void ModIndex::change_mod_info(const std::string& modFileName, const std::string& infoData) {
    if (auto i = mods_.find(modFileName); i != mods_.end()) {
        i->second->change_info(infoData);
    } else {
        throw std::runtime_error("Mod not found!");
    }
}

void ModIndex::change_mod_image(const std::string& modFileName, std::filesystem::path const & path) {
    if (auto i = mods_.find(modFileName); i != mods_.end()) {
        i->second->change_image(path);
    } else {
        throw std::runtime_error("Mod not found!");
    }
}

void ModIndex::remove_mod_image(const std::string& modFileName) {
    if (auto i = mods_.find(modFileName); i != mods_.end()) {
        i->second->remove_image();
    } else {
        throw std::runtime_error("Mod not found!");
    }
}

std::vector<Wad const*> ModIndex::add_mod_wads(const std::string& modFileName, WadMakeQueue& wads,
                                               ProgressMulti& progress, Conflict conflict)
{
    if (auto i = mods_.find(modFileName); i != mods_.end()) {
        return i->second->add_wads(wads, progress, conflict);
    } else {
        throw std::runtime_error("Mod not found!");
    }
}


