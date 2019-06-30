#pragma once
#include <functional>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

using fspath = std::filesystem::path;
using wad_paths = std::vector<std::string>;
using wad_mods = std::unordered_map<std::string, wad_paths>;
using updatefn = std::function<void(std::string const& name, int64_t bdone, int64_t btotal)>;

extern void wad_merge(
        fspath const& dstpath,
        fspath const& base,
        wad_paths const& wadps,
        updatefn update,
        int32_t const buffercap = 1024 * 1024);

extern void wad_make(
        fspath const& dstpath,
        fspath const& src,
        updatefn update,
        int32_t const buffercap = 1024 * 1024);

extern void wad_mods_scan_one(
        wad_mods& mods,
        fspath const& mod_path);

extern void wad_mods_scan_recursive(
            wad_mods& mods,
            fspath const& modspath);

extern void wad_mods_install(
        wad_mods const& mods,
        fspath const& orgdir,
        fspath const& modsdir,
        updatefn update,
        int32_t const buffercap = 1024 * 1024);
