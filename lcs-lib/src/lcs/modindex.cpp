#include "modindex.hpp"
#include "modunzip.hpp"
#include "wadmakequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "conflict.hpp"
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
    clean_tmp_make();
    clean_tmp_extract();
    for (auto const& file : fs::directory_iterator(path)) {
        auto dirpath = file.path();
        if (file.is_directory()) {
            auto mod = new Mod { dirpath };
            mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
        }
    }
}

void ModIndex::clean_tmp_make() {
    if (fs::exists(path_ / "tmp_make")) {
        fs::remove_all(path_ / "tmp_make");
    }
}

fs::path ModIndex::create_tmp_make() {
    clean_tmp_make();
    fs::create_directories(path_ / "tmp_make");
    return path_ / "tmp_make";
}

void ModIndex::clean_tmp_extract() {
    if (fs::exists(path_ / "tmp_extract")) {
        fs::remove_all(path_ / "tmp_extract");
    }
}

fs::path ModIndex::create_tmp_extract() {
    clean_tmp_extract();
    fs::create_directories(path_ / "tmp_extract");
    return path_ / "tmp_extract";
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

Mod* ModIndex::install_from_zip(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(srcpath)
                );
    std::string filename = srcpath.filename().replace_extension().generic_string();
    lcs_assert_msg("Mod already exists!", !fs::exists(path_ / filename));
    fs::path tmp_extract = create_tmp_extract();
    {
        ModUnZip zip(srcpath);
        zip.extract(tmp_extract, progress);
    }
    auto info = std::string{};
    auto image = (tmp_extract / "META" / "image.png").generic_string();
    {
        InFile info_file(tmp_extract / "META" / "info.json");
        info.resize(info_file.size());
        info_file.read(info.data(), info.size());
        auto json = nlohmann::json::parse(info);
        if (json.is_object() && json.contains("image") && json["image"].is_string()) {
            auto img_name = json["image"].get<std::string>();
            image = (tmp_extract / "META" / img_name).generic_string();
        }
    }
    auto queue = WadMakeQueue(index);
    for (auto const& entry: fs::directory_iterator(tmp_extract / "WAD")) {
        if (entry.is_directory()) {
            queue.addItem(std::make_unique<WadMake>(entry.path()), Conflict::Abort);
        } else {
            queue.addItem(std::make_unique<WadMakeCopy>(entry.path()), Conflict::Abort);
        }
    }
    auto mod = make(filename, info, image, queue, progress);
    clean_tmp_extract();
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
    fs::path tmp_make = create_tmp_make();
    fs::create_directories(tmp_make / "META");
    {
        auto outfile = OutFile(tmp_make / "META" / "info.json");
        outfile.write(info.data(), info.size());
    }
    if (!image.empty() && fs::exists(image)) {
        std::error_code error;
        fs::copy_file(image, tmp_make / "META" / "image.png", error);
    }
    fs::create_directories(tmp_make / "WAD");
    queue.write(tmp_make / "WAD", progress);
    fs::create_directories(dest.parent_path());
    fs::rename(tmp_make, dest);
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


