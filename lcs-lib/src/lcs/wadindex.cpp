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

WadIndex::WadIndex(fs::path const& path, bool blacklist) : path_(fs::absolute(path)) {
    lcs_trace_func();
    lcs_trace("path_: ", path_);
    for (auto file : fs::recursive_directory_iterator(path_ / "DATA")) {
        if (file.is_regular_file()) {
            if (auto filepath = file.path(); filepath.extension() == ".client") {
                if (blacklist && is_blacklisted(filepath.filename().generic_string())) {
                    continue;
                }
                try {
                    auto wad = new Wad { filepath };
                    wads_.insert_or_assign(wad->name(),  std::unique_ptr<Wad const>{wad});
                    for(auto const& entry: wad->entries()) {
                        lookup_.insert(std::make_pair(entry.xxhash, wad));
                    }
                } catch(std::runtime_error const& error) {
                    if (error.what() != std::string_view("All zero .wad")) {
                        throw;
                    }
                }
            }
        }
    }
    lcs_assert_msg("Not a wad directory!", lookup_.size() != 0);
}
