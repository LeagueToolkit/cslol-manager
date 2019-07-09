#pragma once
#include <functional>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <map>
#include <vector>
#include <optional>
#include "file.hpp"
#include "wad.h"
#include "wxy.h"

using fspath = std::filesystem::path;

struct WadFile {
    File file;
    WadHeader header;
    std::map<uint64_t, WadEntry> entries;
    int32_t data_start;
    int32_t data_size;

    WadFile(std::filesystem::path const& path) : file(path, L"rb") {
        file.read(header);
        for(uint32_t i = 0; i < header.filecount; i++) {
            WadEntry entry{};
            file.read(entry);
            entries[entry.xxhash] = entry;
        }
        data_start = file.tell();
        file.seek_end(0);
        data_size = file.tell() - data_start;
        file.seek_beg(data_start);
    }
};

using wad_files = std::map<std::string, WadFile>;
using wad_mods = std::unordered_map<std::string, wad_files>;
using updatefn = std::function<void(std::string const& name, int64_t bdone, int64_t btotal)>;
using conflictfn = std::function<int(uint64_t xx, std::string const& lm, std::string const& rm)>;

extern void wad_merge(
        fspath const& dstpath,
        std::optional<WadFile> const& base,
        wad_files const& wadps,
        updatefn update,
        int32_t const buffercap = 1024 * 1024);

extern void wad_make(
        fspath const& dstpath,
        fspath const& src,
        updatefn update,
        int32_t const buffercap = 1024 * 1024);

extern void wad_mods_scan_one(
        wad_mods& mods,
        fspath const& mod_path,
        conflictfn = nullptr);

extern void wad_mods_scan_recursive(
        wad_mods& mods,
        fspath const& modspath,
        conflictfn = nullptr);

extern void wad_mods_install(
        wad_mods const& mods,
        fspath const& orgdir,
        fspath const& modsdir,
        updatefn update,
        int32_t const buffercap = 1024 * 1024);

struct HashEntry {
    char name[256-8] = {0};
};
using HashMap = std::unordered_map<uint64_t, HashEntry>;

extern bool load_hashdb(HashMap &hashmap, fspath const& name);

extern bool save_hashdb(HashMap const& hashmap, fspath const& name);

extern bool import_hashes(HashMap& hashmap, fspath const& name);

extern void wad_extract(fspath const& src, fspath const& dst,
                        HashMap const& hashmap,
                        updatefn update = nullptr,
                        int32_t bufferSize = 1024 * 1024);

extern uint32_t xx32_lower(std::string str) noexcept;

extern uint64_t xx64_lower(std::string str) noexcept;

extern std::string read_link(std::string const& path) noexcept;

extern std::string find_league(std::string from) noexcept;

extern std::string read_line(std::string const& from) noexcept;

extern bool write_line(std::string const& to, std::string const& line) noexcept;

extern void wxy_extract(fspath const& dst, fspath const& src, updatefn update,
                        int32_t const buffercap = 1024 * 1024);

