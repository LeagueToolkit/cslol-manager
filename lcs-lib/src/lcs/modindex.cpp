#include "modindex.hpp"
#include "modunzip.hpp"
#include "wadmakequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include <miniz.h>
#include <json.hpp>

using namespace LCS;

ModIndex::ModIndex(fs::path path)
    : path_(fs::absolute(path))
{
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    fs::create_directories(path_);
    for (auto file : fs::directory_iterator(path)) {
        auto dirpath = file.path();
        if (file.is_directory() && dirpath.filename() != "tmp") {
            auto mod = new Mod { dirpath };
            mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
        }
    }
    if (fs::exists(path_ / "tmp")) {
        fs::remove_all(path_ / "tmp");
    }
}

bool ModIndex::remove(std::string const& filename) noexcept {
    lcs_trace_func();
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
    lcs_trace_func();
    bool found = false;
    for (auto file : fs::directory_iterator(path_)) {
        auto dirpath = file.path();
        if (file.is_directory()) {
            auto paths = dirpath.filename().generic_string();
            if (auto i = mods_.find(paths); i == mods_.end()) {
                auto mod = new Mod { dirpath };
                mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
                found = true;
            }
        }
    }
    return found;
}

Mod* ModIndex::install_from_zip(fs::path path, ProgressMulti& progress) {
    lcs_trace_func();
    lcs_trace("path: ", path);
    fs::path filename = path.filename();
    filename.replace_extension();
    fs::path dest = path_ / filename;
    lcs_assert_msg("Mod already exists!", !fs::exists(dest));
    fs::path tmp = path_ / "tmp";
    if (fs::exists(tmp)) {
        fs::remove_all(tmp);
    }
    fs::create_directories(tmp);
    {
        ModUnZip zip(path);
        zip.extract(tmp, progress);
    }
    fs::create_directories(dest.parent_path());
    lcs_assert(fs::exists(tmp / "META" / "info.json"));
    fs::rename(tmp, dest);
    auto mod = new Mod { dest };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

Mod* ModIndex::make(const std::string_view& fileName,
                    const std::string_view& info,
                    const std::string_view& image,
                    WadMakeQueue const& queue,
                    ProgressMulti& progress)
{
    lcs_trace_func();
    fs::path dest = path_ / fileName;
    lcs_trace("dest: ", dest);
    lcs_assert_msg("Mod already exists!", !fs::exists(dest));
    fs::path tmp = path_ / "tmp";
    if (fs::exists(tmp)) {
        fs::remove_all(tmp);
    }
    fs::create_directories(tmp);
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
    fs::create_directories(dest.parent_path());
    fs::rename(tmp, dest);
    auto mod = new Mod { dest };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

void ModIndex::export_zip(const std::string& filename, std::filesystem::path path, ProgressMulti& progress) {
    lcs_trace_func();
    lcs_trace("filename: ", filename);
    auto i = mods_.find(filename);
    lcs_assert(i != mods_.end());
    i->second->write_zip(path, progress);
}

void ModIndex::remove_mod_wad(const std::string& modFileName, const std::string& wadName) {
    lcs_trace_func();
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->remove_wad(wadName);
}

void ModIndex::change_mod_info(const std::string& modFileName, const std::string& infoData) {
    lcs_trace_func();
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->change_info(infoData);
}

void ModIndex::change_mod_image(const std::string& modFileName, std::filesystem::path const & path) {
    lcs_trace_func();
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->change_image(path);
}

void ModIndex::remove_mod_image(const std::string& modFileName) {
    lcs_trace_func();
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->remove_image();
}

std::vector<Wad const*> ModIndex::add_mod_wads(const std::string& modFileName, WadMakeQueue& wads,
                                               ProgressMulti& progress, Conflict conflict)
{
    lcs_trace_func();
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    return i->second->add_wads(wads, progress, conflict);
}


