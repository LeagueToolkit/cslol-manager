#include <filesystem>
#include <string>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include "installer.h"
#include "file.hpp"
#include "utils.h"
#include "wad.h"
#include "xxhash.h"
#include "picosha2.hpp"

namespace fs = std::filesystem;

static inline uint64_t str2xxh64(fspath const& path) noexcept {
    std::string str = path.stem().generic_string();
    if(str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        char* next = nullptr;
        auto result = strtoull(&str[2], &next, 16);
        if(next == &str[str.size()]) {
            return result;
        }
    }
    return xx64(path.lexically_normal().generic_string());
}

struct WadFile {
    File file;
    WadHeader header;
    std::vector<WadEntry> entries;
    int32_t data_start;
    int32_t data_size;

    void load_toc() {

    }

    WadFile(std::filesystem::path const& path) : file(path, L"rb") {
        file.read(header);
        entries.resize(static_cast<size_t>(header.filecount));
        file.read(entries);

        data_start = file.tell();
        file.seek_end(0);
        data_size = file.tell() - data_start;
        file.seek_beg(data_start);
    }
};

struct RawFile {
    File file;
    uint64_t xxhash;
    int32_t data_size;
    RawFile(std::filesystem::path const& path, uint64_t xxh) : file(path, L"rb"), xxhash(xxh) {
        int32_t data_start = file.tell();
        file.seek_end(0);
        data_size = file.tell() - data_start;
        file.seek_beg(data_start);
    }
};

void wad_merge(
        fspath const& dstpath,
        fspath const& base,
        wad_paths const& wadps,
        updatefn update,
        int32_t const buffercap) {
    WadHeader header{};
    std::map<uint64_t, WadEntry> entries{};

    std::vector<uint8_t> buffer(static_cast<size_t>(buffercap), uint8_t(0));

    int64_t bdone = 0;
    int64_t btotal = 0;
    std::vector<WadFile> wads;
    wads.reserve(wadps.size());

    XXH64_state_s xxstate{};
    XXH64_reset(&xxstate, 0);
    // Reads in the base .wad if provided
    if(!base.empty()) {
        auto const& wad = wads.emplace_back(base);
        btotal += wad.data_size;
        header.signature = wad.header.signature;
        XXH64_update(&xxstate, &wad.entries[0], wad.entries.size() * sizeof(WadEntry));
    }

    // Reads in all wads in their order
    for(auto const& wadp: wadps) {
        auto const& wad = wads.emplace_back(wadp);
        btotal += wad.data_size;
        XXH64_update(&xxstate, &wad.entries[0], wad.entries.size() * sizeof(WadEntry));
    }

    // Sets the signature to first .wad if no base provided
    if(base.empty() && wads.size() > 0) {
        header.signature = wads.front().header.signature;
    }
    uint64_t currentChecksum = XXH64_digest(&xxstate);


    std::string const dststr = dstpath.generic_string();

    // Check if file exists and compares the TOC checksum
    if(fs::exists(dstpath) && fs::is_regular_file(dstpath) && !fs::is_directory(dstpath)) {
        File dstfile(dstpath, L"rb");
        WadHeader dstheader{};
        dstfile.read(dstheader);
        if(dstheader.checksum == currentChecksum) {
            if(update) {
                update(dststr, 0, 0);
            }
            return;
        }
    }
    header.checksum = currentChecksum;

    fs::create_directories(dstpath.parent_path());
    File dstfile(dstpath, L"wb");

    // Pre-allocate new wad entries and calculate new start offset
    for(auto const& wad: wads) {
        for(auto const& entry: wad.entries) {
            entries[entry.xxhash] = {};
        }
    }
    int32_t offset = static_cast<int32_t>(WadHeader::SIZE + (entries.size() * WadEntry::SIZE));
    dstfile.seek_beg(offset);

    // Copy contents
    for(auto const& wad: wads) {
        // Translate offsets
        offset = dstfile.tell();
        for(auto const& entry: wad.entries) {
            auto& copy = entries[entry.xxhash];
            bool isDuplicate = copy.isDuplicate;
            copy = entry;
            copy.isDuplicate = copy.isDuplicate || isDuplicate;
            copy.dataOffset -= wad.data_start;
            copy.dataOffset += offset;
        }

        // Write files with a buffer
        wad.file.seek_beg(wad.data_start);
        for(int32_t i = 0; i < wad.data_size; i += buffercap) {
            auto const size = std::min(wad.data_size - i, buffercap);
            buffer.resize(static_cast<size_t>(size));
            wad.file.read(buffer);
            dstfile.write(buffer);
            if(update) {
                bdone += size;
                update(dststr, bdone, btotal);
            }
        }
    }

    header.filecount = static_cast<uint32_t>(entries.size());
    dstfile.seek_beg(0);
    dstfile.write(header);
    for(auto const& [xx, entry] : entries) {
        dstfile.write(entry);
    }
}

