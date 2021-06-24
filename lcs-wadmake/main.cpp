#include <cstdio>
#include <lcs/error.hpp>
#include <lcs/progress.hpp>
#include <lcs/wad.hpp>
#include <lcs/wadmake.hpp>

using namespace LCS;

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            throw std::runtime_error("lolcustomskin-wadmake.exe <folder path> <optional: wad path>");
        }
        fs::path source = argv[1];
        fs::path dest;
        if (argc > 2) {
            dest = argv[2];
        } else {
            std::string source_str = source.generic_string();
            if (source_str.size() && source_str.back() == '/') {
                source_str.pop_back();
            }
            source_str.append(".wad.client");
            dest = source_str;
        }
        fs::create_directories(dest.parent_path());

        std::string source_str = source.generic_string();
        std::string dest_str = dest.generic_string();
        printf("Reading %s\n", source_str.c_str());
        WadMake wadmake(source);
        printf("Packing %s\n", dest_str.c_str());
        Progress progress = {};
        wadmake.write(dest, progress);
        printf("Finished!\n");
    } catch(std::runtime_error const& error) {
        auto message = error_stack_trace_cstr(error.what());
        printf("Error: %s\r\n", message.c_str());
        if (argc < 3) {
            printf("Press enter to exit...!\n");
            getc(stdin);
        }
    }
}
