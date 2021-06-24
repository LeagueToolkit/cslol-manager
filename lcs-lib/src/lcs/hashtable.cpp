#include "hashtable.hpp"
#include "error.hpp"
#include <fstream>
#include <charconv>

using namespace LCS;

void HashTable::add_from_file(fs::path const& path) {
    lcs_trace_func(
                lcs_trace_var(path)
                );
    std::basic_ifstream<char8_t> file(path, std::ios::binary);
    std::u8string line;
    while(std::getline(file, line) && !line.empty()) {
        auto space = line.find_first_of(u8' ');
        if (space == std::u8string::npos) {
            continue;
        }
        std::u8string_view line_hex = { line.data(), space };
        std::u8string_view line_path = { line.data() + space + 1, line.size() - space - 1 };
        fs::path path_converted = line_path;
        std::u8string normal = path_converted.lexically_normal().generic_u8string();
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
        auto const start = reinterpret_cast<char const*>(line_hex.data());
        auto const end = start + line_hex.size();
        auto const result = std::from_chars(start, end, hash, 16);
        if (result.ec != std::errc{} || result.ptr != end) {
            continue;
        }
        hashes_.insert_or_assign(hash, path_converted.lexically_normal().generic_u8string());
    }
}
