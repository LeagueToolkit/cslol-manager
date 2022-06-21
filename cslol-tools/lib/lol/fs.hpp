#pragma once
#include <filesystem>
#include <lol/common.hpp>
#include <string>

namespace lol::fs {
    using namespace std::filesystem;

    using names = fs::path;

    extern auto home_path_str() noexcept -> std::string const &;

    extern auto current_path_str() noexcept -> std::string const &;

    struct tmp_dir {
        tmp_dir(fs::path path);
        ~tmp_dir() noexcept;

        tmp_dir(tmp_dir const &) = delete;
        tmp_dir(tmp_dir &&) = delete;
        tmp_dir &operator=(tmp_dir const &) = delete;
        tmp_dir &operator=(tmp_dir &&) = delete;

        auto move(fs::path const &dst) -> void;

        fs::path path;
    };
}

template <>
struct fmt::formatter<lol::fs::path> : formatter<std::string> {
    template <typename FormatContext>
    auto format(lol::fs::path const &path, FormatContext &ctx) {
        auto result = path.generic_string();
        if (auto const &cd = lol::fs::current_path_str(); !cd.empty() && result.starts_with(cd)) {
            result.replace(0, cd.size(), "./");
        } else if (auto const &home = lol::fs::home_path_str(); !home.empty() && result.starts_with(home)) {
            result.replace(0, home.size(), "~/");
        }
        return formatter<std::string>::format(result, ctx);
    }
};
