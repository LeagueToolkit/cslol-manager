#include "utility.hpp"

using namespace LCS;

namespace {
    static inline constexpr MagicExt const magicext[] = {
        { "\x00\x00\x01\x00", "ico" },
        { "\x47\x49\x46\x38\x37\x61", "gif" },
        { "\x47\x49\x46\x38\x39\x61", "gif" },
        { "\x49\x49\x2A\x00", "tif" },
        { "\x49\x49\x00\x2A", "tiff" },
        { "\xFF\xD8\xFF\xDB", "jpg" },
        { "\xFF\xD8\xFF\xE0", "jpg" },
        { "\xFF\xD8\xFF\xEE", "jpg" },
        { "\xFF\xD8\xFF\xE1", "jpg" },
        { "\x42\x4D", "bmp" },
        { "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", "png" },
        { "DDS", "dds" },
        { "OggS", "ogg" },
        { "\x00\x01\x00\x00\x00", "ttf" },
        { "OTTO\x00", "otf" },
        { "PROP", "bin" },
        { "PTCH", "bin" },
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
}

std::string LCS::ScanExtension(char const* data, size_t size) noexcept {
    if (!size) {
        return {};
    }
    for(auto const& magic: magicext) {
        if((magic.magic_size + magic.offset) > size) {
            continue;
        }
        if(std::memcmp(magic.magic, data + magic.offset, magic.magic_size) == 0) {
            return std::string(magic.ext, magic.ext_size);
        }
    }
    return {};
}
