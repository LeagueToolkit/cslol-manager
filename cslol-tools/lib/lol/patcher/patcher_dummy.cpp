#if !defined(_WIN32) && !defined(__APPLE__)
#    include <chrono>
#    include <thread>
// do not reorder
#    include <lol/error.hpp>
#    include <lol/patcher/patcher.hpp>

using namespace lol;
using namespace lol::patcher;
using namespace std::chrono_literals;

auto patcher::run(std::function<void(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path,
                  fs::names const& opts) -> void {
    (void)profile_path;
    (void)config_path;
    (void)game_path;
    (void)opts;
    for (;;) {
        update(M_WAIT_START, "");
        sleep_ms(250);
    }
}

#endif
