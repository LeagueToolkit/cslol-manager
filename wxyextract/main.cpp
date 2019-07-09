#include <string>
#include <filesystem>
#include <iostream>
#include "wadlib.h"
#include "file.hpp"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
    fspath srcfile;

    if(argc > 1) {
        srcfile = argv[1];
    }
    srcfile = "OldHealthbar.wxy";

    if(srcfile.empty()) {
        puts("Extract .wad to a folder!");
        puts("To extract drop your .wxy here and press enter:");
        std::string path;
        std::getline(std::cin, path);
        if(path.size() >= 3) {
            if(path[0] == '"') {
                path = path.substr(1, path.size() - 2);
            }
            srcfile = path;
        }
    }

    auto const parent = srcfile.parent_path();
    if(fs::exists(srcfile) && !fs::is_directory(srcfile)) {
        try {
            if(srcfile.extension() == ".wxy") {
                auto dst = srcfile;
                if(dst.extension() == ".wxy") {
                    dst.replace_extension();
                }
                wxy_extract(dst, srcfile, [](auto const&, auto done, auto total) {
                    printf("\r%4.2fMB/%4.2fMB", done / 1024.0 / 1024.0, total / 1024.0 / 1024.0);
                    if(done == total) {
                        printf("\n");
                    }
                });
                puts("Done!");
            }
        } catch(std::exception const& err){
            puts("Error: ");
            puts(err.what());
        }
    }
    getc(stdin);
    return 0;
}
