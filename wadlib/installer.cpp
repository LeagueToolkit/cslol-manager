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
#include "zlib.h"
#include "zstd.h"

namespace fs = std::filesystem;

static inline uint64_t str2xxh64(fspath const& path) noexcept {
    std::string str = path.stem().generic_string();
    char* next = nullptr;
    auto result = strtoull(&str[0], &next, 16);
    if(next == &str[str.size()]) {
        return result;
    }
    return xx64(path.lexically_normal().generic_string());
}

WadFile::WadFile(const std::filesystem::path &path)
    : file(path, L"rb") {
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

RawFile::RawFile(const std::filesystem::path &path, uint64_t xxh)
    : file(path, L"rb"), xxhash(xxh) {
    int32_t data_start = file.tell();
    file.seek_end(0);
    data_size = file.tell() - data_start;
    file.seek_beg(data_start);
}

void wad_merge(
        fspath const& dstpath,
        std::optional<WadFile> const& base,
        wad_files const& wad_files,
        updatefn update,
        int32_t const buffercap) {
    WadHeader header{};
    std::map<uint64_t, WadEntry> entries{};

    // A pointer to every WadFile with base WadFile first
    std::vector<WadFile const*> wad_files_ptr;
    wad_files_ptr.reserve(wad_files.size() + 1);

    int64_t bdone = 0;
    int64_t btotal = 0;

    XXH64_state_s xxstate{};
    XXH64_reset(&xxstate, 0);
    // Reads in the base .wad if provided
    if(base) {
        btotal += base->data_size;
        for(auto const& [xx, entry]: base->entries) {
            XXH64_update(&xxstate, &entry, sizeof(WadEntry));
        }
        wad_files_ptr.push_back(&*base);
        header.signature = base->header.signature;
    } else if(wad_files.size()) {
        header.signature = wad_files.begin()->second.header.signature;
    }

    // Reads in all wads in their order
    for(auto const& [xx, wad_file]: wad_files) {
        btotal += wad_file.data_size;
        for(auto const& [xx, entry]: wad_file.entries) {
            XXH64_update(&xxstate, &entry, sizeof(WadEntry));
        }
        wad_files_ptr.push_back(&wad_file);
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
    for(auto const& wad_file: wad_files_ptr) {
        for(auto const& [xx, entry]: wad_file->entries) {
            entries[xx] = {};
        }
    }
    int32_t offset = static_cast<int32_t>(WadHeader::SIZE + (entries.size() * WadEntry::SIZE));
    dstfile.seek_beg(offset);

    // Copy contents
    std::vector<uint8_t> buffer(static_cast<size_t>(buffercap), uint8_t(0));
    for(auto const& wad_file: wad_files_ptr) {
        offset = dstfile.tell();
        for(auto const& [xx, entry]: wad_file->entries) {
            auto& copy = entries[xx];
            copy = {
                entry.xxhash,
                entry.dataOffset - wad_file->data_start + offset,
                entry.sizeCompressed,
                entry.sizeUncompressed,
                entry.type,
                entry.isDuplicate || copy.isDuplicate,
                {0,0},
                entry.sha256
            };
        }

        wad_file->file.seek_beg(wad_file->data_start);
        for(int32_t i = 0; i < wad_file->data_size; i += buffercap) {
            auto const size = std::min(wad_file->data_size - i, buffercap);
            buffer.resize(static_cast<size_t>(size));
            wad_file->file.read(buffer);
            dstfile.write(buffer);
            if(update) {
                bdone += size;
                update(dststr, bdone, btotal);
            }
        }
    }

    dstfile.seek_beg(static_cast<int32_t>(WadHeader::SIZE));
    header.filecount = 0;
    for(auto const& [xx, entry] : entries) {
        if(entry.type == 2 && entry.sizeCompressed <= 5 && entry.sizeUncompressed <= 5) {
            continue;
        }
        dstfile.write(entry);
        header.filecount++;
    }
    dstfile.seek_beg(0);
    dstfile.write(header);
}

void wad_make(
        fspath const& dstpath,
        fspath const& src,
        updatefn update,
        int32_t const buffercap) {
    WadHeader header{};
    std::map<uint64_t, WadEntry> entries{};
    std::map<uint64_t, std::string> redirects{};

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
                for(auto const& dir_ent: fs::recursive_directory_iterator(top)) {
                    if(dir_ent.is_directory()) {
                        continue;
                    }
                    auto relative = fs::relative(dir_ent, src).lexically_normal().generic_string();
                    auto& file = files.emplace_back(dir_ent, xx64(std::move(relative)));
                    btotal += file.data_size;
                }
            } else if(top.path().filename() == "redirections.txt") {
                File orgfile(top, L"rb");
                for(;!orgfile.is_eof();) {
                    std::string line = orgfile.readline();
                    auto const h_start = 0;
                    auto const h_end = line.find_first_of(':');
                    if(h_end == line.size() || h_end < 2) {
                        continue;
                    }
                    auto const n_start = h_end + 1;
                    auto const n_end = line.size() - n_start;
                    auto const h = str2xxh64(line.substr(h_start, h_end));
                    auto const n = line.substr(n_start, n_end);
                    redirects[h] = std::move(n);
                }
            } else {
                auto& file = files.emplace_back(top, str2xxh64(fs::relative(top, src)));
                btotal += file.data_size;
            }
        }
        int32_t offset = static_cast<int32_t>(WadHeader::SIZE +
                                              ((files.size() + redirects.size()) * WadEntry::SIZE));
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

    for(auto const& [xx, redir] : redirects) {
        WadEntry entry = {};
        entry.xxhash = xx;
        entry.type = 2;
        entry.sizeCompressed = static_cast<int32_t>(redir.size() + 5);
        entry.sizeUncompressed = static_cast<int32_t>(redir.size() + 5);
        uint32_t size = static_cast<uint32_t>(redir.size());
        entry.sha256 = {};
        entry.dataOffset = dstfile.tell();
        dstfile.write(size);
        dstfile.write(redir);
        dstfile.write(uint8_t{0});
        entries[xx] = std::move(entry);
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
        fspath const& mod_path,
        conflictfn) {
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
        mods[str].emplace(mod_path.filename().generic_string(), wad_ent.path());
    }
}

