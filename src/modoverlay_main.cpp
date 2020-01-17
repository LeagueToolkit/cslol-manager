#include <cstdio>
#include <stdexcept>
#include <string_view>
#include "modoverlay.hpp"

using namespace LCS;
struct Main {
    ModOverlay::Config config = {};
    bool once = false;

    Main(int argc, char** argv) noexcept {
        if(argc > 1) {
            std::string overlay = argv[1];
            if (overlay.size() > 0 && overlay.back() != '/' && overlay.back() != '\\') {
                overlay.push_back('/');
            }
            std::copy(overlay.data(), overlay.data() + overlay.size() + 1, config.prefix.data());

            for (int i = 2; i < argc; i++) {
                std::string_view arg = argv[i];
                if (arg == "--once") {
                    once = true;
                } else if (arg == "--config" && (i + 1) < argc) {
                    i++;
                    config.configpath = argv[i];
                } else if (arg == "-r" || arg == "-i") {
                    // old options, those are now default anyways
                } else {
                    fprintf(stdout, "WARNING: Unknown option: %s", argv[i]);
                }
            }
        }

        if (config.configpath.empty()) {
            std::string path = argv[0];
            auto separator = path.find_last_of('/');
            if(separator == std::string::npos) {
                separator = path.find_last_of('\\');
            }
            if(separator == std::string::npos) {
                separator = path.size();
            }
            path.erase(separator, path.size() - separator);
            path.append("/lolcustomskin.txt");
            config.configpath = path;
        }

        puts("INFO: Source at https://github.com/moonshadow565/lolcustomskin");
        printf("INFO: Put your moded files into: %s\n", config.prefix.data());
        puts("INFO: =============================================================");
        config.load();
        printf("CONFIG: ");
        config.print();
        putchar('\n');
        fflush(stdout);
    }

    void patch_league_pid(uint32_t pid) noexcept {
        try {
            auto process = Process(pid);
            if(config.good(process)) {
                puts("STATUS: PATCH_EARLY");
                fflush(stdout);
                config.patch(process);
            } else {
                puts("STATUS: OFFSET_UPDATE");
                fflush(stdout);
                process.WaitWindow("League of Legends (TM) Client", 50);
                if (config.rescan(process)) {
                    config.save();

                    printf("CONFIG: ");
                    config.print();
                    putchar('\n');
                    puts("STATUS: PATCH_LATE");
                    fflush(stdout);

                    config.patch(process);
                } else {
                    throw std::runtime_error("Failed to fetch offsets!");
                }
            }
            puts("STATUS: WAIT_LOL_EXIT");
            fflush(stdout);
            process.WaitExit();
        } catch(std::exception const& err) {
            printf("ERROR: %s\n", err.what());
            puts("INFO: Press enter to continue...");
            fflush(stdout);
            getc(stdin);
        }
    }

   [[noreturn]] void run() noexcept {
        do {
            puts("INFO: =============================================================");
            puts("STATUS: WAIT_LOL_START");
            fflush(stdout);
            uint32_t pid = 0;
            while(pid == 0) {
                SleepMiliseconds(50);
                pid = Process::Find("League of Legends.exe");
            }
            puts("STATUS: FOUND_LOL");
            fflush(stdout);
            patch_league_pid(pid);
        } while(!once);
        puts("STATUS: EXIT");
        fflush(stdout);
        exit(0);
    }
};

int main(int argc, char** argv) {
    Main main(argc, argv);
    main.run();
}
