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
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    fs::create_directories(path_);
    for (auto const& file : fs::directory_iterator(path)) {
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
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(filename)
                );
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
    lcs_trace_func(
                lcs_trace_var(this->path_)
                );
    bool found = false;
    for (auto const& file : fs::directory_iterator(path_)) {
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

Mod* ModIndex::install_from_zip(fs::path srcpath, ProgressMulti& progress) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(srcpath)
                );
    fs::path filename = srcpath.filename();
    filename.replace_extension();
    fs::path dest = path_ / filename;
    lcs_assert_msg("Mod already exists!", !fs::exists(dest));
    fs::path tmp = path_ / "tmp";
    if (fs::exists(tmp)) {
        fs::remove_all(tmp);
    }
    fs::create_directories(tmp);
    {
        ModUnZip zip(srcpath);
        zip.extract(tmp, progress);
    }
    fs::create_directories(dest.parent_path());
    lcs_assert(fs::exists(tmp / "META" / "info.json"));
    fs::rename(tmp, dest);
    auto mod = new Mod { dest };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

Mod* ModIndex::make(std::string_view const& fileName,
                    std::string_view const& info,
                    std::string_view const& image,
                    WadMakeQueue const& queue,
                    ProgressMulti& progress)
{
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(fileName),
                lcs_trace_var(image)
                );
    fs::path dest = path_ / fileName;
    lcs_assert_msg("Mod already exists!", !fs::exists(dest));
    fs::path tmp = path_ / "tmp";
    if (fs::exists(tmp)) {
        fs::remove_all(tmp);
    }
    fs::create_directories(tmp);
    fs::create_directories(tmp / "META");
    {
        auto outfile = OutFile(tmp / "META" / "info.json");
        outfile.write(info.data(), info.size());
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

void ModIndex::export_zip(std::string const& filename, fs::path dstpath, ProgressMulti& progress) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(filename),
                lcs_trace_var(dstpath)
                );
    auto i = mods_.find(filename);
    lcs_assert(i != mods_.end());
    i->second->write_zip(dstpath, progress);
}

void ModIndex::remove_mod_wad(std::string const& modFileName, std::string const& wadName) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName),
                lcs_trace_var(wadName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->remove_wad(wadName);
}

void ModIndex::change_mod_info(std::string const& modFileName, std::string const& infoData) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->change_info(infoData);
}

void ModIndex::change_mod_image(std::string const& modFileName, fs::path const& dstpath) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName),
                lcs_trace_var(dstpath)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->change_image(dstpath);
}

void ModIndex::remove_mod_image(std::string const& modFileName) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->remove_image();
}

std::vector<Wad const*> ModIndex::add_mod_wads(std::string const& modFileName, WadMakeQueue& wads,
                                               ProgressMulti& progress, Conflict conflict)
{
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    return i->second->add_wads(wads, progress, conflict);
}


