#include <cstdio>
#include <lcs/wxyextract.hpp>

using namespace LCS;

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            throw std::runtime_error("lolcustomskin-wxyextract.exe <wxy path> <optional: dest folder>");
        }
        fs::path source = argv[1];
//        fs::path source = "v7 - Koi Soraka (By Sislex).wxy";
//        fs::path source = "v5 - Beach_Rift_Day_-_Textures_pack_v6.16_By_Chewy.wxy";
//        fs::path source = "v3 - Old_Healthbar_Season_2_v1_By_Socr4te.wxy";
        fs::path dest;
        if (argc > 2) {
            dest = argv[2];
        } else {
            dest = source;
            dest.replace_extension();
        }
        fs::create_directories(dest);

        std::string source_str = source.generic_string();
        std::string dest_str = dest.generic_string();
        printf("Reading %s\n", source_str.c_str());
        WxyExtract wxy(source);
        printf("Extracting %s\n", dest_str.c_str());
        Progress progress = {};
        wxy.extract_files(dest / "RAW", progress);
        wxy.extract_meta(dest / "META", progress);
        printf("Finished!\n");
    } catch(std::runtime_error const& error) {
        printf("Error: %s\r\n", error.what());
        if (argc < 3) {
            printf("Press enter to exit...!\n");
            getc(stdin);
        }
    }
}
