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
    try {
        prefix = fs::absolute(prefix);
        print_path("Prefix", prefix);
        for (int result = 1;;) {
            if (result == 1) {
                puts("===============================================================================");
                puts("Scanning for league");
            }
            result = overlay.run([&](LCS::ModOverlay::Message m) -> bool {
                  switch(m) {
                  case LCS::ModOverlay::M_FOUND:
                  puts("Found league");
                  break;
                  case LCS::ModOverlay::M_WAIT_INIT:
                  puts("Wait initialized");
                  break;
                  case LCS::ModOverlay::M_SCAN:
                  puts("Scanning");
                  break;
                  case LCS::ModOverlay::M_NEED_SAVE:
                  puts("Saving");
                  overlay.save(configfile);
                  break;
                  case LCS::ModOverlay::M_WAIT_PATCHABLE:
                  puts("Wait patchable");
                  break;
                  case LCS::ModOverlay::M_PATCH:
                  puts(overlay.to_string().c_str());
                  puts("Patching");
                  break;
                  case LCS::ModOverlay::M_WAIT_EXIT:
                  puts("Waiting for exit");
                  break;
                  }
                  return true;
            }, prefix);
        }
    } catch (std::runtime_error const &error) {
        printf("Error: %s\n"
               "Press enter to continue...",
               error.what());
        getc(stdin);
    }
})
