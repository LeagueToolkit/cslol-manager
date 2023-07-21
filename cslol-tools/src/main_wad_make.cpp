#include <cstdio>
#include <lol/error.hpp>
#include <lol/fs.hpp>
#include <lol/log.hpp>
#include <lol/utility/cli.hpp>
#include <lol/wad/archive.hpp>

using namespace lol;

static auto wad_make(fs::path src, fs::path dst) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst));
    lol_throw_if(src.empty());

    if (dst.empty()) {
        if (auto filename = src.generic_string(); filename.ends_with(".wad")) {
            dst = filename + ".client";
        } else {
            dst = filename + ".wad.client";
        }
    }

    logi("Packing: {}", src);
    auto archive = wad::Archive{};
    for (auto const& entry : fs::recursive_directory_iterator(src)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        logi("Reading: {}", path);
        archive.add_from_directory(path, src);
    }

    logi("Writing: {}", dst);
    archive.write_to_file(dst);
}

int main(int argc, char** argv) {
    utility::set_binary_io();
    fmtlog::setHeaderPattern("[{l}] ");
    fmtlog::setLogFile(stdout, false);
    lol::init_logging_thread();
    try {
        fs::path exe, src, dst;
        auto flags = utility::argv_parse(utility::argv_fix(argc, argv), exe, src, dst);
        wad_make(src, dst);
        logi("Done!");
    } catch (std::exception const& error) {
        fmtlog::poll(true);
        fflush(stdout);

        fmt::print(stderr, "backtrace: {}\nerror: {}\n", lol::error::stack_trace(), error);
        fflush(stderr);

        if (argc <= 2) {  // Backwards compatabile as interactive version
            fmt::print(stderr, "Press enter to exit...!\n");
            getc(stdin);
        }
        return EXIT_FAILURE;
    }
    fmtlog::poll(true);
    fflush(stdout);
    return EXIT_SUCCESS;
}
