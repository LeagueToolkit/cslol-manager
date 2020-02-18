#include "modoverlay.hpp"
#include "process.hpp"
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <thread>

int main(int argc, char** argv) {
    std::string prefix = argc > 1 ? argv[1] : "MOD/";
    std::filesystem::path exefile = argv[0];
    std::string configfile = (exefile.parent_path() / "lolcustomskin.txt").generic_string();
    LCS::ModOverlay overlay = {};

    overlay.load(configfile.c_str());
    printf("Source at https://github.com/moonshadow565/lolcustomskin\n"
           "Put your moded files into: %s\n"
           "Config format: %s\n"
           "Config: %s\n",
           prefix.c_str(), LCS::ModOverlay::INFO, overlay.to_string().c_str());
    try {
        for (;;) {
            puts("===============================================================================");
            uint32_t pid = 0;
            puts("Looking for league");
            while (pid == 0) {
                LCS::SleepMS(50);
                pid = LCS::Process::FindPID("League of Legends.exe");
            }
            puts("Found league");
            auto process = LCS::Process(pid);
            if (!process) {
                throw std::runtime_error("Failed to open process!");
            }
            if (!process.WaitInitialized()) {
                throw std::runtime_error("League not initialized in time!");
            }
            if (!overlay.check(process)) {
                puts("Rescanning");
                if (overlay.scan(process)) {
                    overlay.save(configfile.c_str());
                    printf("Config: %s\n", overlay.to_string().c_str());
                } else {
                    throw std::runtime_error("Failed to scan for offsets!");
                }
            }
            puts("Patching");
            overlay.patch(process, prefix);
            puts("Waiting for exit");
            process.WaitExit();
        }
    } catch (std::runtime_error const &error) {
        printf("Error: %s\n"
               "Press enter to continue...",
               error.what());
        getc(stdin);
    }
}
