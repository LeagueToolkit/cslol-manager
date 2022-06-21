#include <lol/error.hpp>
#include <lol/io/file.hpp>
#include <lol/wad/index.hpp>

using namespace lol;
using namespace lol::wad;

auto Index::from_game_folder(fs::path const& game_path) -> Index {
    lol_trace_func(lol_trace_var("{}", game_path));
    auto index = Index{game_path.filename().generic_string()};
    if (fs::path final_path = game_path / "DATA" / "FINAL"; fs::exists(final_path)) {
        index.add_from_game_folder(final_path, game_path);
    }
    return index;
}

auto Index::add_from_game_folder(fs::path const& path, fs::path const& game_path) -> void {
    for (auto const& dirent : fs::directory_iterator(path)) {
        auto path = dirent.path();
        auto filename = path.filename().generic_string();
        if (dirent.is_directory()) {
            if (filename.ends_with(".wad") || filename.ends_with(".wad.client")) {
                continue;
            }
            this->add_from_game_folder(path, game_path);
        } else {
            if (!filename.ends_with(".wad.client")) {
                continue;
            }
            auto mounted = Mounted{};
            if (auto error = mounted.read_from_game_file(path, game_path)) {
                logw("Failed to open game wad file {}: {}", path, error);
                continue;
            }
            auto name = mounted.name();
            mounts.insert_or_assign(std::move(name), std::move(mounted));
        }
    }
}

auto Index::from_mod_folder(fs::path const& mod_path) -> Index {
    lol_trace_func(lol_trace_var("{}", mod_path));
    lol_throw_if_msg(!fs::exists(mod_path / "META" / "info.json"), "Not valid mod!");
    auto index = Index{mod_path.filename().generic_string()};
    if (fs::path wads_path = mod_path / "WAD"; fs::exists(wads_path)) {
        for (auto const& dirent : fs::directory_iterator(wads_path)) {
            auto path = dirent.path();
            auto mounted = Mounted{};
            if (dirent.is_regular_file()) {
                if (auto error = mounted.read_from_mod_file(path, mod_path)) {
                    logw("Failed to open mod wad file {}: {}", path, error);
                    continue;
                }
            } else if (dirent.is_directory()) {
                if (auto error = mounted.read_from_mod_folder(path, mod_path)) {
                    logw("Failed to open mod wad folder {}: {}", path, error);
                    continue;
                }
            } else {
                continue;
            }
            auto name = mounted.name();
            index.mounts.insert_or_assign(std::move(name), std::move(mounted));
        }
    }
    if (fs::path raw_path = mod_path / "RAW"; fs::exists(raw_path)) {
        auto mounted = Mounted{};
        mounted.relpath = "WAD/_RAW.wad.client";
        mounted.archive = Archive::pack_from_directory(raw_path);
        auto name = mounted.name();
        index.mounts.insert_or_assign(std::move(name), std::move(mounted));
    }
    return index;
}

auto Index::write_to_directory(fs::path const& dir) const -> void {
    lol_trace_func(lol_trace_var("{}", this->name), lol_trace_var("{}", dir));
    fs::create_directories(dir);
    for (auto const& [mount_name, mounted] : mounts) {
        logi("Writing wad: {}", mounted.relpath);
        auto const& [relpath, archive] = mounted;
        archive.write_to_file(dir / relpath);
    }
}

auto Index::cleanup_in_directory(fs::path const& dir) const -> void {
    lol_trace_func(lol_trace_var("{}", this->name), lol_trace_var("{}", dir));
    for (auto const& dirent : fs::recursive_directory_iterator(dir)) {
        auto path = dirent.path();
        auto filename = path.filename().generic_string();
        auto wadclient = std::string_view{".wad.client"};
        if (!filename.ends_with(".wad.client")) {
            continue;
        }
        if (dirent.is_regular_file() && !mounts.contains(Mounted::make_name(path.filename()))) {
            fs::remove(path);
        }
    }
}

auto Index::find_by_overlap(Archive const& archive) const noexcept -> Mounted const* {
    std::size_t max_count = 0;
    Mounted const* found = nullptr;
    for (auto const& [mount_name, mounted] : mounts) {
        auto count = std::size_t{};
        mounted.archive.foreach_overlap(
            archive,
            [&](Archive::Map::const_reference lhs, Archive::Map::const_reference rhs) { ++count; });
        if (count > max_count) {
            max_count = count;
            found = &mounted;
        }
    }
    return found;
}

auto Index::find_by_mount_name(std::string const& mount_name) const noexcept -> Mounted const* {
    if (auto base = mounts.find(mount_name); base != mounts.end()) {
        return &base->second;
    }
    return nullptr;
}

auto Index::find_by_mount_name_or_overlap(std::string const& mount_name, Archive const& archive) const noexcept
    -> Mounted const* {
    if (auto base = find_by_mount_name(mount_name)) {
        return base;
    }
    return find_by_overlap(archive);
}

auto Index::touch() -> void {
    for (auto& [mount_name, mounted] : mounts) {
        mounted.archive.touch();
    }
}

auto Index::rebase_from_game(Index const& game) const -> Index {
    lol_trace_func(lol_trace_var("{}", this->name));
    auto copy = Index{};
    for (auto const& [mount_name, mounted] : mounts) {
        if (mount_name.empty() || !game.mounts.contains(mount_name)) {
            auto base_mounted = game.find_by_overlap(mounted.archive);
            lol_throw_if_msg(!base_mounted, "Failed to find base wad for: {}", mount_name);
            auto fixed = Mounted{mounted.relpath.parent_path() / base_mounted->relpath.filename(), mounted.archive};
            copy.mounts.emplace(base_mounted->name(), std::move(fixed));
            continue;
        }
        copy.mounts.emplace(mount_name, mounted);
    }
    return copy;
}

auto Index::add_overlay_mod(Index const& game, Index const& mod) -> void {
    lol_trace_func(lol_trace_var("{}", mod.name));
    for (auto const& mod_mounted : mod.mounts) {
        auto const& mod_mount_name = mod_mounted.first;
        auto const& mod_mounted_archive = mod_mounted.second.archive;
        auto base_mounted = game.find_by_mount_name_or_overlap(mod_mount_name, mod_mounted_archive);
        lol_throw_if_msg(!base_mounted, "Failed to find base wad for: {}", mod_mount_name);
        auto combined_mounted = mounts.find(mod_mount_name);
        if (combined_mounted == mounts.end()) {
            combined_mounted = mounts.emplace_hint(combined_mounted, base_mounted->name(), *base_mounted);
        }
        combined_mounted->second.archive.merge_in(mod_mounted_archive);
        for (auto const& [extra_mount_name, extra_mounted] : game.mounts) {
            if (&extra_mounted == base_mounted) continue;
            auto overlap_archive = extra_mounted.archive.overlaping(mod_mounted_archive);
            if (overlap_archive.entries.empty()) continue;
            auto combined = mounts.find(extra_mount_name);
            if (combined == mounts.end()) {
                combined = mounts.emplace_hint(combined, extra_mount_name, extra_mounted);
            }
            combined->second.archive.merge_in(overlap_archive);
        }
    }
}

auto Index::resolve_conflicts(Index const& other, bool ignore) -> void {
    lol_trace_func(lol_trace_var("{}", this->name), lol_trace_var("{}", other.name));
    for (auto& [mount_name, mounted] : mounts) {
        for (auto const& [other_mount_name, other_mounted] : other.mounts) {
            mounted.resolve_conflicts(other_mounted, ignore);
        }
    }
}
