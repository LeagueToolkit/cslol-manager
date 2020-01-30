#ifndef WXYEXTRACT_HPP
#define WXYEXTRACT_HPP
#include "common.hpp"

namespace LCS {
    struct WxyExtract {
        WxyExtract(fs::path const& path);
        WxyExtract(WxyExtract const&) = delete;
        WxyExtract(WxyExtract&&) = default;
        WxyExtract& operator=(WxyExtract const&) = delete;
        WxyExtract& operator=(WxyExtract&&) = delete;

        void extract_files(fs::path const& dest, Progress& progress) const;
    private:
        fs::path path_;
        mutable std::ifstream file_;
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
        struct Image {
            uint8_t type;
            int32_t offset;
            uint32_t size;
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
        std::vector<Image> images;

        void read_old();
        void read_oink();
    };
}

#endif // WXYEXTRACT_HPP
