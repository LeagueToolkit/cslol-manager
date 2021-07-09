#include "modindex.hpp"
#include "modunzip.hpp"
#include "wadmakequeue.hpp"
#include "error.hpp"
#include "progress.hpp"
#include "conflict.hpp"
#include "wxyextract.hpp"
#include <miniz.h>
#include <json.hpp>

using namespace LCS;

using json = nlohmann::json;

ModIndex::ModIndex(fs::path path)
    : path_(fs::absolute(path))
{
    lcs_hint("If this error persists try re-installing mods in new installation!");
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

bool ModIndex::remove(fs::path const& filename) noexcept {
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
            auto paths = dirpath.filename();
            if (auto i = mods_.find(paths); i == mods_.end()) {
                auto mod = new Mod { dirpath };
                mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
                found = true;
            }
        }
    }
    return found;
}

Mod* ModIndex::install_from_folder(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
        lcs_trace_var(this->path_),
        lcs_trace_var(srcpath)
        );
    fs::path filename = srcpath.filename();
    lcs_assert_msg("Mod already exists!", !fs::exists(path_ / filename));
    lcs_assert_msg("Not a valid mod file!", fs::exists(srcpath) && fs::is_directory(srcpath));
    return install_from_folder_impl(srcpath, index, progress, filename);
}


Mod* ModIndex::install_from_fantome(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
        lcs_trace_var(this->path_),
        lcs_trace_var(srcpath)
        );
    fs::path filename = srcpath.filename().replace_extension();
    lcs_assert_msg("Mod already exists!", !fs::exists(path_ / filename));
    fs::path tmp_extract = create_tmp_extract();
    ModUnZip zip(srcpath);
    zip.extract(tmp_extract, progress);
    auto mod = install_from_folder_impl(tmp_extract, index, progress, filename);
    clean_tmp_extract();
    return mod;
}

Mod* ModIndex::install_from_wxy(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
        lcs_trace_var(this->path_),
        lcs_trace_var(srcpath)
        );
    fs::path filename = srcpath.filename().replace_extension();
    lcs_assert_msg("Mod already exists!", !fs::exists(path_ / filename));
    fs::path tmp_extract = create_tmp_extract();
    WxyExtract wxy(srcpath);
    wxy.extract_files(tmp_extract / "RAW", progress);
    wxy.extract_meta(tmp_extract / "META", progress);
    auto mod = install_from_folder_impl(tmp_extract, index, progress, filename);
    clean_tmp_extract();
    return mod;
}


Mod* ModIndex::install_from_folder_impl(fs::path srcpath, WadIndex const& index, ProgressMulti& progress,
                                        fs::path const& filename) {
    std::u8string info = {};
    fs::path image = srcpath / "META" / "image.png";
    {
        fs::path info_file_path = srcpath / "META" / "info.json";
        lcs_assert_msg("Valid mod must contain META/info.json file!", fs::exists(info_file_path));
        InFile info_file(info_file_path);
        info.resize(info_file.size());
        info_file.read(info.data(), info.size());
        auto json = nlohmann::json::parse(info);
        if (json.is_object() && json.contains("image") && json["image"].is_string()) {
            auto img_name = json["image"].get<std::u8string>();
            image = srcpath / "META" / img_name;
        }
    }
    auto queue = WadMakeQueue(index);
    if (fs::exists(srcpath / "WAD")) {
        for (auto const& entry: fs::directory_iterator(srcpath / "WAD")) {
            if (entry.is_directory()) {
                queue.addItem(std::make_unique<WadMake>(entry.path()), Conflict::Abort);
            } else {
                queue.addItem(std::make_unique<WadMakeCopy>(entry.path()), Conflict::Abort);
            }
        }
    }
    if (fs::exists(srcpath / "RAW")) {
        queue.addItem(std::make_unique<WadMake>(srcpath / "RAW"), Conflict::Abort);
    }
    auto mod = make(filename, info, image, queue, progress);

    return mod;
}

Mod* ModIndex::install_from_wad(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
        lcs_trace_var(this->path_),
        lcs_trace_var(srcpath)
        );
    lcs_assert_msg("Wad mod does not exist!", fs::exists(srcpath));
    fs::path filename = srcpath.filename().replace_extension();
    if (filename.extension().generic_u8string() == u8".wad") {
        filename = filename.replace_extension();
    }
    lcs_assert_msg("Mod already exists!", !fs::exists(path_ / filename));
    auto queue = WadMakeQueue(index);
    if (fs::is_directory(srcpath)) {
        queue.addItem(std::make_unique<WadMake>(srcpath), Conflict::Abort);
    } else {
        queue.addItem(std::make_unique<WadMakeCopy>(srcpath), Conflict::Abort);
    }
    json j = {
        { "Name", filename.generic_u8string() },
        { "Author", "UNKNOWN" },
        { "Version", "0.0.0" },
        { "Description", "Converted from .wad" },
    };
    auto jstr = j.dump(2);
    return make(filename, { jstr.begin(), jstr.end() }, u8"", queue, progress);
}

Mod* ModIndex::make(fs::path const& fileName,
                    std::u8string const& info,
                    fs::path const& image,
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

void ModIndex::export_zip(fs::path const& filename, fs::path dstpath, ProgressMulti& progress) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(filename),
                lcs_trace_var(dstpath)
                );
    auto i = mods_.find(filename);
    lcs_assert(i != mods_.end());
    i->second->write_zip(dstpath, progress);
}

void ModIndex::remove_mod_wad(fs::path const& modFileName, fs::path const& wadName) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName),
                lcs_trace_var(wadName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->remove_wad(wadName);
}

void ModIndex::change_mod_info(fs::path const& modFileName, std::u8string const& infoData) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->change_info(infoData);
}

void ModIndex::change_mod_image(fs::path const& modFileName, fs::path const& dstpath) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName),
                lcs_trace_var(dstpath)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->change_image(dstpath);
}

void ModIndex::remove_mod_image(fs::path const& modFileName) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(modFileName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    i->second->remove_image();
}

std::vector<Wad const*> ModIndex::add_mod_wads(fs::path const& modFileName, WadMakeQueue& wads,
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


