#include <cstdio>
#include <lol/error.hpp>
#include <lol/fs.hpp>
#include <lol/hash/dict.hpp>
#include <lol/log.hpp>
#include <lol/utility/cli.hpp>
#include <lol/wad/index.hpp>

using namespace lol;

static auto wad_extract(fs::path src, fs::path dst, fs::path hashdict) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst), lol_trace_var("{}", hashdict));
    lol_throw_if(src.empty());

    if (dst.empty()) {
        dst = src;
    }

    logi("Process dictionary");
    hash::Dict dict = {};
    if (hashdict.empty() || !dict.load(hashdict)) {
        dict.load(fs::current_path() / "hashes.game.txt");
    }

    logi("Searching WAD files in: {}", src);
    std::vector<fs::path> wad_files;
    for (auto entry: fs::recursive_directory_iterator(src)) {
        if (entry.is_directory())
            continue;
        auto extension = entry.path().filename().generic_u8string();
        if (extension.find(u8".wad") == std::string::npos)
            continue;
        wad_files.push_back(entry.path());
    }

    logi("Found {} WAD files", wad_files.size());
    for (auto const& srcpath : wad_files) {
        logi("Reading: {}", srcpath);

        auto relpath = fs::relative(srcpath, src);
        auto outpath = dst / relpath;
        outpath.replace_extension();

        auto archive = wad::Archive::read_from_file(srcpath);

        logi("Extracting to: {}", outpath);
        fs::create_directories(outpath);
        for (auto const& entry : archive.entries) {
            auto const name = entry.first;
            auto const& data = entry.second;
            logi("Writing: {:#016x}", name);
            data.write_to_dir(name, outpath, &dict);
        }
    }
}

int main(int argc, char** argv) {
    utility::set_binary_io();
    fmtlog::setHeaderPattern("[{l}] ");
    fmtlog::setLogFile(stdout, false);
    lol::init_logging_thread();
    try {
        fs::path exe, src, dst, hashdict;
        auto flags = utility::argv_parse(utility::argv_fix(argc, argv), exe, src, dst, hashdict);
        wad_extract(src, dst, !hashdict.empty() ? hashdict : exe.parent_path() / "hashes.game.txt");
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
