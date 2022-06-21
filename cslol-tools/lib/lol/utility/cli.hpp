#pragma once
#include <lol/common.hpp>
#include <lol/fs.hpp>
#include <map>
#include <vector>

namespace lol::utility {
    extern auto set_binary_io() noexcept -> void;

    extern auto argv_fix(int argc, char **argv) noexcept -> std::vector<fs::path>;

    template <typename... ArgsOut>
    inline std::map<std::string, fs::path> argv_parse(std::vector<fs::path> argv, ArgsOut &...positional) noexcept {
        auto flags = std::map<std::string, fs::path>{};
        std::erase_if(argv, [&flags](fs::path const &cur) mutable -> bool {
            if (auto str = cur.generic_string(); str.starts_with("--")) {
                if (auto colon = str.find(':'); colon != std::string::npos) {
                    flags[str.substr(0, colon + 1)] = str.substr(colon + 1);
                } else {
                    flags[str] = "";
                }
                return true;
            }
            return false;
        });
        argv.resize(std::max(argv.size(), sizeof...(positional)));
        std::size_t i = 0;
        ((positional = argv[i++]), ...);
        return flags;
    }
}