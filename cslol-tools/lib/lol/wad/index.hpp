#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <lol/wad/archive.hpp>
#include <lol/wad/mounted.hpp>
#include <map>
#include <set>

namespace lol::wad {
    struct Index {
        using Map = std::map<std::string, Mounted>;

        static auto from_game_folder(fs::path const& game_path) -> Index;

        static auto from_mod_folder(fs::path const& mod_path) -> Index;

        auto write_to_directory(fs::path const& dir) const -> void;

        auto cleanup_in_directory(fs::path const& dir) const -> void;

        auto find_by_overlap(Archive const& archive) const noexcept -> Mounted const*;

        auto find_by_mount_name(std::string const& name) const noexcept -> Mounted const*;

        auto find_by_mount_name_or_overlap(std::string const& name, Archive const& archive) const noexcept
            -> Mounted const*;

        auto touch() -> void;

        auto rebase_from_game(Index const& game) const -> Index;

        auto add_overlay_mod(Index const& game, Index const& mod) -> void;

        auto resolve_conflicts(Index const& other, bool ignore) -> void;

        template <typename Func>
            requires(std::is_invocable_r_v<bool, Func, Map::const_reference>)
        auto remove_filter(Func&& func) -> void {
            for (auto i = mounts.begin(); i != mounts.end();) {
                if (func(*i)) {
                    i = mounts.erase(i);
                } else {
                    ++i;
                }
            }
        }

        std::string name;
        Map mounts = {};

    private:
        auto add_from_game_folder(fs::path const& path, fs::path const& game_path) -> void;
    };
}
