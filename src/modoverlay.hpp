#pragma once
#include "process.hpp"
#include <cinttypes>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace LCS {
    struct Process;
    struct ModOverlay {
        static inline constexpr char SCHEMA[] = "lolskinmod-overlay v0 0x%08X 0x%08X 0x%08X";
        static inline constexpr char INFO[] = "lolskinmod-overlay v0 checksum off_fp off_pmeth";
        uint32_t checksum = {};
        PtrStorage off_fp = {};
        PtrStorage off_pmeth = {};

        void save(char const *filename) const noexcept;
        void load(char const *filename) noexcept;
        std::string to_string() const noexcept;
        void from_string(std::string const &) noexcept;
        bool check(Process const &process) const;
        bool scan(Process const &process);
        void patch(Process const &process, std::string_view prefix) const;
    };
}
