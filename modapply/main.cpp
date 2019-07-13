#include <filesystem>
#include <cstdio>
#include "wadlib.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    auto progpath = fs::directory_entry(fspath(argv[0]).parent_path()).path();
    fs::current_path(progpath);

    fspath const lol = get_league_path(argc > 1 ? argv[1] : "", progpath / "wadinstall.txt");
    try {
        if(!fs::exists(lol / "League of Legends.exe")) {
            throw std::exception("League of Legends.exe folder has not been set!");
        }
        wad_mods mods = {};
        wad_mods_scan_recursive(mods, fspath(".") / "mods");
        UpdatePrinter progressBar{};
        wad_mods_install(std::move(mods), lol, lol / "MOD", progressBar);
        msg_done();
    } catch(std::exception const& err) {
        msg_error(err);
    }
    return 0;
}
