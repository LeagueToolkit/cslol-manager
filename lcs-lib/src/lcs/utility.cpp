#include "utility.hpp"
#include <cstring>

using namespace LCS;

namespace {
    static inline constexpr MagicExt const magicext[] = {
        { "\x00\x00\x01\x00", u8"ico" },
        { "\x47\x49\x46\x38\x37\x61", u8"gif" },
        { "\x47\x49\x46\x38\x39\x61", u8"gif" },
        { "\x49\x49\x2A\x00", u8"tif" },
        { "\x49\x49\x00\x2A", u8"tiff" },
        { "\xFF\xD8\xFF\xDB", u8"jpg" },
        { "\xFF\xD8\xFF\xE0", u8"jpg" },
        { "\xFF\xD8\xFF\xEE", u8"jpg" },
        { "\xFF\xD8\xFF\xE1", u8"jpg" },
        { "\x42\x4D", u8"bmp" },
        { "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", u8"png" },
        { "DDS", u8"dds" },
        { "TEX\0", u8"tex" },
        { "OggS", u8"ogg" },
        { "\x00\x01\x00\x00\x00", u8"ttf" },
        { "OTTO\x00", u8"otf" },
        { "PROP", u8"bin" },
        { "PTCH", u8"bin" },
        { "BKHD", u8"bnk" },
        { "r3d2Mesh", u8"scb" },
        { "[ObjectBegin]", u8"sco" },
        { "r3d2aims", u8"aimesh" },
        { "r3d2anmd", u8"anm" },
        { "r3d2canm", u8"anm" },
        { "r3d2sklt", u8"skl" },
        { "r3d2wght", u8"wgt" },
        { "r3d2", u8"wpk" },
        { "\x33\x22\x11\x00", u8"skn" },
        { "PreLoadBuildingBlocks = {", u8"preload" },
        { "\x1BLuaQ\x00\x01\x04\x04", u8"luabin" },
        { "\x1BLuaQ\x00\x01\x04\x08", u8"luabin64" },
        { "OPAM", u8"mob" },
        { "[MaterialBegin]", u8"mat" },
        { "WGEO", u8"wgeo" },
        { "MGEO", u8"mgeo" },
        { "NVR\x00", u8"nvr" },
    };
}

std::u8string LCS::ScanExtension(char const* data, size_t size) noexcept {
    if (!size) {
        return {};
    }
    for(auto const& magic: magicext) {
        if((magic.magic_size + magic.offset) > size) {
            continue;
        }
        if(std::memcmp(magic.magic, data + magic.offset, magic.magic_size) == 0) {
            return std::u8string(magic.ext, magic.ext_size);
        }
    }
    return {};
}
