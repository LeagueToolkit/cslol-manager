#include <string>
#include <filesystem>
#include "wadlib.h"
#include "file.hpp"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
    auto progpath = fs::directory_entry(fspath(argv[0]).parent_path()).path();
    fs::current_path(progpath);

    fspath srcfile = argc > 1 ? argv[1] : "";

    if(srcfile.empty()) {
        srcfile = open_file_browse(browse_filter_wad_or_hashtable, "File.wad.client",
                                   "Open .wad for extraction!");
    }

    HashMap hashmap = {};
    load_hashdb(hashmap, progpath /  "xxh64.db");

    auto const parent = srcfile.parent_path();
    try {
        if(!fs::exists(srcfile) || fs::is_directory(srcfile)) {
            throw std::exception("No valid file selected!");
        }
        if(srcfile.extension() == ".hashtable" || srcfile.extension() == ".txt") {
            puts("Importing hashes(this might take a while)...");
            import_hashes(hashmap, srcfile);
            save_hashdb(hashmap, "xxh64.db");
            msg_done();
        } else if(srcfile.extension() == ".wad" || srcfile.extension() == ".client") {
            auto defdst = srcfile;
            defdst.replace_extension();
            if(defdst.extension() == ".wad") {
                defdst.replace_extension();
            }
            auto dst = open_dir_browse(defdst, "Destination folder!");
            puts("Extracting to:");
            puts(dst.generic_string().c_str());
            UpdatePrinter progressBar{};
            wad_extract(srcfile, dst, hashmap, progressBar);
            msg_done();
        }
    } catch(std::exception const& err){
        msg_error(err);
    }

    return 0;
}
