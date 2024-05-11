#ifdef _WIN32
#    include <lol/error.hpp>
#    include <lol/patcher/patcher.hpp>

// do not reorder
#    include "cslol-api.h"
#    include "utility/delay.hpp"

using namespace lol;
using namespace lol::patcher;
using namespace std::chrono_literals;

auto patcher::run(std::function<void(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path,
                  fs::names const& opts) -> void {
    bool debugpatcher = 0;
    for (auto const& o : opts) {
        if (o == "debugpatcher") {
            debugpatcher = true;
        }
    }

    // Initialize first proces.
    lol_throw_if(cslol_init());
    lol_throw_if(cslol_set_flags(CSLOL_HOOK_DISALBE_NONE));
    lol_throw_if(cslol_set_log_level(debugpatcher ? CSLOL_LOG_DEBUG : CSLOL_LOG_INFO));
    lol_throw_if(cslol_set_config(profile_path.generic_u16string().c_str()));

    for (;;) {
        auto tid = run_until_or(
            30s,
            Intervals{10ms},
            [&] {
                update(M_WAIT_START, "");
                return cslol_find();
            },
            []() -> std::uint32_t { return 0; });
        if (!tid) {
            continue;
        }

        // Signal that process has been found.
        update(M_FOUND, "");

        // Hook in 5 seconds or error.
        lol_throw_if(cslol_hook(tid, 300000, 100));

        // Wait for exit.
        update(M_WAIT_EXIT, "");
        run_until_or(
            3h,
            Intervals{5s, 10s, 15s},
            [&, tid] {
                char const* msg = NULL;
                while ((msg = cslol_log_pull())) {
                    // post log message
                    update(M_WAIT_EXIT, msg);
                }
                return tid != cslol_find();
            },
            []() -> bool { throw PatcherTimeout(std::string("Timed out exit")); });
    }
}
#endif
