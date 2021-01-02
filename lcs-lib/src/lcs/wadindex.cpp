#include "wadindex.hpp"
#include "error.hpp"
#include <utility>

using namespace LCS;

static constexpr std::string_view INDEX_BLACKLIST[] = {
    "Map21.wad.client",
    "Map22.wad.client",
};

static bool is_blacklisted(std::string filename) noexcept {
    for (auto name: INDEX_BLACKLIST) {
        if (name == filename) {
            return true;
        }
    }
    return false;
}

WadIndex::WadIndex(fs::path const& path, bool blacklist, bool ignorebad) :
    path_(fs::absolute(path)), blacklist_(blacklist) {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(blacklist_),
                lcs_trace_var(ignorebad)
                );
    last_write_time_ = fs::last_write_time(path_ / "DATA" / "FINAL");
    for (auto const& file : fs::recursive_directory_iterator(path_ / "DATA" / "FINAL")) {
        if (file.is_regular_file()) {
            if (auto filepath = file.path(); filepath.extension() == ".client") {
                if (blacklist_ && is_blacklisted(filepath.filename().generic_string())) {
                    continue;
                }
                last_write_time_ = std::max(last_write_time_, fs::last_write_time(filepath));
                try {
                    auto wad = new Wad { filepath };
                    wads_.insert_or_assign(wad->name(),  std::unique_ptr<Wad const>{wad});
                    for(auto const& entry: wad->entries()) {
                        lookup_.insert(std::make_pair(entry.xxhash, wad));
                    }
                } catch(std::runtime_error const& err) {
                    if (err.what() != std::string_view("All zero .wad") && !ignorebad) {
                        throw;
                    }
                }
            }
        }
    }
    lcs_assert_msg("Not a wad directory!", lookup_.size() != 0);
}

bool WadIndex::is_uptodate() const {
    lcs_trace_func(
                lcs_trace_var(this->path_),
                lcs_trace_var(blacklist_)
                );
    auto new_last_write_time = fs::last_write_time(path_ / "DATA" / "FINAL");
    for (auto const& file : fs::recursive_directory_iterator(path_ / "DATA" / "FINAL")) {
        if (file.is_regular_file()) {
            if (auto filepath = file.path(); filepath.extension() == ".client") {
                if (blacklist_ && is_blacklisted(filepath.filename().generic_string())) {
                    continue;
                }
                new_last_write_time = std::max(new_last_write_time, fs::last_write_time(filepath));
            }
         }
    }
    return new_last_write_time == last_write_time_;
}
