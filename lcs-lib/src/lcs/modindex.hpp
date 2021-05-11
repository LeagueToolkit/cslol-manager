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

        bool remove(std::string const& filename) noexcept;

        bool refresh() noexcept;

        // Throws std::runtime_error
        Mod* install_from_zip(fs::path srcpath, WadIndex const& index, ProgressMulti& progress);

        Mod* make(std::string_view const& fileName,
                  std::string_view const& info,
                  std::string_view const& image,
                  WadMakeQueue const& mergequeue,
                  ProgressMulti& progress);

        void export_zip(std::string const& name, fs::path dstpath, ProgressMulti& progress);

        void remove_mod_wad(std::string const& modFileName, std::string const& wadName);

        void change_mod_info(std::string const& modFileName, std::string const& infoData);

        void change_mod_image(std::string const& modFileName, fs::path const& dstpath);

        void remove_mod_image(std::string const& modFileName);

        std::vector<Wad const*> add_mod_wads(std::string const& modFileName, WadMakeQueue& wads,
                                             ProgressMulti& progress, Conflict conflict);
    private:
        fs::path path_;
        std::unordered_map<std::string, std::unique_ptr<Mod>> mods_;
        void clean_tmp_make();
        fs::path create_tmp_make();
        void clean_tmp_extract();
        fs::path create_tmp_extract();
    };
}

#endif // LCS_MODINDEX_HPP
