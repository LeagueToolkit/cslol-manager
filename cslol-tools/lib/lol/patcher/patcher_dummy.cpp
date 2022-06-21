#if !defined(_WIN32) && !defined(__APPLE__)
#    include <chrono>
#    include <thread>
// do not reorder
#    include <lol/error.hpp>
#    include <lol/patcher/patcher.hpp>

using namespace lol;
using namespace lol::patcher;
using namespace std::chrono_literals;

auto patcher::run(std::function<bool(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path) -> void {
    (void)profile_path;
    (void)config_path;
    (void)game_path;
    for (;;) {
        if (!update(M_WAIT_START, "")) return;
        std::this_thread::sleep_for(250ms);
    }
}

#endif
