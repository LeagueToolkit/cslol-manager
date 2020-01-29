#include "HashTable.hpp"
#include <charconv>

using namespace LCS;

void HashTable::add_from_file(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::string line_hex;
    std::string line_path;
    while(file >> line_hex >> line_path && !line_hex.empty() && !line_path.empty()) {
        if (line_path.size() > 127) {
            continue;
        }
        uint64_t hash;
        auto result = std::from_chars(line_hex.data(), line_hex.data() + line_hex.size(), hash, 16);
        if (result.ec != std::errc{} || result.ptr != (line_hex.data() + line_hex.size())) {
            continue;
        }
        hashes_.insert_or_assign(hash, fs::path(line_path));
    }
}
