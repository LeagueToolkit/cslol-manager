#include "modoverlay.hpp"
#include "process.hpp"
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

make_main({
    fs::path prefix = argc > 1 ? fs::path(argv[1]) : fs::path("MOD/");
    fs::path configfile = fs::path(argv[0]).parent_path() / "lolcustomskin.txt";
    LCS::ModOverlay overlay = {};

    overlay.load(configfile);
    printf("Source at https://github.com/moonshadow565/lolcustomskin\n"
           "Config format: %s\n"
           "Config: %s\n",
           LCS::ModOverlay::INFO, overlay.to_string().c_str());
    print_path("Prefix", prefix);
    try {
        for (;;) {
            puts("===============================================================================");
            std::optional<LCS::Process> process = {};
            puts("Looking for league");
            while (!process) {
                LCS::SleepMS(50);
                process = LCS::Process::Find("League of Legends.exe");
            }
            puts("Found league");
            if (!overlay.check(*process)) {
                puts("Wait initialized!");
                process->WaitInitialized();
                puts("Rescanning");
                overlay.scan(*process);
                overlay.save(configfile);
                printf("Config: %s\n", overlay.to_string().c_str());
            } else {
                puts("Wait patchable");
                overlay.wait_patchable(*process);
            }
            puts("Patching");
            overlay.patch(*process, prefix);
            puts("Waiting for exit");
            process->WaitExit();
        }
    } catch (std::runtime_error const &error) {
        printf("Error: %s\n"
               "Press enter to continue...",
               error.what());
        getc(stdin);
    }
})
