#pragma once
#include <string_view>
#include <array>
#include <unordered_map>
#include <filesystem>

class File;
struct WadFile;

struct WadHeader {
    std::array<char, 4> version = {'R', 'W', '\x03', '\x00'};
    std::array<uint8_t, 256> signature;
    uint64_t checksum;
    uint32_t filecount;
    static inline constexpr size_t SIZE = 4 + 256 + 8 + 4;
};

struct WadEntry {
    uint64_t xxhash;
    int32_t dataOffset;
    int32_t sizeCompressed;
    int32_t sizeUncompressed;
    uint8_t type;
    uint8_t isDuplicate;
    std::array<uint8_t, 2> pad;
    std::array<uint8_t, 8> sha256;
    static inline constexpr size_t SIZE = 32;
};
static_assert(sizeof(WadEntry) == WadEntry::SIZE);

extern void read(File const& file, WadEntry& wad);

extern void write(File const& file, WadEntry const & wad);

extern void read(File const& file, WadHeader& header);

extern void write(File const& file, WadHeader const & header);




