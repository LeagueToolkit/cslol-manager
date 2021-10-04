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
        enum Message {
            M_WAIT_START,
            M_FOUND,
            M_WAIT_INIT,
            M_SCAN,
            M_NEED_SAVE,
            M_WAIT_PATCHABLE,
            M_PATCH,
            M_WAIT_EXIT,
            M_DONE,
            M_COUNT_OF,
        };

        static constexpr const char* const STATUS_MSG[LCS::ModOverlay::M_COUNT_OF] = {
            "Waiting for league match to start",
            "Found League",
            "Wait initialized",
            "Scanning",
            "Saving",
            "Wait patchable",
            "Patching",
            "Waiting for exit",
            "League exited",
        };

        ModOverlay();
        ~ModOverlay();
        void save(std::filesystem::path const& filename) const noexcept;
        void load(std::filesystem::path const& filename) noexcept;
        std::string to_string() const noexcept;
        void from_string(std::string const &) noexcept;
        void run(std::function<bool(Message)> update, std::filesystem::path const& profilePath);
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
