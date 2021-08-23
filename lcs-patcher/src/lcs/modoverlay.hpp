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
            M_NONE,
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

        static constexpr const char* const STATUS_MSG[2][LCS::ModOverlay::M_COUNT_OF] = {
            // User friendly status messages
            {
                "",
                "Found League",
                "Wait initialized",
                "Scanning",
                "Saving",
                "Wait patchable",
                "Patching",
                "Waiting for exit",
                "Waiting for league match to start",
            },
            // Machine friendly messages
            {
                "Status: M_NONE",
                "Status: FOUND",
                "Status: WAIT_INIT",
                "Status: SCAN",
                "Status: NEED_SAVE",
                "Status: WAIT_PATCHABLE",
                "Status: PATCH",
                "Status: WAIT_EXIT",
                "Status: DONE",
            }
        };

        ModOverlay();
        ~ModOverlay();
        void save(std::filesystem::path const& filename) const noexcept;
        void load(std::filesystem::path const& filename) noexcept;
        std::string to_string() const noexcept;
        void from_string(std::string const &) noexcept;
        void run(std::function<bool(Message)> update, std::filesystem::path const& profilePath);
    private:
        struct Config;
        std::unique_ptr<Config> config_;
    };
}
