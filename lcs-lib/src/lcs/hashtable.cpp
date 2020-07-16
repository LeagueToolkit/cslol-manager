#include "hashtable.hpp"
#include "error.hpp"
#include <fstream>
#include <charconv>

using namespace LCS;

void HashTable::add_from_file(fs::path const& path) {
    lcs_trace_func();
    lcs_trace("log_file: ", path);
    std::ifstream file(path);
    std::string line;
    while(std::getline(file, line) && !line.empty()) {
        auto space = line.find_first_of(' ');
        if (space == std::string::npos) {
            continue;
        }
        std::string_view line_hex = { line.data(), space };
        std::string_view line_path = { line.data() + space + 1, line.size() - space - 1 };
        fs::path path_converted = line_path;
        std::string normal = path_converted.lexically_normal().generic_string();
        if (normal != line_path) {
            continue;
        }
        bool good = true;
        for(auto const& component: path_converted) {
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
        hashes_.insert_or_assign(hash, path_converted.lexically_normal().generic_string());
    }
}