void wad_make(
        fspath const& dstpath,
        fspath const& src,
        updatefn update,
        int32_t const buffercap) {
    WadHeader header{};
    std::map<uint64_t, WadEntry> entries{};

    std::string const dststr = dstpath.generic_string();

    fs::create_directories(dstpath.parent_path());
    File dstfile(dstpath, L"wb");

    int64_t bdone = 0;

    int64_t btotal = 0;
    std::vector<uint8_t> buffer(static_cast<size_t>(buffercap), uint8_t(0));
    {
        std::vector<RawFile> files{};
        for(auto const& top: fs::directory_iterator(src)) {
            if(top.is_directory()) {
                for(auto const& dir_ent: fs::recursive_directory_iterator(src)) {
                    if(dir_ent.is_directory()) {
                        continue;
                    }
                    auto relative = fs::relative(dir_ent, src).lexically_normal().generic_string();
                    auto& file = files.emplace_back(dir_ent, xx64(std::move(relative)));
                    btotal += file.data_size;
                }
            } else {
                auto& file = files.emplace_back(top, str2xxh64(fs::relative(top, src)));
                btotal += file.data_size;
            }
        }
        int32_t offset = static_cast<int32_t>(WadHeader::SIZE + (files.size() * WadEntry::SIZE));
        dstfile.seek_beg(offset);
        for(auto const& file: files) {
            WadEntry entry = {};
            entry.xxhash = file.xxhash;
            entry.type = 0;
            entry.sizeCompressed = file.data_size;
            entry.sizeUncompressed = file.data_size;
            entry.dataOffset = offset;
            picosha2::hash256_one_by_one sha256;
            sha256.init();
            for(int32_t i = 0; i < file.data_size; i += buffercap) {
                auto const size = std::min(file.data_size - i, buffercap);
                buffer.resize(static_cast<size_t>(size));
                file.file.read(buffer);
                dstfile.write(buffer);
                sha256.process(buffer.cbegin(), buffer.cend());
                if(update) {
                    bdone += size;
                    update(dststr, bdone, btotal);
                }
            }
            sha256.finish();
            sha256.get_hash_bytes(entry.sha256.begin(), entry.sha256.end());
            entries[file.xxhash] = entry;
            offset += file.data_size;
        }
    }

    header.filecount = static_cast<uint32_t>(entries.size());
    dstfile.seek_beg(0);
    dstfile.write(header);

    for(auto const& [xx, entry]: entries) {
        dstfile.write(entry);
    }
}

void wad_mods_scan_one(
        wad_mods &mods,
        fspath const& mod_path) {
    if(!fs::exists(mod_path) || !fs::is_directory(mod_path)) {
        return;
    }
    for(auto wad_ent : fs::recursive_directory_iterator(mod_path)) {
        if(wad_ent.path().extension() != ".client") {
            continue;
        }
        if(wad_ent.is_directory()) {
            continue;
        }
        auto str = fs::relative(wad_ent, mod_path).lexically_normal().generic_string();
        for(auto& c: str) { c = static_cast<char>(::tolower(c)); }
        mods[str].push_back(wad_ent.path().lexically_normal().generic_string());
    }
}

void wad_mods_scan_recursive(
        wad_mods &mods,
        fspath const& modspath) {
    if(!fs::exists(modspath) || !fs::is_directory(modspath)) {
        return;
    }
    for(auto const& mod_ent : fs::directory_iterator(modspath)) {
        if(!fs::is_directory(mod_ent)) {
            continue;
        }
        auto const mod_path = mod_ent.path();
        if(mod_path.extension() == ".disabled") {
            continue;
        }
        wad_mods_scan_one(mods, mod_path);
    }
}

void wad_mods_install(
        wad_mods const&mods,
        fspath const& orgdir,
        fspath const& modsdir,
        updatefn update,
        int32_t const buffercap) {
    for(auto& [dst_raw, wads] : mods) {
        auto org = fs::directory_entry(orgdir / dst_raw);
        if(!org.exists()) {
            continue;
        }
        auto dst = modsdir / fs::relative(org, orgdir);
        wad_merge(dst, org, wads, update, buffercap);
    }

    for(auto wad_ent : fs::recursive_directory_iterator(modsdir)) {
        if(wad_ent.path().extension() != ".client") {
            continue;
        }
        if(wad_ent.is_directory()) {
            continue;
        }
        auto str = fs::relative(wad_ent, modsdir).lexically_normal().generic_string();
        for(auto& c: str) { c = static_cast<char>(::tolower(c)); }
        if(auto f = mods.find(str); f == mods.end() || f->second.empty()) {
            fs::remove(wad_ent);
        }
    }
}


