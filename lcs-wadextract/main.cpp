#include <cstdio>
#include <lcs/error.hpp>
#include <lcs/progress.hpp>
#include <lcs/wad.hpp>
#include <lcs/hashtable.hpp>


using namespace LCS;

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            throw std::runtime_error("lolcustomskin-wadextract.exe <wad path> <optional: dest folder>");
        }
        fs::path source = argv[1];
        fs::path dest;
        if (argc > 2) {
            dest = argv[2];
        } else {
            dest = source;
            dest.replace_extension();
            dest.replace_extension();
        }
        fs::create_directories(dest);

        HashTable hashtable = {};
        hashtable.add_from_file(fs::path(argv[0]).parent_path() / "hashes.game.txt");
        std::string source_str = source.generic_string();
        std::string dest_str = dest.generic_string();
        printf("Reading %s\n", source_str.c_str());
        Wad wad(source);
        printf("Extracting %s\n", dest_str.c_str());
        Progress progress = {};
        wad.extract(dest, hashtable, progress);
        printf("Finished!\n");
    } catch(std::runtime_error const& error) {
        auto message = error_stack_trace(error.what());
        printf("Error: %s\r\n", message.c_str());
        if (argc < 3) {
            printf("Press enter to exit...!\n");
            getc(stdin);
        }
    }
}
