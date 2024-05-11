#include <cstdio>
#include <lol/error.hpp>
#include <lol/fs.hpp>
#include <lol/hash/dict.hpp>
#include <lol/io/file.hpp>
#include <lol/log.hpp>
#include <lol/patcher/patcher.hpp>
#include <lol/utility/cli.hpp>
#include <lol/utility/zip.hpp>
#include <lol/wad/archive.hpp>
#include <lol/wad/index.hpp>
#include <thread>
#include <unordered_set>

using namespace lol;

static auto FILTER_NONE(wad::Index::Map::const_reference i) noexcept -> bool { return false; }

static auto FILTER_TFT(wad::Index::Map::const_reference i) noexcept -> bool {
    static constexpr std::string_view BLOCKLIST[] = {"map21", "map22"};
    for (auto const& name : BLOCKLIST)
        if (name == i.first) return true;
    return false;
}

static auto is_wad(fs::path const& path) -> bool {
    auto filename = path.filename().generic_string();
    if (filename.ends_with(".wad") || filename.ends_with(".wad.client")) return true;
    if (fs::exists(path / "data")) return true;
    if (fs::exists(path / "data2")) return true;
    if (fs::exists(path / "levels")) return true;
    if (fs::exists(path / "assets")) return true;
    if (fs::exists(path / "assets")) return true;
    if (fs::exists(path / "OBSIDIAN_PACKED_MAPPING.txt")) return true;
    return false;
}

static auto mod_addwad(fs::path src, fs::path dst, fs::path game, bool noTFT, bool removeUNK) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst), lol_trace_var("{}", game));
    lol_throw_if(src.empty());
    lol_throw_if(dst.empty());
    lol_throw_if(!fs::exists(dst / "META" / "info.json"));

    auto mounted = wad::Mounted{src.filename()};
    if (fs::is_directory(src)) {
        mounted.archive = wad::Archive::pack_from_directory(src);
    } else {
        mounted.archive = wad::Archive::read_from_file(src);
    }

    if (!game.empty()) {
        logi("Indexing game wads");
        auto game_index = wad::Index::from_game_folder(game);
        lol_throw_if_msg(game_index.mounts.empty(), "Not a valid Game folder");
        game_index.remove_filter(noTFT ? FILTER_TFT : FILTER_NONE);

        logi("Rebasing");
        auto base = game_index.find_by_mount_name_or_overlap(mounted.name(), mounted.archive);
        lol_throw_if_msg(!base, "Failed to find base wad for: {}", mounted.name());
        if (removeUNK) {
            mounted.remove_unknown(*base);
        }
        mounted.remove_unmodified(*base);
        mounted.relpath = mounted.relpath.parent_path() / base->relpath.filename();
    } else {
        logw("No game folder selected, falling back to manual rename!");
        auto filename = mounted.relpath.filename().generic_string();
        if (!filename.ends_with(".wad.client")) {
            if (filename.ends_with(".wad")) {
                filename.append(".client");
            } else {
                filename.append(".wad.client");
            }
        }
    }

    mounted.archive.write_to_file(dst / "WAD" / mounted.relpath);
}

static auto mod_copy(fs::path src, fs::path dst, fs::path game, bool noTFT) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst), lol_trace_var("{}", game));
    lol_throw_if(src.empty());
    lol_throw_if(dst.empty());
    lol_throw_if(!fs::exists(src / "META" / "info.json"));

    if (src == dst) {
        logi("Creating tmp directory");
        auto tmp = fs::tmp_dir{dst.generic_string() + ".tmp"};
        mod_copy(src, tmp.path, game, noTFT);

        logi("Removing original directory");
        fs::remove_all(src);

        logi("Moving tmp directory");
        tmp.move(src);
        return;
    }

    logi("Indexing mod wads");
    auto mod_index = wad::Index::from_mod_folder(src);

    if (!game.empty()) {
        logi("Indexing game wads");
        auto game_index = wad::Index::from_game_folder(game);
        lol_throw_if_msg(game_index.mounts.empty(), "Not a valid Game folder");
        game_index.remove_filter(noTFT ? FILTER_TFT : FILTER_NONE);

        logi("Rebasing wads");
        mod_index = mod_index.rebase_from_game(game_index);
    }

    logi("Resolving conflicts");
    mod_index.resolve_conflicts(mod_index, false);

    if (fs::exists(dst)) {
        fs::remove_all(dst);
    }

    logi("Copying META files");
    fs::create_directories(dst / "META");
    for (auto const& dirent : fs::directory_iterator(src / "META")) {
        auto relpath = fs::relative(dirent.path(), src);
        fs::copy_file(src / relpath, dst / relpath, fs::copy_options::overwrite_existing);
    }

    logi("Writing wads");
    fs::create_directories(dst);
    mod_index.write_to_directory(dst);
}

