#include "HashTable.hpp"
#include <charconv>
#include <cstdio>

using namespace LCS;

void HashTable::add_from_file(fs::path const& path) {
    std::ifstream file(path);
    std::string line_hex;
    std::string line_path;
    while(file >> line_hex >> line_path && !line_hex.empty() && !line_path.empty()) {
        if (line_path.size() > 255) {
            continue;
        }
        fs::path path = line_path;
        bool good = true;
        for(auto const& component: path) {
            if (component.native().size() > 127) {
                good = false;
                break;
            }
        }
        if (!good) {
            continue;
        }
        uint64_t hash;
        auto result = std::from_chars(line_hex.data(), line_hex.data() + line_hex.size(), hash, 16);
        if (result.ec != std::errc{} || result.ptr != (line_hex.data() + line_hex.size())) {
            continue;
        }
        // printf("Hash: %016llX %s\n", hash, line_path.c_str());
        hashes_.insert_or_assign(hash, fs::path(line_path));
    }
}
