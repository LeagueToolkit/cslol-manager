#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <mutex>
#include <filesystem>
#include "modoverlay.hpp"

namespace fs = std::filesystem;

struct Main {
    ModOverlay::Config config = {};
    bool once = false;
    bool paused = false;
    std::mutex pauseLock;

    Main(int argc, char** argv) {
        if(argc > 1) {
            std::string overlay = argv[1];
            if (overlay.size() > 0 && overlay.back() != '/' && overlay.back() != '\\') {
                overlay.push_back('/');
            }
            std::copy(overlay.data(), overlay.data() + overlay.size() + 1, config.prefix.data());

            for (int i = 2; i < argc; i++) {
                if (std::string_view{ argv[i] } == "--once") {
                    once = true;
                } else if (std::string_view { argv[i] } == "--pause") {
                    paused = true;
                } else if (std::string_view { argv[i] } == "--config" && (i + 1) < argc) {
                    i++;
                    config.configpath = argv[i];
                } else {
                    fprintf(stdout, "WARNING: Unknown option: %s", argv[i]);
                }
            }
        }
        if (config.configpath.empty()) {
            auto path = fs::path(argv[0]).parent_path() / "lolcustomskin.txt";
            config.configpath = path.generic_string();
        }

        ConsoleEchoOff();

        puts("INFO: Source at https://github.com/moonshadow565/lolskinmod");
        puts("INFO: Put your moded files into <LoL Folder>/Game/MOD");
        puts("INFO: Press enter to PAUSE or UNPAUSE");
        puts("INFO: =============================================================");
        config.load();
        printf("CONFIG: ");
        config.print();
        putchar('\n');
        if (paused) {
            puts("STATUS: PAUSED");
        } else {
            puts("STATUS: UNPAUSED");
        }
        fflush(stdout);

        pauseLock.lock();
        std::thread thread([this] { process_input(); });
        thread.detach();
    }

    [[noreturn]] void process_input() noexcept {
        for(;;) {
            getc(stdin);
            if(pauseLock.try_lock()) {
                paused = !paused;
                if (paused) {
                    puts("STATUS: PAUSED");
                    fflush(stdout);
                } else {
                    puts("STATUS: UNPAUSED");
                    fflush(stdout);
                }
                pauseLock.unlock();
            }
        }
    }

    uint32_t find_league_pid() noexcept {
        pauseLock.unlock();
        uint32_t pid = 0;
        for(;;) {
            if (pauseLock.try_lock()) {
                if (!paused) {
                    pid = Process::Find("League of Legends.exe");
                    if (pid != 0) {
                        break;
                    }
                }
                pauseLock.unlock();
            }
            SleepMiliseconds(1000);
        }
        return pid;
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
            puts("STATUS: PAUSED");
            fflush(stdout);
            paused = true;
        }
    }

   [[noreturn]] void run() noexcept {
        do {
            puts("INFO: =============================================================");
            puts("STATUS: WAIT_LOL_START");
            fflush(stdout);
            uint32_t pid = find_league_pid();
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
