#include <cstdio>
#include <stdexcept>
#include "modoverlay.hpp"


int main(int argc, char** argv) {
    ModOverlay::Config config = {};
    bool interactive = true;

    if(argc > 1) {
        auto const size = strlen(argv[1]) + 1;
        std::copy(argv[1], argv[1] + size, config.prefix.data());
        interactive = argc > 2 && strcmp(argv[2], "-i") == 0;
    }

    if(interactive) {
        puts("Source at https://github.com/moonshadow565/lolskinmod");
        puts("Put your moded files into <LoL Folder>/Game/MOD");
        puts("=============================================================");
    }

    config.load();
    printf("CONFIG: ");
    config.print();

    try {
        do {
            if(interactive) {
                puts("=============================================================");
            }
            puts("STATUS: Waiting for league to start");
            auto process = Process("League of Legends.exe", 100);
            puts("STATUS: Found league");
            if(config.good(process)) {
                puts("STATUS: Early patching");
                config.patch(process);
            } else {
                puts("STATUS: Updating offsets");
                if(config.rescan(process)) {
                    config.save();
                    printf("CONFIG: ");
                    config.print();
                    puts("STATUS: Late patching");
                    config.patch(process);
                } else {
                    throw std::runtime_error("Failed to fetch offsets!");
                }
            }
            puts("STATUS: Waiting for league to exit");
            process.WaitExit(15000);
        } while(interactive);
    } catch(std::exception const& err) {
        puts("ERROR: ");
        puts(err.what());
        if(interactive) {
            puts("Press enter to exit...");
            getc(stdin);
        }
    }

    puts("STATUS: Exit");
    fflush(stdout);
    return 0;
}
