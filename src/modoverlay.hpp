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
        static inline constexpr char SCHEMA[] = "lolskinmod-patcher v1 0x%08X 0x%08X 0x%08X";
        static inline constexpr char INFO[] = "lolskinmod-patcher v1 checksum off_rsa_meth off_unused";
        uint32_t checksum = {};
        PtrStorage off_rsa_meth = {};
        PtrStorage off_unused = {};

        void save(char const *filename) const noexcept;
        void load(char const *filename) noexcept;
        std::string to_string() const noexcept;
        void from_string(std::string const &) noexcept;
        bool check(Process const &process) const;
        void scan(Process const &process);
        void wait_patchable(Process const &process, uint32_t timeout = 60 * 1000);
        void patch(Process const &process, std::string prefix) const;
    };
}
