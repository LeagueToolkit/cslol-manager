#include <stdlib.h>

#include <lol/error.hpp>
#include <lol/utility/cli.hpp>

using namespace lol;

#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
// do not reorder
#    include <processenv.h>
#    include <shellapi.h>
// do not reoder
#    include <fcntl.h>
#    include <io.h>
#endif

auto utility::argv_fix(int argc, char** argv) noexcept -> std::vector<fs::path> {
    lol_trace_func();
    auto result = std::vector<fs::path>(argv, argv + argc);
#ifdef _WIN32
    if (auto wargc = 0; auto wargv = CommandLineToArgvW(GetCommandLineW(), &wargc)) {
        result = std::vector<fs::path>(wargv, wargv + wargc);
    }
    if (result.size() < 1 || result[0].empty()) {
        result[0] = "./out.exe";
    }
    // we can do this on windows...
    if (wchar_t buffer[1 << 15]; auto outsize = GetModuleFileNameW(NULL, buffer, 1 << 15)) {
        auto exepath = std::wstring_view(buffer, outsize);
        if (std::wstring_view root = L"\\\\?\\"; exepath.starts_with(root)) {
            exepath.remove_prefix(root.size());
        }
        result[0] = exepath;
    }
#else
    if (result.size() < 1 || result[0].empty()) {
        result[0] = "./a.out";
    }
#endif
    return result;
}

auto utility::set_binary_io() noexcept -> void {
#ifdef _WIN32
    _setmode(_fileno(stdout), O_BINARY);
    _setmode(_fileno(stdin), O_BINARY);
#endif
}