static auto mod_export(fs::path src, fs::path dst, fs::path game, bool noTFT) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst), lol_trace_var("{}", game));
    lol_throw_if(src.empty());
    lol_throw_if(dst.empty());
    lol_throw_if(!fs::exists(src / "META" / "info.json"));

    logi("Creating tmp directory");
    auto tmp = fs::tmp_dir{dst.generic_string() + ".tmp"};

    logi("Optimizing before zipping");
    mod_copy(src, tmp.path, game, noTFT);

    logi("Zipping mod");
    utility::zip(tmp.path, dst);
}

static auto mod_import(fs::path src, fs::path dst, fs::path game, bool noTFT) -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst), lol_trace_var("{}", game));
    lol_throw_if(src.empty());
    lol_throw_if(dst.empty());

    logi("Creating tmp directory");
    auto tmp = fs::tmp_dir{dst.generic_string() + ".tmp"};

    if (is_wad(src)) {
        auto info_json = fmt::format(
            R"({{ "Author": "Unknown", "Description": "Imported from wad", "Name": "{}", "Version": "1.0.0" }})",
            wad::Mounted::make_name(src));
        auto info = io::File::create(tmp.path / "META" / "info.json");
        info.write(0, info_json.data(), info_json.size());
        mod_addwad(src, tmp.path, game, noTFT, false);
    } else if (auto filename = src.filename().generic_string();
               filename.ends_with(".zip") || filename.ends_with(".fantome")) {
        logi("Unzipping mod");
        utility::unzip(src, tmp.path);

        logi("Optimizing after unzipping");
        mod_copy(tmp.path, tmp.path, game, noTFT);
    } else if (fs::exists(src / "META" / "info.json")) {
        mod_copy(src, tmp.path, game, noTFT);
    } else {
        lol_throw_msg("Unsuported mod file!");
    }

    if (fs::exists(dst)) {
        logi("Remove existing mod");
        fs::remove_all(dst);
    }

    logi("Moving tmp directory");
    tmp.move(dst);
}

static auto mod_mkoverlay(fs::path src, fs::path dst, fs::path game, fs::names mods, bool noTFT, bool ignoreConflict)
    -> void {
    lol_trace_func(lol_trace_var("{}", src), lol_trace_var("{}", dst), lol_trace_var("{}", game));
    lol_throw_if(src.empty());
    lol_throw_if(dst.empty());
    lol_throw_if(game.empty());

    logi("Indexing game");
    auto game_index = wad::Index::from_game_folder(game);
    lol_throw_if_msg(game_index.mounts.empty(), "Not a valid Game folder");
    game_index.remove_filter(noTFT ? FILTER_TFT : FILTER_NONE);

    auto blocked = std::unordered_set<hash::Xxh64>{};
    for (auto const& [_, mounted] : game_index.mounts) {
        auto subchunk_name = fs::path(mounted.relpath).replace_extension(".SubChunkTOC").generic_string();
        blocked.insert(hash::Xxh64(subchunk_name));
    }
    auto mod_queue = std::vector<wad::Index>{};

    logi("Reading mods");
    for (auto const& mod_name : mods) {
        auto mod_index = wad::Index::from_mod_folder(src / mod_name);
        for (auto& [path_, mounted] : mod_index.mounts) {
            std::erase_if(mounted.archive.entries, [&](auto const& kvp) { return blocked.contains(kvp.first); });
        }
        if (mod_index.mounts.empty()) {
            logw("Empty mod: {}", mod_index.name);
            continue;
        }

        // We have to resolve any conflicts inside mod itself
        mod_index.resolve_conflicts(mod_index, ignoreConflict);

        // We try to resolve conflicts with other mods
        for (auto& old : mod_queue) {
            old.resolve_conflicts(mod_index, ignoreConflict);
        }
        mod_queue.push_back(std::move(mod_index));
    }

    auto overlay_index = wad::Index{};
    logi("Merging mods");
    for (auto& mod_index : mod_queue) {
        overlay_index.add_overlay_mod(game_index, mod_index);
    }

    logi("Writing wads");
    fs::create_directories(dst);
    overlay_index.write_to_directory(dst);

    logi("Cleaning up stray wads");
    overlay_index.cleanup_in_directory(dst);
}

