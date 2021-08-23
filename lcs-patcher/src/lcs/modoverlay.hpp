#pragma once
#include <cinttypes>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <functional>
#include <string_view>
#include <filesystem>

namespace LCS {
    extern void SleepMS(uint32_t time) noexcept;

    struct ModOverlay {
        static char const SCHEMA[];
        static char const INFO[];

        enum Message {
            M_FOUND,
            M_WAIT_INIT,
            M_SCAN,
            M_NEED_SAVE,
            M_WAIT_PATCHABLE,
            M_PATCH,
            M_WAIT_EXIT,
        };

        ModOverlay();
        ~ModOverlay();
        void save(std::filesystem::path const& filename) const noexcept;
        void load(std::filesystem::path const& filename) noexcept;
        std::string to_string() const noexcept;
        void from_string(std::string const &) noexcept;
        int run(std::function<bool(Message)> update, std::filesystem::path const& profilePath);
    private:
        struct Config;
        std::unique_ptr<Config> config_;
    };
}
