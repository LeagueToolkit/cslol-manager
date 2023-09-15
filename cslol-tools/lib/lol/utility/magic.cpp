#include <cstddef>
#include <lol/utility/magic.hpp>

using namespace lol;
using namespace lol::utility;

[[nodiscard]] std::string_view Magic::find(std::span<char const> data) noexcept {
    constexpr Magic const list[] = {
        {"RW\x01", ".wad"},
        {"RW\x02", ".wad"},
        {"RW\x03", ".wad"},
        {"RLSM", ".releasemanifest"},
        {"RMAN", ".manifest"},
        {"RADS Solution", ".solutionmanifest"},
        {"RST", ".stringtable"},
        {"\x00\x00\x01\x00", ".ico"},
        {"\x47\x49\x46\x38\x37\x61", ".gif"},
        {"\x47\x49\x46\x38\x39\x61", ".gif"},
        {"\x49\x49\x2A\x00", ".tif"},
        {"\x49\x49\x00\x2A", ".tiff"},
        {"\xFF\xD8\xFF\xDB", ".jpg"},
        {"\xFF\xD8\xFF\xE0", ".jpg"},
        {"\xFF\xD8\xFF\xEE", ".jpg"},
        {"\xFF\xD8\xFF\xE1", ".jpg"},
        {"\x42\x4D", ".bmp"},
        {"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", ".png"},
        {"DDS", ".dds"},
        {"TEX\0", ".tex"},
        {"OggS", ".ogg"},
        {"\x00\x01\x00\x00\x00", ".ttf"},
        {"\x74\x72\x75\x65\x00", ".ttf"},
        {"OTTO\x00", ".otf"},
        {"PROP", ".bin"},
        {"PTCH", ".bin"},
        {"BKHD", ".bnk"},
        {"[ObjectBegin]", ".sco"},
        {"r3d2Mesh", ".scb"},
        {"r3d2aims", ".aimesh"},
        {"r3d2anmd", ".anm"},
        {"r3d2canm", ".anm"},
        {"r3d2sklt", ".skl"},
        {"r3d2blnd", ".blnd"},
        {"r3d2wght", ".wgt"},
        {"r3d2", ".wpk"},
        {"\xC3\x4F\xFD\x22", ".skl", 4},
        {"\x33\x22\x11\x00", ".skn"},
        {"PreLoad", ".preload"},
        {"\x1BLuaQ\x00\x01\x04\x04", ".luabin"},
        {"\x1BLuaQ\x00\x01\x04\x08", ".luabin64"},
        {"OPAM", ".mob"},
        {"[MaterialBegin]", ".mat"},
        {"WGEO", ".wgeo"},
        {"MGEO", ".mapgeo"},
        {"OEGM", ".mapgeo"},
        {"NVR\x00", ".nvr"},
        {"{\r\n", ".json"},
        {"[\r\n", ".json"},
        {"<?xml", ".xml"},
        // There is more of those:
        {"\x00\x00\x0A\x00\x00\x00\x00\x00", ".tga"},
        {"\x00\x00\x0A\x00\x00\x00\x00\x00", ".tga"},
        {"\x00\x00\x03\x00\x00\x00\x00\x00", ".tga"},
        {"\x00\x00\x02\x00\x00\x00\x00\x00", ".tga"},
        {"\x50\x4B\x03\x04", ".zip"},
        {"\x52\x61\x72\x21\x1A\x07", ".rar"},
        {"\x1F\x8B\x08", ".gz"},
        {"\x75\x73\x74\x61\x72", ".tar"},
        /*
         * aimesh_ngrid
         * ngrid_overlay
         * rg_overlay
         * ddf
         * cur
         * usm
        */
    };
    for (auto const& [magic, extension, offset] : list) {
        auto src = std::string_view{data.data(), data.size()};
        if (src.size() >= offset) {
            src = src.substr(offset);
        } else {
            continue;
        }
        if (src.starts_with(magic)) {
            return extension;
        }
    }
    return "";
}
