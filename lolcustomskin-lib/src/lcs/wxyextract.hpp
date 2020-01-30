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

        void extract_meta(fs::path const& dest, Progress& progress) const;

        inline auto const& name() const& noexcept {
            return name_;
        }

        inline auto const& author() const& noexcept {
            return author_;
        }

        inline auto const& version() const& noexcept {
            return version_;
        }

        inline auto const& category() const& noexcept {
            return category_;
        }

        inline auto const& subCategory() const& noexcept {
            return subCategory_;
        }
    private:
        fs::path path_;
        mutable std::ifstream file_;

        struct SkinFile {
            fs::path path;
            std::string pathFirst;
            std::string fileGamePath;
            int32_t uncompresedSize;
            int32_t compressedSize;
            std::array<uint8_t, 16> checksum;
            std::array<uint8_t, 32> checksum2;
            uint32_t adler;
            std::string project;
            int32_t offset;
            uint8_t compressionMethod;
        };

        struct Preview {
            enum Type : uint8_t {
                Image = 0,
                Model = 1,
            };
            Type type;
            bool main;
            uint32_t offset;
            uint32_t size;
        };

        std::string name_;
        std::string author_;
        std::string version_;
        std::string category_;
        std::string subCategory_;
        int32_t wxyVersion_;
        std::vector<SkinFile> filesList_;
        std::vector<SkinFile> deleteList_;
        std::vector<Preview> previews_;

        void read_old();
        void read_oink();
        void build_paths();

        void decryptStr(std::string& str) const;
        void decryptStr2(std::string& str) const;
        void decompressStr(std::string& str) const;
    };
}

#endif // WXYEXTRACT_HPP
