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
                lcs_trace_var(path)
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

Mod* ModIndex::install_from_auto(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    std::u8string filename = srcpath.filename().generic_u8string();
    if (fs::is_directory(srcpath)) {
        if (fs::exists(srcpath / "META")) {
            return install_from_folder(srcpath, index, progress);
        } else if (filename.ends_with(u8".wad.client") ||
                   filename.ends_with(u8".wad") ||
                   fs::exists(srcpath / "data") ||
                   fs::exists(srcpath / "data2") ||
                   fs::exists(srcpath / "levels") ||
                   fs::exists(srcpath / "assets") ||
                   fs::exists(srcpath / "assets")||
                   fs::exists(srcpath / "OBSIDIAN_PACKED_MAPPING.txt")) {
            return install_from_wad(srcpath, index, progress);
        } else {
            throw std::runtime_error("Invalid mod folder!");
        }
    } else {
        if (filename.ends_with(u8".zip") || filename.ends_with(u8".fantome")) {
            return install_from_fantome(srcpath, index, progress);
        } else if (filename.ends_with(u8".wxy")) {
            return install_from_wxy(srcpath, index, progress);
        } else {
            throw std::runtime_error("Invalid mod file!");
        }
    }
}

Mod* ModIndex::install_from_folder(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
        lcs_trace_var(srcpath)
        );
    fs::path filename = srcpath.filename();
    lcs_assert_msg("Mod already exists!", !fs::exists(path_ / filename));
    lcs_assert_msg("Not a valid mod file!", fs::exists(srcpath) && fs::is_directory(srcpath));
    return install_from_folder_impl(srcpath, index, progress, filename);
}

Mod* ModIndex::install_from_fantome(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
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

Mod* ModIndex::install_from_folder_impl(fs::path srcpath,
                                        WadIndex const& index,
                                        ProgressMulti& progress,
                                        fs::path const& filename) {
    lcs_assert_msg("Valid mod must contain META/info.json file!", fs::exists(srcpath / "META" / "info.json"));
    fs::path dest = path_ / filename;
    fs::path tmp_make = create_tmp_make();
    // Write meta
    for (auto const& entry: fs::recursive_directory_iterator(srcpath / "META")) {
        auto const src_path = entry.path();
        auto const dst_path = tmp_make / "META" / fs::relative(src_path, srcpath / "META");
        fs::create_directories(tmp_make / dst_path.parent_path());
        fs::copy_file(src_path, dst_path);
    }
    // Write wads
    {
        auto queue = WadMakeQueue(index, false, false); // TODO: expose options
        if (fs::exists(srcpath / "WAD")) {
            for (auto const& entry: fs::directory_iterator(srcpath / "WAD")) {
                queue.addItem(entry.path(), Conflict::Abort);
            }
        }
        if (fs::exists(srcpath / "RAW")) {
            queue.addItem(srcpath / "RAW", Conflict::Abort);
        }
        fs::create_directories(tmp_make / "WAD");
        queue.write(tmp_make / "WAD", progress);
    }
    fs::create_directories(dest.parent_path());
    fs::rename(tmp_make, dest);
    auto mod = new Mod { dest };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

Mod* ModIndex::install_from_wad(fs::path srcpath, WadIndex const& index, ProgressMulti& progress) {
    lcs_trace_func(
        lcs_trace_var(srcpath)
        );
    lcs_assert_msg("Wad mod source does not exist!", fs::exists(srcpath));
    fs::path filename = srcpath.filename().replace_extension();
    if (filename.extension().generic_u8string() == u8".wad") {
        filename = filename.replace_extension();
    }
    fs::path dest = path_ / filename;
    lcs_assert_msg("Mod already exists!", !fs::exists(dest));
    fs::path tmp_make = create_tmp_make();
    // Write meta
    {
        fs::create_directories(tmp_make / "META");
        auto outfile = OutFile(tmp_make / "META" / "info.json");
        auto info = json {
            { "Name", filename.generic_u8string() },
            { "Author", "UNKNOWN" },
            { "Version", "0.0.0" },
            { "Description", "Converted from .wad" },
        }.dump(2);
        outfile.write(info.data(), info.size());
    }
    // Write wads
    {
        auto queue = WadMakeQueue(index, false, false); // TODO: expose options
        queue.addItem(srcpath, Conflict::Abort);
        queue.write(tmp_make / "WAD", progress);
    }
    fs::create_directories(dest.parent_path());
    fs::rename(tmp_make, dest);
    auto mod = new Mod { dest };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

Mod* ModIndex::make(fs::path const& fileName, std::u8string const& info, fs::path const& image) {
    lcs_trace_func(
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
    fs::create_directories(dest.parent_path());
    fs::rename(tmp_make, dest);
    auto mod = new Mod { dest };
    mods_.insert_or_assign(mod->filename(),  std::unique_ptr<Mod>{mod});
    return mod;
}

Mod* ModIndex::get_mod(const fs::path &modFileName) {
    lcs_trace_func(
                lcs_trace_var(modFileName)
                );
    auto i = mods_.find(modFileName);
    lcs_assert(i != mods_.end());
    return i->second.get();
}



