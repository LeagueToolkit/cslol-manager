#include <cstdio>
#include <stdexcept>
#include <string_view>
#include "modoverlay.hpp"


int main(int argc, char** argv) {
    ModOverlay::Config config = {};
    bool interactive = true;
    bool repeat = true;

    if(argc > 1) {
        auto const size = strlen(argv[1]) + 1;
        std::copy(argv[1], argv[1] + size, config.prefix.data());
        interactive = false;
        repeat = false;
        for (int i = 2; i < argc; i++) {
            if (std::string_view{ argv[i] } == "-i") {
                interactive = true;
            }
            if (std::string_view{ argv[i] } == "-r") {
                repeat = true;
            }
        }
    }

    if(interactive) {
        puts("Source at https://github.com/moonshadow565/lolskinmod");
        puts("Put your moded files into <LoL Folder>/Game/MOD");
        puts("=============================================================");
        fflush(stdout);
    }

    config.load();
    printf("CONFIG: ");
    config.print();
    fflush(stdout);

    do {
        try {
            if(interactive) {
                puts("=============================================================");
                fflush(stdout);
            }
            puts("STATUS: WAIT_LOL_START");
            fflush(stdout);

            auto process = Process("League of Legends.exe", 100);
            puts("STATUS: FOUND_LOL");
            fflush(stdout);

            if(config.good(process)) {
                config.patch(process);
                puts("STATUS: PATCH_EARLY");
                fflush(stdout);
            } else {
                puts("STATUS: OFFSET_UPDATE");
                if(config.rescan(process)) {
                    config.save();

                    printf("CONFIG: ");
                    config.print();
                    fflush(stdout);

                    config.patch(process);
                    puts("STATUS: PATCH_LATE");
                    fflush(stdout);
                } else {
                    throw std::runtime_error("Failed to fetch offsets!");
                }
            }

            puts("STATUS: WAIT_LOL_EXIT");
            fflush(stdout);
            process.WaitExit(15000);
        } catch(std::exception const& err) {
            printf("ERROR: %s\n", err.what());
            fflush(stdout);

            if(interactive) {
                puts("Press enter to continue...");
                fflush(stdout);
            }

            getc(stdin);
        }
    } while (repeat);

    puts("STATUS: EXIT");
    fflush(stdout);
    return 0;
}
