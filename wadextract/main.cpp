#include <string>
#include <filesystem>
#include <iostream>
#include "installer.h"
#include "file.hpp"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
    fspath srcfile;
    HashMap hashmap = {};

    load_hashdb(hashmap, "xxh64.db");

    if(argc > 1) {
        srcfile = argv[1];
    }
    if(srcfile.empty()) {
        puts("Extract .wad to a folder!");
        puts("To add (un)hash definitions drop .hashtable or .txt here and press enter.");
        puts("To extract drop your .wad here and press enter:");
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
            if(srcfile.extension() == ".hashtable" || srcfile.extension() == ".txt") {
                import_hashes(hashmap, srcfile);
                save_hashdb(hashmap, "xxh64.db");
                puts("Done!");
            } else if(srcfile.extension() == ".wad" || srcfile.extension() == ".client") {
                auto dst = srcfile;
                dst.replace_extension();
                if(dst.extension() == ".wad") {
                    dst.replace_extension();
                }
                wad_extract(srcfile, dst, hashmap, [](auto const&, auto done, auto total) {
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
