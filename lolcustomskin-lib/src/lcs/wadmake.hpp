#ifndef WADMAKE_HPP
#define WADMAKE_HPP
#include "common.hpp"
#include "wad.hpp"
#include "wadindex.hpp"

namespace LCS {
    struct WadMakeCopy {
        WadMakeCopy(fs::path const& path);

        void write(fs::path const& path, Progress& progress) const;

        inline auto size() const noexcept {
            return size_;
        }

        inline auto const& name() const& noexcept {
            return name_;
        }

        inline auto const& path() const& noexcept {
            return path_;
        }

        inline auto const& entries() const& noexcept {
            return entries_;
        }
    private:
        fs::path path_;
        std::string name_;
        std::vector<Wad::Entry> entries_;
        size_t size_ = 0;
    };

    struct WadMake {
        WadMake(fs::path const& path);

        void write(fs::path const& path, Progress& progress) const;

        inline auto size() const noexcept {
            return size_;
        }

        inline auto const& name() const& noexcept {
            return name_;
        }

        inline auto const& path() const& noexcept {
            return path_;
        }

        inline auto const& entries() const& noexcept {
            return entries_;
        }
    private:
        fs::path path_;
        std::string name_;
        std::map<uint64_t, fs::path> entries_;
        size_t size_ = 0;
    };

    struct WadMakeUnZip {
        struct FileEntry {
            fs::path path;
            unsigned int index;
            size_t size;
        };
        WadMakeUnZip(fs::path const& source, void* archive);

        void add(fs::path const& path, unsigned int zipEntry, size_t size);

        void write(fs::path const& path, Progress& progress) const;

        inline size_t size() const noexcept;

        inline auto const& name() const& noexcept {
            return name_;
        }

        inline auto const& path() const& noexcept {
            return path_;
        }

        inline auto const& entries() const& noexcept {
            return entries_;
        }
    private:
        fs::path path_;
        std::string name_;
        void* archive_;
        std::map<uint64_t, FileEntry> entries_;
        mutable size_t size_ = 0;
        mutable bool sizeCalculated_;
    };
}

#endif // WADMAKE_HPP
