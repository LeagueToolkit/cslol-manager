#include <string>
#include <filesystem>
#include <iostream>
#include "wadlib.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char**argv) {
    fspath srcfile;
    if(argc > 1) {
        srcfile = argv[1];
    }

    if(srcfile.empty()) {
        puts("Converts folder to .wad.client!");
        puts("Drop your folder here and press enter:");
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
    if(fs::exists(srcfile) && fs::is_directory(srcfile) && fs::exists(parent)) {
        auto const filename = srcfile.filename().generic_string();
        auto const dstfile = parent / (filename + ".wad.client");
        try {
            printf("Making wad from: %s to %s\n",
                   srcfile.generic_string().c_str(), dstfile.generic_string().c_str());
            wad_make(dstfile, srcfile, [] (auto const&, auto done, auto total) {
                printf("\r%4.2fMB/%4.2fMB", done / 1024.0 / 1024.0, total / 1024.0 / 1024.0);
                if(done == total) {
                    printf("\n");
                }
            });
            puts("Done!");
        } catch(std::exception const& err) {
            puts("Error:");
            puts(err.what());
        }
    } else {
        puts("Srcfile doesn't exist!");
    }
    getc(stdin);
    return 0;
}
