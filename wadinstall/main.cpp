#include <iostream>
#include <string>
#include <filesystem>
#include "installer.h"
#include "utils.h"

using namespace std;

fspath find_league_folder(char* arg) {
    if(arg) {
        auto result = find_league(arg);
        if(!result.empty()) {
            write_line("wadinstall.txt", result);
            return result;
        }
    }
    return read_line("wadinstall.txt");
}

int main(int argc, char** argv) {
    std::filesystem::current_path(fspath(argv[0]).parent_path());
    fspath lol = find_league_folder(argc > 1 ? argv[1] : nullptr);

    while(lol.empty() || !std::filesystem::exists(lol / "League of Legends.exe")) {
        puts("Installs .wad's into league!");
        puts("Drop your LoL.exe here and press enter:");
        std::string path;
        std::getline(std::cin, path);
        lol = find_league(path);
        if(!lol.empty() && std::filesystem::exists(lol / "League of Legends.exe")) {
            write_line("wadinstall.txt", lol.generic_string());
        }
    }

    try {
        wad_mods mods = {};
        wad_mods_scan_recursive(mods, fspath(".") / "mods");
        std::string old = "";
        wad_mods_install(std::move(mods), lol, lol / "MOD",
                         [&old](auto const &n, auto done, auto total){
            if(old != n) {
                old = n;
                printf("%s\n", n.c_str());
            }
            if(done == 0 && total == 0) {
                puts("Skiping!");
            } else {
                printf("\r%4.2fMB/%4.2fMB", done / 1024.0 / 1024.0, total / 1024.0 / 1024.0);
                if(done == total) {
                    printf("\n");
                }
            }
        });
        puts("Done!\n");
    } catch(std::exception const& err) {
        puts("Error!");
        puts(err.what());
    }
    getc(stdin);
    return 0;
}
