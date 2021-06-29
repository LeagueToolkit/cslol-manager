#include <cstdio>
#include <lcs/error.hpp>
#include <lcs/progress.hpp>
#include <lcs/wxyextract.hpp>

using namespace LCS;

#ifdef WIN32
#define print_path(name, path) wprintf(L"%s: %s\n", L ## name, path.c_str())
int wmain(int argc, wchar_t **argv, wchar_t **envp) {
#else
#define print_path(name, path) printf("%s: %s\n", name, path.c_str())
int main(int argc, char** argv) {
#endif
    try {
        if (argc < 2) {
            throw std::runtime_error("lolcustomskin-wxyextract.exe <wxy path> <optional: dest folder>");
        }
        fs::path source = argv[1];
        fs::path dest;
        if (argc > 2) {
            dest = argv[2];
        } else {
            dest = source;
            dest.replace_extension();
        }
        fs::create_directories(dest);

        print_path("Reading", source);
        WxyExtract wxy(source);
        print_path("Extracting", dest);
        Progress progress = {};
        wxy.extract_files(dest / "RAW", progress);
        wxy.extract_meta(dest / "META", progress);
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
