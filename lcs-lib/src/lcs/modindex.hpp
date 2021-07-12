#ifndef LCS_MODINDEX_HPP
#define LCS_MODINDEX_HPP
#include "common.hpp"
#include "mod.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

namespace LCS {
    struct ModIndex {
        // Throws std::runtime_error
        ModIndex(fs::path path);
        ModIndex(ModIndex const&) = delete;
        ModIndex(ModIndex&&) = default;
        ModIndex& operator=(ModIndex const&) = delete;
        ModIndex& operator=(ModIndex&&) = delete;

        inline auto const& path() const& noexcept {
            return path_;
        }

        inline auto const& mods() const& noexcept {
            return mods_;
        }

        bool remove(fs::path const& filename) noexcept;

        bool refresh() noexcept;

        // Throws std::runtime_error
        Mod* install_from_auto(fs::path srcpath, WadIndex const& index, ProgressMulti& progress);

        // Throws std::runtime_error
        Mod* install_from_folder(fs::path srcpath, WadIndex const& index, ProgressMulti& progress);

        // Throws std::runtime_error
        Mod* install_from_fantome(fs::path srcpath, WadIndex const& index, ProgressMulti& progress);

        // Throws std::runtime_error
        Mod* install_from_wxy(fs::path srcpath, WadIndex const& index, ProgressMulti& progress);

        // Throws std::runtime_error
        Mod* install_from_wad(fs::path srcpath, WadIndex const& index, ProgressMulti& progress);

        Mod* make(fs::path const& fileName, std::u8string const& info, fs::path const& image);

        // Throws std::runtime_error
        Mod* get_mod(fs::path const& modFileName);
    private:
        fs::path path_;
        std::map<fs::path, std::unique_ptr<Mod>> mods_;
        void clean_tmp_make();
        fs::path create_tmp_make();
        void clean_tmp_extract();
        fs::path create_tmp_extract();

        Mod* install_from_folder_impl(fs::path srcpath, WadIndex const& index, ProgressMulti& progress, fs::path const& filename);
    };
}

#endif // LCS_MODINDEX_HPP