void wad_mods_scan_recursive(
        wad_mods &mods,
        fspath const& modspath,
        conflictfn) {
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
        auto org_raw = orgdir / dst_raw;
        if(!fs::exists(org_raw)) {
            continue;
        }
        auto org = fs::directory_entry(org_raw).path();
        auto dst = modsdir / fs::relative(org, orgdir);
        wad_merge(dst, org, wads, update, buffercap);
    }

    for(auto wad_ent : fs::recursive_directory_iterator(modsdir)) {
        if(wad_ent.path().extension() != ".client" || wad_ent.path().extension() == ".wad") {
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

bool load_hashdb(HashMap &hashes, fspath const& name) {
    try {
        File file(name, L"rb");
        uint32_t count = 0;
        file.read(count);
        std::array<char, 4> sig = {};
        file.read(sig);
        if(sig != std::array<char, 4>{'x', 'x', '6', '4'}) {
            return false;
        }
        file.seek_cur(256 - 8);
        hashes.reserve(hashes.size() + static_cast<size_t>(count));
        for(uint32_t i = 0; i < count; i++) {
            uint64_t xx;
            HashEntry entry;
            file.read(xx);
            file.read(entry.name);
            entry.name[sizeof(entry.name) - 1] = '\0';
            hashes[xx] = entry;
        }
    } catch(File::Error const&) {
        return false;
    }
    return true;
}

bool save_hashdb(const HashMap &hashmap, fspath const &name) {
    File file(name, L"wb");
    uint32_t count = static_cast<uint32_t>(hashmap.size());
    file.write(count);
    std::array<char, 4> sig = {'x','x','6','4'};
    file.write(sig);
    file.seek_cur(256 - 8);
    for(auto const& [xx, entry]: hashmap) {
        file.write(xx);
        file.write(entry.name);
    }
    return true;
}

bool import_hashes(HashMap& hashmap, fspath const& name) {
    File file(name, L"rb");
    std::string line;
    fspath path;
    for(; !file.is_eof() ;) {
        HashEntry entry{};
        line = file.readline();
        if(line.empty()) {
            continue;
        }
        uint64_t const h = xx64(line);
        if(line.size() >= sizeof(entry.name)) {
            continue;
        }
        std::copy(line.begin(), line.end(), entry.name);
        hashmap[h] = entry;
    }
    return true;
}

static inline bool unhash(HashMap const& hashmap, uint64_t hash, std::string& result) {
    if(auto h = hashmap.find(hash); h != hashmap.end()) {
        result = h->second.name;
        return false;
    } else {
        char buffer[17];
        sprintf_s(buffer, sizeof(buffer), "%llx", hash);
        result = buffer;
        return true;
    }
}

struct MagicExt {
    char const* const magic;
    size_t const magic_size;
    char const* const ext;
    size_t const ext_size;
    template<size_t SM, size_t SE>
    MagicExt(char const(&magic)[SM], char const(&ext)[SE])
        : magic(magic), magic_size(SM - 1), ext(ext), ext_size(SE - 1) {}
};

static MagicExt const magicext[] = {
    { "OggS", "ogg" },
    { "\x00\x01\x00\x00\x00", "ttf" },
    { "OTTO\x00", "otf" },
    { "DDS", "dds" },
    { "PROP", "bin" },
    { "BKHD", "bnk" },
    { "r3d2Mesh", "scb" },
    { "[ObjectBegin]", "sco" },
    { "r3d2aims", "aimesh" },
    { "r3d2anmd", "anm" },
    { "r3d2canm", "anm" },
    { "r3d2sklt", "skl" },
    { "r3d2wght", "wgt" },
    { "r3d2", "wpk" },
    { "\x33\x22\x11\x00", "skn" },
    { "PreLoadBuildingBlocks = {", "preload" },
    { "\x1BLuaQ\x00\x01\x04\x04", "luabin" },
    { "\x1BLuaQ\x00\x01\x04\x08", "luabin64" },
    { "OPAM", "mob" },
    { "[MaterialBegin]", "mat" },
    { "WGEO", "wgeo" },
    { "MGEO", "mgeo" },
    { "NVR\x00", "nvr" },
};

static inline std::string scan_ext(std::vector<uint8_t> const& buffer) {
    for(auto const& magic: magicext) {
        if(buffer.size() < magic.ext_size) {
            continue;
        }
        if(std::memcmp(magic.magic, &buffer[0], magic.magic_size) == 0) {
            return std::string(magic.ext, magic.ext_size);
        }
    }
    return {};
}


void wad_extract(fspath const& src, fspath const& dst, HashMap const& hashmap,
                 updatefn update, int32_t) {

    if(fs::exists(dst)) {
        std::error_code errorc;
        fs::remove_all(dst, errorc);
        if(errorc) {
            throw File::Error(errorc.message().c_str());
        }
    }
    WadFile file(src);
    std::vector<uint8_t> compressedBuffer;
    std::vector<uint8_t> uncompressedBuffer;
    int32_t maxCompressed = 0;
    int32_t maxUncompressed = 0;
    int64_t btotal = 0;
    int64_t bdone = 0;

    for(auto const& [xx, entry]: file.entries) {
        if(entry.type != 2) {
            btotal += entry.sizeUncompressed;
            if(entry.sizeCompressed > maxCompressed) {
                maxCompressed = entry.sizeCompressed;
            }
            if(entry.sizeUncompressed > maxUncompressed) {
                maxUncompressed = entry.sizeUncompressed;
            }
        }
    }
    compressedBuffer.reserve(static_cast<size_t>(maxCompressed));
    uncompressedBuffer.reserve(static_cast<size_t>(maxUncompressed));

    std::string name;
    for(auto const& [xx, entry]: file.entries) {
        if(entry.type == 2 || entry.type > 3) {
            continue;
        }
        uncompressedBuffer.resize(static_cast<size_t>(entry.sizeUncompressed));
        file.file.seek_beg(entry.dataOffset);
        if(entry.type == 0) {
            file.file.read(uncompressedBuffer);
        } else if(entry.type == 1) {
            compressedBuffer.resize(static_cast<size_t>(entry.sizeCompressed));
            file.file.read(compressedBuffer);
            z_stream strm = {};
            if (inflateInit2_(
                        &strm,
                        16 + MAX_WBITS,
                        ZLIB_VERSION,
                        static_cast<int>(sizeof(z_stream)) != Z_OK)) {
                throw File::Error("Failed to init uncompress stream!");
            }
            strm.next_in = &compressedBuffer[0];
            strm.avail_in = static_cast<unsigned int>(compressedBuffer.size());
            strm.next_out = &uncompressedBuffer[0];
            strm.avail_out = static_cast<unsigned int>(uncompressedBuffer.size());
            inflate(&strm, Z_FINISH);
            inflateEnd(&strm);
        } else if(entry.type == 3) {
            compressedBuffer.resize(static_cast<size_t>(entry.sizeCompressed));
            file.file.read(compressedBuffer);
            ZSTD_decompress(
                        &uncompressedBuffer[0], uncompressedBuffer.size(),
                        &compressedBuffer[0], compressedBuffer.size());
        }
        auto isHashed = unhash(hashmap, xx, name);
        auto dstfile_path = dst / name;
        if(isHashed && dstfile_path.extension().empty()) {
            auto ext = scan_ext(uncompressedBuffer);
            if(!ext.empty()) {
                dstfile_path.replace_extension(ext);
            }
        }
        std::error_code errorc;
        fs::create_directories(dstfile_path.parent_path(), errorc);
        if(errorc) {
            throw File::Error(errorc.message().c_str());
        }
        File dstfile(dstfile_path, L"wb");
        dstfile.write(uncompressedBuffer);
        if(update) {
            bdone += entry.sizeUncompressed;
            update(name, bdone, btotal);
        }
    }

    std::error_code errorc;
    fs::create_directories(dst, errorc);
    if(errorc) {
        throw File::Error(errorc.message().c_str());
    }
    File dstfile(dst / "redirections.txt", L"wb");
    for(auto const& [xx, entry]: file.entries) {
        if(entry.type != 2) {
            continue;
        }
        unhash(hashmap, xx, name);
        file.file.seek_beg(entry.dataOffset);
        if(entry.sizeCompressed < 4) {
            throw File::Error("Redirection buffer size too small!");
        }
        int32_t strsize = 0;
        file.file.read(strsize);
        if(strsize > (entry.sizeCompressed - 4)) {
            throw File::Error("Redirection string size too small!");
        }
        std::string to;
        to.resize(static_cast<size_t>(strsize));
        file.file.read(to);
        dstfile.write(name);
        dstfile.write(':');
        dstfile.write(to);
        dstfile.write('\n');
    }
}
