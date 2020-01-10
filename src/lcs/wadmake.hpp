#ifndef WADMAKE_HPP
#define WADMAKE_HPP
#include "common.hpp"
#include "wad.hpp"
#include "wadindex.hpp"

namespace LCS {
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
}

#endif // WADMAKE_HPP
