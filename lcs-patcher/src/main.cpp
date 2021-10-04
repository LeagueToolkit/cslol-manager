#include "lcs/modoverlay.hpp"
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace fs = std::filesystem;

#ifdef WIN32
#define print_path(name, path) wprintf(L"%s: %s\n", L ## name, path.c_str())
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <processenv.h>
#define make_main(body) int main() { auto argc = 0; auto argv = CommandLineToArgvW(GetCommandLineW(), &argc); body }
#else
#include <unistd.h>
#define print_path(name, path) printf("%s%s\n", name, path.c_str())
#define make_main(body) int main(int argc, char** argv) { body }
#endif

make_main({
    fs::path prefix = argc > 1 ? fs::path(argv[1]) : fs::path("MOD/");
    prefix = fs::absolute(prefix.lexically_normal());
    print_path("Path: prefix", prefix);
    fflush(stdout);

    fs::path configfile = argc > 2 ? fs::path(argv[2]) : fs::path(argv[0]).parent_path() / "lolcustomskin.txt";
    configfile = fs::absolute(configfile.lexically_normal());
    print_path("Path: configfile", configfile);
    fflush(stdout);

    LCS::ModOverlay overlay = {};
    bool const is_background = argc > 2;

    overlay.load(configfile);
    printf("Config: %s\n", overlay.to_string().c_str());
    fflush(stdout);
    try {
        auto old_m = LCS::ModOverlay::M_DONE;
        overlay.run([&] (LCS::ModOverlay::Message m) -> bool {
            if (m != old_m) {
                old_m = m;
                printf("Status: %s\n", LCS::ModOverlay::STATUS_MSG[m]);
                fflush(stdout);
            }
            switch (m) {
            case LCS::ModOverlay::M_NEED_SAVE:
                overlay.save(configfile);
                break;
            default:
                break;
            }
            return !is_background || true;
        }, prefix);
    } catch (std::runtime_error const &error) {
        printf("Error: %s\n", error.what());
        fflush(stdout);
        if (!is_background) {
            getc(stdin);
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
})
