#include "wadlib.h"
#include "zlib.h"
#include "zstd.h"
#include <filesystem>
namespace fs = std::filesystem;


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
    size_t const offset;

    template<size_t SM, size_t SE>
    MagicExt(char const(&magic)[SM], char const(&ext)[SE], size_t offset = 0)
        : magic(magic),
          magic_size(SM - 1),
          ext(ext),
          ext_size(SE - 1),
          offset(offset)
    {}
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
        if(buffer.size() < (magic.ext_size + magic.offset)) {
            continue;
        }
        if(std::memcmp(magic.magic, &buffer[magic.offset], magic.magic_size) == 0) {
            return std::string(magic.ext, magic.ext_size);
        }
    }
    return {};
}


void wad_extract(fspath const& src, fspath const& dst, HashMap const& hashmap,
                 updatefn update, int32_t) {

    std::error_code errorc;
    fs::create_directories(dst, errorc);
    if(errorc) {
        throw File::Error(errorc.message().c_str());
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
                        static_cast<int>(sizeof(z_stream))) != Z_OK) {
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
            update("", bdone, btotal);
        }
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
