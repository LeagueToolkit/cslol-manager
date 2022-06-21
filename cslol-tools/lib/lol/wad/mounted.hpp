#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <lol/wad/archive.hpp>
#include <map>
#include <set>

namespace lol::wad {
    struct Mounted {
        fs::path relpath;
        Archive archive;

        static auto make_name(fs::path const& path) noexcept -> std::string {
            std::string name = path.filename().generic_string();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (std::string_view client = ".client"; name.ends_with(client)) {
                name.resize(name.size() - client.size());
            }
            if (std::string_view wad = ".wad"; name.ends_with(".wad")) {
                name.resize(name.size() - wad.size());
            }
            return name;
        };

        auto name() const noexcept -> std::string { return make_name(relpath); }

        auto resolve_conflicts(Mounted const& other, bool ignor) -> void;

        auto remove_unknown(Mounted const& other) noexcept -> std::size_t;

        auto remove_unmodified(Mounted const& other) noexcept -> std::size_t;

        auto read_from_game_file(fs::path const& path, fs::path const& game_path) -> char const*;

        auto read_from_mod_file(fs::path const& path, fs::path const& mod_path) -> char const*;

        auto read_from_mod_folder(fs::path const& path, fs::path const& mod_path) -> char const*;

        auto add_from_mod_legacy_raw(fs::path const& path, fs::path const& mod_path) -> char const*;
    };
}
