#include "lcs/modoverlay.hpp"
#include "lcs/process.hpp"
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
#define print_path(name, path) printf("%s%s\n", name, path.c_str())
#define make_main(body) int main(int argc, char** argv) { body }
#endif

#include <unistd.h>
make_main({
    fs::path prefix = argc > 1 ? fs::path(argv[1]) : fs::path("MOD/");
    fs::path configfile = argc > 2 ? fs::path(argv[2]) : fs::path(argv[0]).parent_path() / "lolcustomskin.txt";
    LCS::ModOverlay overlay = {};
    int message_type = argc > 2;
    overlay.load(configfile);
    printf("Source: https://github.com/LoL-Fantome/lolcustomskin-tools\n"
           "Schema: %s\n"
           "Config: %s\n",
           LCS::ModOverlay::INFO, overlay.to_string().c_str());
    fflush(stdout);
    try {
        prefix = fs::absolute(prefix);
        print_path("Prefix: ", prefix);
        fflush(stdout);
        overlay.run([&](LCS::ModOverlay::Message m) -> bool {
            if (m) {
                puts(LCS::ModOverlay::STATUS_MSG[message_type][m]);
                fflush(stdout);
            }
            if (m == LCS::ModOverlay::M_NEED_SAVE) {
                overlay.save(configfile);
            }
            return LCS::Process::ThisProcessHasParent();
        }, prefix);
    } catch (std::runtime_error const &error) {
        printf("Error: %s\n", error.what());
        fflush(stdout);
        if (!message_type) {
            return EXIT_FAILURE;
        }
        getc(stdin);
    }
    return EXIT_SUCCESS;
})
