#ifdef _WIN32
#    include <algorithm>
#    include <array>
#    include <chrono>
#    include <fstream>
#    include <lol/error.hpp>
#    include <lol/patcher/patcher.hpp>
#    include <thread>

#    include "utility/delay.hpp"
// do not reorder
#    define WIN32_LEAN_AND_MEAN
// do not reorder
#    include <windows.h>
#    define last_error() std::error_code((int)GetLastError(), std::system_category())

using namespace lol;
using namespace lol::patcher;
using namespace std::chrono_literals;

static inline constexpr auto LOL_WINDOW = "League of Legends (TM) Client";

struct PidAndTid {
    DWORD pid{};
    DWORD tid{};

    static auto Find(char const* window) -> PidAndTid {
        if (auto hwnd = FindWindowExA(nullptr, nullptr, nullptr, window)) {
            auto pid = DWORD{};
            auto tid = GetWindowThreadProcessId(hwnd, &pid);
            return PidAndTid{pid, tid};
        }
        return {};
    }

    constexpr bool operator==(PidAndTid const& other) const { return pid == other.pid && tid == other.tid; }
    constexpr bool operator!=(PidAndTid const& other) const { return !(*this == other); }
    constexpr explicit operator bool() const { return pid && tid; }
    constexpr bool operator!() const { return !(bool)*this; }
};

struct Context {
    std::u16string prefix;
    std::shared_ptr<void> hmod;
    HOOKPROC lpfn;

    auto set_prefix(fs::path const& profile_path) -> void {
        prefix = fs::absolute(profile_path.lexically_normal()).generic_u16string();
        for (auto& c : prefix) {
            if (c == u'/') {
                c = u'\\';
            }
        }
        if (!prefix.starts_with(u"\\\\")) {
            prefix = u"\\\\?\\" + prefix;
        }
        if (!prefix.ends_with(u"\\")) {
            prefix.push_back(u'\\');
        }
        if ((prefix.size() + 1) * sizeof(char16_t) >= sizeof(char16_t) * 512) {
            lol_throw_msg("Prefix path too big!");
        }

        hmod = std::shared_ptr<void>(LoadLibraryA("cslol-dll.dll"), [](LPVOID p) {
            if (p) FreeLibrary((HMODULE)p);
        });
        lol_throw_if_msg(!hmod, "LoadLibraryA cslol-dll.dll: {}", last_error());

        lpfn = (HOOKPROC)GetProcAddress((HMODULE)hmod.get(), "msg_hookproc");
        lol_throw_if_msg(!lpfn, "GetProcAddress msg_hookproc: {}", last_error());

        auto const prefix_buffer = (char16_t*)GetProcAddress((HMODULE)hmod.get(), "prefix_buffer");
        lol_throw_if_msg(!prefix_buffer, "GetProcAddress prefix: {}", last_error());

        // Sanity check if someone else loaded dll and set prefix before us:
        lol_throw_if_msg(prefix_buffer[0]++ != 0, "Detected another instance running!");

        memcpy(prefix_buffer, prefix.c_str(), (prefix.size() + 1) * sizeof(char16_t));
    }

    auto patch(std::uint32_t tid) -> void {
        auto const hhook = SetWindowsHookExA(WH_GETMESSAGE, lpfn, (HINSTANCE)hmod.get(), tid);
        lol_throw_if_msg(!hhook, "SetWindowsHookExA: {}", last_error());
    }
};

auto patcher::run(std::function<void(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path,
                  fs::names const& opts) -> void {

    auto ctx = Context{};
    ctx.set_prefix(profile_path);
    (void)game_path;
    for (;;) {
        // Wait for window to spawn.
        const auto pid_and_tid = run_until_or(
            30s,
            Intervals{10ms},
            [&] {
                update(M_WAIT_START, "");
                return PidAndTid::Find(LOL_WINDOW);
            },
            []() -> PidAndTid { return {}; });
        if (!pid_and_tid) {
            continue;
        }

        // Signal that process has been found.
        update(M_FOUND, "");

        // Signal that patching is under way.
        update(M_PATCH, "");
        ctx.patch(pid_and_tid.tid);

        // Wait until exit.
        update(M_WAIT_EXIT, "");
        run_until_or(
            3h,
            Intervals{5s, 10s, 15s},
            [&, pid_and_tid] { return PidAndTid::Find(LOL_WINDOW) != pid_and_tid; },
            []() -> bool { throw PatcherTimeout(std::string("Timed out exit")); });

        // Signal done.
        update(M_DONE, "");
    }
}

#endif
