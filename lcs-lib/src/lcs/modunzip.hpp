#ifndef LCS_MODUNZIP_HPP
#define LCS_MODUNZIP_HPP
#include "common.hpp"
#include "wadmake.hpp"
#include "iofile.hpp"
#include <miniz.h>
#include <vector>
#include <unordered_map>
#include <memory>

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

        inline auto size() const noexcept {
            return size_;
        }
    private:
        struct CopyFile {
            fs::path path;
            mz_uint index;
            std::uint64_t size;
        };
        void extractFile(fs::path const& dest, CopyFile const& file, Progress& progress);

        fs::path path_;
        mz_zip_archive zip_archive = {};
        std::unique_ptr<InFile> infile_ = {};
        std::vector<CopyFile> files_;
        std::uint64_t size_ = 0;
    };
}

#endif // LCS_MODUNZIP_HPP
