#pragma once

#include <functional>
#include <lol/common.hpp>
#include <lol/fs.hpp>

namespace lol::patcher {
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

    static constexpr const char* const STATUS_MSG[Message::M_COUNT_OF] = {
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

    extern auto run(std::function<void(Message, char const*)> update,
                    fs::path const& profile_path,
                    fs::path const& config_path,
                    fs::path const& game_path,
                    fs::names const& opts) -> void;

    struct PatcherTimeout : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct PatcherAborted : std::runtime_error {
        PatcherAborted() : std::runtime_error("Aborted as expected") {}
    };
}
