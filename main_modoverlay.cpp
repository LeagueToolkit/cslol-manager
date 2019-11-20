#include <cstdio>
#include <stdexcept>
#include "def_modoverlay.hpp"


int main() {
    puts("Source: https://github.com/moonshadow565/lolskinmod");
    puts("Put your moded files into <LoL Folder>/Game/MOD");
    puts("=============================================================");
    ModOverlay::Config config = {};
    config.load();
    config.print();

    try {
        for(;;) {
            puts("=============================================================");
            puts("Waiting for league to start...");
            auto process = Process("League of Legends.exe", 5);
            if(config.good(process)) {
                puts("Early patching...");
                config.patch(process);
            } else {
                puts("Updating offsets...");
                if(config.rescan(process)) {
                    config.save();
                    config.print();
                    puts("Late patching...");
                    config.patch(process);
                } else {
                    throw std::runtime_error("Failed to fetch offsets!");
                }
            }
            puts("Wating for league to exit...");
            process.WaitExit(5);
        }
    } catch(std::exception const& err) {
        puts("Error: ");
        puts(err.what());
        puts("Press enter to exit...");
        getc(stdin);
    }
}
