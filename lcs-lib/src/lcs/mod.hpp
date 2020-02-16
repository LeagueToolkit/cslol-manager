#ifndef LCS_MOD_HPP
#define LCS_MOD_HPP
#include "common.hpp"
#include "wad.hpp"

namespace LCS {
    struct Mod {
        Mod(fs::path path);
        Mod(Mod const&) = delete;
        Mod(Mod&&) = default;
        Mod& operator=(Mod const&) = delete;
        Mod& operator=(Mod&&) = delete;

        inline auto const& path() const& noexcept {
            return path_;
        }

        inline auto const& filename() const& noexcept {
            return filename_;
        }

        inline auto const& info() const& noexcept {
            return info_;
        }

        inline auto const& image() const& noexcept {
            return image_;
        }

        inline auto const& wads() const& noexcept {
            return wads_;
        }

        void write_zip(fs::path path, ProgressMulti& progress) const;

        void remove_wad(std::string const& name);

        void change_info(std::string const& infoData);

        void change_image(fs::path const& path);

        void remove_image();

        std::vector<Wad const*> add_wads(WadMakeQueue& wads, ProgressMulti& progress, Conflict conflict);
    private:
        fs::path path_;
        std::string filename_;
        std::string info_;
        fs::path image_;
        std::map<std::string, std::unique_ptr<Wad>> wads_;
    };
}
#endif // LCS_MOD_HPP
