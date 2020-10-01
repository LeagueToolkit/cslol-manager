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
        static char const SCHEMA[];
        static char const INFO[];
        uint32_t checksum = {};
        PtrStorage off_rsa_meth = {};
        PtrStorage off_open = {};
        PtrStorage off_open_ref = {};

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
