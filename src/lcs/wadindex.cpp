#include "wadindex.hpp"
#include <algorithm>

using namespace LCS;

WadIndex::WadIndex(fs::path const& path) {
    path_ = fs::absolute(path);
    for (auto file : fs::recursive_directory_iterator(path_ / "DATA")) {
        if (file.is_regular_file()) {
            if (auto filepath = file.path(); filepath.extension() == ".client") {
                auto wad = new Wad { std::move(filepath) };
                wads_.insert_or_assign(wad->name(),  std::unique_ptr<Wad const>{wad});
                for(auto const& entry: wad->entries()) {
                    lookup_.insert(std::pair{entry.xxhash, wad});
                }
            }
        }
    }
    if (lookup_.size() == 0) {
        throw std::runtime_error("Not a wad directory!");
    }
}
