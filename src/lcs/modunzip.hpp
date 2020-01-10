#ifndef LCS_MODUNZIP_HPP
#define LCS_MODUNZIP_HPP
#include "common.hpp"
#include <miniz.h>

namespace LCS {
    struct ModUnZip {
        // Throws std::runtime_error
        ModUnZip(fs::path path);
        ~ModUnZip();
        ModUnZip(ModUnZip const&) = delete;
        ModUnZip(ModUnZip&&) = default;
        ModUnZip& operator=(ModUnZip const&) = delete;
        ModUnZip& operator=(ModUnZip&&) = delete;

        // Throws std::runtime_error
        void extract(fs::path const& dest, ProgressMulti& progress);
    private:
        void extract_meta_files(fs::path const& dest, ProgressMulti& progress);
        void extract_wad_files(fs::path const& dest, ProgressMulti& progress);
        void extract_wad_folders(fs::path const& dest, ProgressMulti& progress);

        struct Entry {
            fs::path path;
            mz_uint index;
            size_t size;
        };

        fs::path path_;
        std::vector<Entry> metafile_;
        std::vector<Entry> wadfile_;
        std::unordered_map<std::string, std::map<uint64_t, Entry>> wadfolder_;
        mz_zip_archive zip_archive = {};
        size_t itemCount = 0;
        size_t dataCount = 0;
    };
}

#endif // LCS_MODUNZIP_HPP
