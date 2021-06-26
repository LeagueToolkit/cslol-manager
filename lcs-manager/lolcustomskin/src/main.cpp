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
    std::filesystem::path configfile = exefile.parent_path() / "lolcustomskin.txt";
    LCS::ModOverlay overlay = {};

    overlay.load(configfile);
    printf("Source at https://github.com/moonshadow565/lolcustomskin\n"
           "Put your moded files into: %s\n"
           "Config format: %s\n"
           "Config: %s\n",
           prefix.c_str(), LCS::ModOverlay::INFO, overlay.to_string().c_str());
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
}
