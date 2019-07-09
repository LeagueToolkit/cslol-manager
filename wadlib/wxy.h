#pragma once
#include "file.hpp"

struct WxySkin;

extern void read(File const& file, WxySkin& wxy);
extern void read_old(File const& file, WxySkin& wxy);
extern void read_oink(File const& file, WxySkin& wxy);

struct WxySkin {
    struct SkinFile {
        std::string fileGamePath;
        std::string fileUserPath;
        int32_t uncompresedSize;
        int32_t compressedSize;
        std::array<uint8_t, 16> checksum;
        std::array<uint8_t, 32> checksum2;
        uint32_t adler;
        std::string project;
        int32_t offset;
        uint8_t compressionMethod;
    };

    std::string skinName;
    std::string skinAuthor;
    std::string skinVersion;
    std::string category;
    std::string subCategory;
    int32_t wooxyFileVersion;
    std::string filePath;
    std::vector<SkinFile> filesList;
    std::vector<SkinFile> deleteList;

    static inline constexpr uint8_t methodNone = 0;
    static inline constexpr uint8_t methodDeflate = 1;
    static inline constexpr uint8_t methodZlib = 2;
};

