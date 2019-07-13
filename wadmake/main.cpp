#include <string>
#include <filesystem>
#include "wadlib.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
    auto progpath = fs::directory_entry(fspath(argv[0]).parent_path()).path();
    fs::current_path(progpath);

    fspath srcfile = argc > 1 ? argv[1] : "";

    if(srcfile.empty()) {
        srcfile = open_dir_browse("", "Source folder from which to make a .wad!");
    }

    try {
        auto const parent = srcfile.parent_path();
        if(!fs::exists(srcfile) || !fs::is_directory(srcfile) || !fs::exists(parent)) {
            throw std::exception("No valid file selected!");
        }
        auto const defdst = parent / (srcfile.filename().generic_string() + ".wad.client");
        auto dstfile = save_file_browse(browse_filter_wad, defdst, "Save .wad file destion!");
        puts("Saving to:");
        puts(dstfile.generic_string().c_str());
        UpdatePrinter progressBar{};
        wad_make(dstfile, srcfile, progressBar);
        msg_done();
    } catch(std::exception const& err) {
        msg_error(err);
    }

    return 0;
}
