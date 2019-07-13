#include <string>
#include <filesystem>
#include <iostream>
#include "wadlib.h"
#include "file.hpp"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
    auto progpath = fs::directory_entry(fspath(argv[0]).parent_path()).path();
    fs::current_path(progpath);

    fspath srcfile = argc > 1 ? argv[1] : "";

    if(srcfile.empty()) {
        srcfile = open_file_browse(browse_filter_wxy, "File.wxy", "Source .wxy file to extract!");
    }

    try {
        auto const parent = srcfile.parent_path();
        if(!fs::exists(srcfile) || fs::is_directory(srcfile)) {
            throw std::exception("No valid file selected!");
        }
        if(srcfile.extension() == ".wxy") {
            auto defdst = srcfile;
            if(defdst.extension() == ".wxy") {
                defdst.replace_extension();
            }
            auto dst = open_dir_browse(defdst, "Destination folder where to extract!");
            puts("Extracting to:");
            puts(dst.generic_string().c_str());
            UpdatePrinter progressBar{};
            wxy_extract(dst, srcfile, progressBar);
            msg_done();
        }
    } catch(std::exception const& err){
        msg_error(err);
    }

    return 0;
}