static auto mod_runoverlay(fs::path overlay, fs::path config_file, fs::path game, fs::names opts) -> void {
    lol_trace_func(lol_trace_var("{}", overlay), lol_trace_var("{}", config_file), lol_trace_var("{}", game));
    lol_throw_if(overlay.empty());
    lol_throw_if(config_file.empty());

    auto lock = new std::atomic_bool(false);
    auto thread = std::thread([lock] {
        setbuf(stdin, nullptr);
        for (;;) {
            int c = fgetc(stdin);
            if (c == '\n' && !*lock) {
                fflush(stdout);
                exit(0);
            }
            if (c == -1) {
                exit(0);
            }
        }
    });
    thread.detach();

    fmtlog::setLogFile(stdout, false);
    auto old_msg = patcher::M_DONE;
    try {
        patcher::run(
            [lock, &old_msg](auto msg, char const* arg) {
                if (msg != old_msg) {
                    old_msg = msg;
                    fprintf(stdout, "Status: %s\n", patcher::STATUS_MSG[msg]);
                    fflush(stdout);
                    if (msg == patcher::M_PATCH || msg == patcher::M_NEED_SAVE) {
                        *lock = true;
                        if (arg && *arg) {
                            fprintf(stdout, "Config: %s\n", arg);
                            fflush(stdout);
                        }
                    } else if (*lock) {
                        *lock = false;
                    }
                }
                if (msg == patcher::M_WAIT_EXIT) {
                    if (arg && *arg) {
                        fprintf(stdout, "[DLL] %s\n", arg);
                        fflush(stdout);
                    }
                }
            },
            overlay,
            config_file,
            game,
            opts);
    } catch (patcher::PatcherAborted const&) {
        // nothing to see here, lol
        lol::error::stack().clear();
        return;
    } catch (...) {
        throw;
    }
}

static auto help(fs::path cmd) -> void {
    lol_trace_func(
        "addwad <src> <dst> --game:<path> --noTFT --removeUNK",
        "copy <src> <dst> --game:<path> --noTFT",
        "export <src> <dst> --game:<path> --noTFT",
        "import <src> <dst> --game:<path> --noTFT",
        "mkoverlay <modsdir> <overlay> --game:<path> --mods:<name1>/<name2>/<name3>... --noTFT --ignoreConflict",
        "runoverlay <overlay> <configfile> --game:<path> --opts:<none/configless>...",
        "help");
    lol_throw_msg("Bad command: {}", cmd);
}

int main(int argc, char** argv) {
    utility::set_binary_io();
    fmtlog::setHeaderPattern("[{l}] ");
    fmtlog::setLogFile(stdout, false);
    lol::init_logging_thread();
    try {
        fs::path exe, cmd, src, dst;
        auto flags = utility::argv_parse(utility::argv_fix(argc, argv), exe, cmd, src, dst);
        if (cmd == "addwad") {
            mod_addwad(src, dst, flags["--game:"], flags.contains("--noTFT"), flags.contains("--removeUNK"));
        } else if (cmd == "copy") {
            mod_copy(src, dst, flags["--game:"], flags.contains("--noTFT"));
        } else if (cmd == "export") {
            mod_export(src, dst, flags["--game:"], flags.contains("--noTFT"));
        } else if (cmd == "import") {
            mod_import(src, dst, flags["--game:"], flags.contains("--noTFT"));
        } else if (cmd == "mkoverlay") {
            mod_mkoverlay(src,
                          dst,
                          flags["--game:"],
                          flags["--mods:"],
                          flags.contains("--noTFT"),
                          flags.contains("--ignoreConflict"));
        } else if (cmd == "runoverlay") {
            mod_runoverlay(src, dst, flags["--game:"], flags["--opts:"]);
        } else {
            help(cmd);
        }
        logi("Done!");
    } catch (std::exception const& error) {
        fmtlog::poll(true);
        fflush(stdout);

        fmt::print(stderr, "backtrace: {}\nerror: {}\n", lol::error::stack_trace(), error);
        fflush(stderr);
        return EXIT_FAILURE;
    }

    fmtlog::poll(true);
    fflush(stdout);
    return EXIT_SUCCESS;
}
