#include <lol/error.hpp>
#include <lol/io/file.hpp>
#include <lol/wad/mounted.hpp>

using namespace lol;
using namespace lol::wad;

auto Mounted::resolve_conflicts(Mounted const& other, bool ignore) -> void {
    if (this == &other) return;
    lol_trace_func(lol_trace_var("{}", this->relpath), lol_trace_var("{}", other.relpath));
    archive.foreach_overlap_mut(
        other.archive,
        [&, ignore](Archive::Map::reference old_entry, Archive::Map::const_reference new_entry) -> void {
            // try to reconcile if checksum is same
            if (old_entry.second.checksum() == new_entry.second.checksum()) {
                old_entry.second = new_entry.second;
                return;
            }
            // try to reconcile if uncompressed checksum is same
            if (old_entry.second.into_decompressed().checksum() == new_entry.second.into_decompressed().checksum()) {
                logw("Compressed checksum conflict: {:#016x} in {}", old_entry.first, other.relpath);
                old_entry.second = new_entry.second;
                return;
            }
            // ignore conflict regardles
            if (ignore) {
                old_entry.second = new_entry.second;
                logw("Conflicting file: {:#016x} in {}", old_entry.first, other.relpath);
                return;
            }
            lol_throw_msg("Conflicting file: {:#016x}", old_entry.first);
        });
}

auto Mounted::remove_unknown(Mounted const& other) noexcept -> std::size_t {
    auto count = std::size_t{};
    for (auto i = archive.entries.begin(); i != archive.entries.end();) {
        if (!other.archive.entries.contains(i->first)) {
            i = archive.entries.erase(i);
            ++count;
        } else {
            ++i;
        }
    }
    return count;
}

auto Mounted::remove_unmodified(Mounted const& other) noexcept -> std::size_t {
    auto count = std::size_t{};
    archive.erase_overlap(
        other.archive,
        [&count](Archive::Map::const_reference old_entry, Archive::Map::const_reference new_entry) mutable -> bool {
            // remove if checksum is same
            if (old_entry.second.checksum() == new_entry.second.checksum()) {
                ++count;
                return true;
            }
            // remove if uncompressed checksum is same
            if (old_entry.second.into_decompressed().checksum() == new_entry.second.into_decompressed().checksum()) {
                ++count;
                return true;
            }
            return false;
        });
    return count;
}

auto Mounted::read_from_game_file(fs::path const& path, fs::path const& game_path) -> char const* {
    lol_trace_func(lol_trace_var("{}", path), lol_trace_var("{}", game_path));
    relpath = fs::relative(path, game_path);
    auto src = io::Bytes::from_file(path);
    auto toc = TOC{};
    if (auto error = toc.read(src)) [[unlikely]] {
        return error;
    }
    if (toc.version != TOC::Version::latest()) [[unlikely]] {
        return "Unknown wad version";
    }
    if (toc.entries.size() == 0) {
        return nullptr;
    }
    archive = Archive::read_from_toc(src, toc);
    archive.mark_optimal();
    return nullptr;
}

auto Mounted::read_from_mod_file(fs::path const& path, fs::path const& mod_path) -> char const* {
    lol_trace_func(lol_trace_var("{}", path), lol_trace_var("{}", mod_path));
    relpath = fs::relative(path, mod_path);
    auto filename = path.filename().generic_string();
    if (!filename.ends_with(".wad.client")) {
        return "Not .wad.client file";
    }
    archive = Archive::read_from_file(path);
    return nullptr;
}

auto Mounted::read_from_mod_folder(fs::path const& path, fs::path const& mod_path) -> char const* {
    lol_trace_func(lol_trace_var("{}", path), lol_trace_var("{}", mod_path));
    relpath = fs::relative(path, mod_path);
    auto filename = path.filename().generic_string();
    if (!filename.ends_with(".wad.client") && !filename.ends_with(".wad")) {
        return "Not .wad folder";
    }
    archive = Archive::pack_from_directory(path);
    return nullptr;
}
