#include <filesystem>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include "wadlib.h"

namespace fs = std::filesystem;
using namespace std;

int main(int argc, char** argv) {
    auto progpath = fs::directory_entry(fspath(argv[0]).parent_path()).path();
    fs::current_path(progpath);

    fspath argpath = argc > 1 ? argv[1] : "";
    bool const isWadPath = argpath.extension() == ".wad" || argpath.extension() == ".client";
    fspath const lol = get_league_path(!isWadPath ? argpath : "", progpath / "wadinstall.txt");

    srand(static_cast<unsigned int>(::time(nullptr)));

    try {
        if(!fs::exists(lol / "League of Legends.exe")) {
            throw std::exception("League of Legends.exe folder has not been set!");
        }

        if(!isWadPath) {
            argpath = open_file_browse(browse_filter_wad, "Wad.wad.client", "Add .wad mod!");
        }

        if(!fs::exists(argpath) || fs::is_directory(argpath)) {
            throw std::exception("No valid file selected!");
        }
        wad_index index{lol / "DATA" };
        auto best = index.find_best(argpath);
        if(best.empty()) {
            throw std::exception("Couldn't find a match!");
        }
        auto dstpath = "DATA" / fspath(best);
        auto const defname = dstpath.stem().generic_string() + std::to_string(rand());
        auto const modname = get_input_string_dialog(defname, "Enter the mod name: ");
        dstpath = "mods" / (modname / dstpath);
        std::error_code errorc;
        std::filesystem::create_directories(dstpath.parent_path(), errorc);
        if(errorc) {
            throw File::Error(errorc.message().c_str());
        }
        puts("Copying!");
        std::filesystem::copy_file(argpath, dstpath,
                                   std::filesystem::copy_options::overwrite_existing, errorc);
        if(errorc) {
            throw File::Error(errorc.message().c_str());
        }
        msg_done();
    } catch(std::exception const& err) {
        msg_error(err);
    }
    return 0;
}
