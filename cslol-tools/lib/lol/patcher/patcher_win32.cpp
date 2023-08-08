#ifdef _WIN32
#    include <algorithm>
#    include <array>
#    include <chrono>
#    include <fstream>
#    include <lol/error.hpp>
#    include <lol/patcher/patcher.hpp>
#    include <lol/patcher/utility/peex.hpp>
#    include <thread>

// do not reorder
#    include "utility/delay.hpp"
#    include "utility/lineconfig.hpp"
#    include "utility/ppp.hpp"
#    include "utility/process.hpp"

using namespace lol;
using namespace lol::patcher;
using namespace std::chrono_literals;

// clang-format off
constexpr inline char PAT_REVISION[] = "patcher-win64-v6";
constexpr auto const find_ptr_CreateFileA =
    &ppp::any<
        "C7 44 24 20 03 00 00 00 "
        "45 8D 41 01 "
        "FF 15 r[?? ?? ?? ??]"_pattern,
        "44 8B C3 "
        "41 8B D6 "
        "89 7C 24 ?? "
        "49 8B CC "
        "FF 15 r[?? ?? ?? ??]"_pattern>;

constexpr auto const find_ptr_CRYPTO_free =
    &ppp::any<
        "80 00 00 00 "
        "01 00 00 00 "
        "FF FF FF FF "
        "01 00 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00 "
        "o[??] ?? ?? ?? ?? ?? 00 00"_pattern>;
// clang-format on

struct ImportTrampoline {
    uint8_t data[16] = {};

    static ImportTrampoline make(Ptr<uint8_t> where) {
        ImportTrampoline result = {{0x48, 0xB8u, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0}};
        memcpy(result.data + 2, &where, sizeof(where));
        return result;
    }

    static ImportTrampoline make_ind(Ptr<uint8_t> where) {
        ImportTrampoline result = {{0x48, 0xB8u, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x20}};
        memcpy(result.data + 2, &where, sizeof(where));
        return result;
    }
};

struct CodePayload {
    // clang-format off
    // Hooking CRYPTO_free impl ptr to redirect control flow in int_rsa_verify
    uint8_t hook_CRYPTO_free[0x40] = {
        0x48, 0x85, 0xc9, 0x74, 0x22, 0x53, 0x48, 0x89,
        0xcb, 0x48, 0x83, 0xec, 0x20, 0xff, 0x15, 0x2d,
        0x00, 0x00, 0x00, 0x48, 0x89, 0xc1, 0x48, 0x31,
        0xd2, 0x49, 0x89, 0xd8, 0xff, 0x15, 0x26, 0x00,
        0x00, 0x00, 0x48, 0x83, 0xc4, 0x20, 0x5b, 0x48,
        0x8b, 0x0c, 0x24, 0x48, 0x8b, 0x09, 0x48, 0x8b,
        0x15, 0x1b, 0x00, 0x00, 0x00, 0x48, 0x39, 0xd1,
        0x74, 0x1e, 0xc3, 0x90, 0x90, 0x90, 0x90, 0x90,
    };
    Ptr<uint8_t> ptr_GetProcessHeap = {};
    Ptr<uint8_t> ptr_HeapFree = {};
    uint8_t ret_match[8] = {0x48, 0x8B, 0x7C, 0x24, 0x70, 0x8B, 0xC3, 0x48};
    uint8_t hook_ret[8] = {0xbb, 0x01, 0x00, 0x00, 0x00, 0xc3};

    // Trampoline back to real CreateFileA
    ImportTrampoline org_CreateFileA = {};

    // Hooking CreateFileA import trampoline
    uint8_t hook_CreateFileA[0x100] = {
        0xc8, 0x00, 0x10, 0x00, 0x53, 0x57, 0x56, 0x48,
        0x83, 0xec, 0x48, 0x4c, 0x89, 0x4d, 0x28, 0x4c,
        0x89, 0x45, 0x20, 0x48, 0x89, 0x55, 0x18, 0x48,
        0x89, 0x4d, 0x10, 0x81, 0x7d, 0x18, 0x00, 0x00,
        0x00, 0x80, 0x0f, 0x85, 0x9c, 0x00, 0x00, 0x00,
        0x83, 0x7d, 0x20, 0x01, 0x0f, 0x85, 0x92, 0x00,
        0x00, 0x00, 0x83, 0x7d, 0x30, 0x03, 0x0f, 0x85,
        0x88, 0x00, 0x00, 0x00, 0x81, 0x7d, 0x38, 0x80,
        0x00, 0x00, 0x00, 0x75, 0x7f, 0x80, 0x39, 0x44,
        0x75, 0x7a, 0x80, 0x79, 0x01, 0x41, 0x75, 0x74,
        0x80, 0x79, 0x02, 0x54, 0x75, 0x6e, 0x80, 0x79,
        0x03, 0x41, 0x75, 0x68, 0x48, 0x8d, 0xbd, 0x00,
        0xf0, 0xff, 0xff, 0x48, 0x8d, 0x35, 0xa6, 0x00,
        0x00, 0x00, 0x66, 0xad, 0x66, 0xab, 0x66, 0x85,
        0xc0, 0x75, 0xf7, 0x48, 0x83, 0xef, 0x02, 0x48,
        0x8b, 0x75, 0x10, 0xb4, 0x00, 0xac, 0x3c, 0x2f,
        0x75, 0x02, 0xb0, 0x5c, 0x66, 0xab, 0x84, 0xc0,
        0x75, 0xf3, 0x48, 0x8b, 0x45, 0x48, 0x48, 0x89,
        0x44, 0x24, 0x30, 0x48, 0x8b, 0x45, 0x38, 0x48,
        0x89, 0x44, 0x24, 0x28, 0x48, 0x8b, 0x45, 0x30,
        0x48, 0x89, 0x44, 0x24, 0x20, 0x4c, 0x8b, 0x4d,
        0x28, 0x4c, 0x8b, 0x45, 0x20, 0x48, 0x8b, 0x55,
        0x18, 0x48, 0x8d, 0x8d, 0x00, 0xf0, 0xff, 0xff,
        0xff, 0x15, 0x4a, 0x00, 0x00, 0x00, 0x48, 0x83,
        0xf8, 0xff, 0x75, 0x31, 0x48, 0x8b, 0x45, 0x48,
        0x48, 0x89, 0x44, 0x24, 0x30, 0x48, 0x8b, 0x45,
        0x38, 0x48, 0x89, 0x44, 0x24, 0x28, 0x48, 0x8b,
        0x45, 0x30, 0x48, 0x89, 0x44, 0x24, 0x20, 0x4c,
        0x8b, 0x4d, 0x28, 0x4c, 0x8b, 0x45, 0x20, 0x48,
        0x8b, 0x55, 0x18, 0x48, 0x8b, 0x4d, 0x10, 0xff,
        0x15, 0x0b, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc4,
        0x48, 0x5e, 0x5f, 0x5b, 0xc9, 0xc3, 0x90, 0x90,
    };
    Ptr<uint8_t> ptr_CreateFileA = {};
    Ptr<uint8_t> ptr_CreateFileW = {};
    char16_t prefix_open_data[0x400] = {};
    // clang-format on
};

extern "C" {
extern void* GetModuleHandleA(char const* name);
extern void* GetProcAddress(void* module, char const* name);
extern void* FindWindowExA(void* parent, void* after, char const* klass, char const* name);
}

struct Kernel32 {
    Ptr<ImportTrampoline> ptr_CreateFileA;
    Ptr<uint8_t> ptr_CreateFileA_iat;
    Ptr<uint8_t> ptr_CreateFileW;
    Ptr<uint8_t> ptr_GetProcessHeap;
    Ptr<uint8_t> ptr_HeapFree;

    static auto load() -> Kernel32 {
        // load kernel32.dll, we rely on game loading this same dll on same address
        static auto kernel32 = GetModuleHandleA("KERNEL32.dll");

        // get CreateFileA function address
        static auto kernel32_ptr_CreateFileA = GetProcAddress(kernel32, "CreateFileA");
        lol_throw_if_msg(!kernel32_ptr_CreateFileA, "Failed to find kernel32 ptr CreateFileA");

        // parse stub instead of IAT cuz easier
        static auto kernel32_ptr_CreateFileA_iat = [] {
            std::uint16_t insn;
            std::int32_t disp;
            memcpy(&insn, (char const*)kernel32_ptr_CreateFileA, sizeof(insn));
            if (insn != 0x25FF) {
                return (void*)nullptr;
            }
            memcpy(&disp, (char const*)kernel32_ptr_CreateFileA + sizeof(insn), sizeof(disp));
            return (void*)(((char const*)kernel32_ptr_CreateFileA + sizeof(insn) + sizeof(disp)) + disp);
        }();

        static auto kernel32_ptr_CreateFileW = GetProcAddress(kernel32, "CreateFileW");
        lol_throw_if_msg(!kernel32_ptr_CreateFileW, "Failed to find kernel32 ptr CreateFileW");

        static auto kernel32_ptr_GetProcessHeap = GetProcAddress(kernel32, "GetProcessHeap");
        lol_throw_if_msg(!kernel32_ptr_GetProcessHeap, "Failed to find kernel32 ptr GetProcessHeap");

        static auto kernel32_ptr_HeapFree = GetProcAddress(kernel32, "HeapFree");
        lol_throw_if_msg(!kernel32_ptr_HeapFree, "Failed to find kernel32 ptr HeapFree");

        return Kernel32{
            .ptr_CreateFileA = Ptr<ImportTrampoline>(kernel32_ptr_CreateFileA),
            .ptr_CreateFileA_iat = Ptr<uint8_t>(kernel32_ptr_CreateFileA_iat),
            .ptr_CreateFileW = Ptr<uint8_t>(kernel32_ptr_CreateFileW),
            .ptr_GetProcessHeap = Ptr<uint8_t>(kernel32_ptr_GetProcessHeap),
            .ptr_HeapFree = Ptr<uint8_t>(kernel32_ptr_HeapFree),
        };
    }
};

struct Context {
    LineConfig<std::uint64_t, PAT_REVISION, "checksum", "ptr_CreateFileA", "ptr_CRYPTO_free"> config;
    Kernel32 kernel32;
    std::string config_str;
    std::u16string prefix;

    auto load_config(fs::path const& path) -> void {
        kernel32 = Kernel32::load();
        if (std::ifstream file(path, std::ios::binary); file) {
            if (auto str = std::string{}; std::getline(file, str)) {
                config.from_string(str);
            }
        }
        config_str = config.to_string();
    }

    auto save_config(fs::path const& path) -> void {
        config_str = config.to_string();
        auto ec = std::error_code{};
        fs::create_directories(path.parent_path(), ec);
        if (std::ofstream file(path, std::ios::binary); file) {
            file.write(config_str.data(), config_str.size());
        }
    }

    auto set_prefix(fs::path const& profile_path) -> void {
        prefix = fs::absolute(profile_path.lexically_normal()).generic_u16string();
        for (auto& c : prefix) {
            if (c == u'/') {
                c = u'\\';
            }
        }
        if (!prefix.starts_with(u"\\\\")) {
            prefix = u"\\\\?\\" + prefix;
        }
        if (!prefix.ends_with(u"\\")) {
            prefix.push_back(u'\\');
        }
        if ((prefix.size() + 1) * sizeof(char16_t) >= sizeof(CodePayload::prefix_open_data)) {
            lol_throw_msg("Prefix path too big!");
        }
    }

    auto is_initialized(Process const& process) -> bool {
        if (auto base = process.TryBase()) {
            auto data = std::array<char, 0x1000>{};
            if (process.TryReadMemory((void*)*base, data.data(), data.size())) {
                return true;
            }
        }
        return false;
    }

    auto scan_runtime(Process const& process, std::uint32_t expected_checksum) -> void {
        lol_trace_func();
        auto const base = process.Base();
        auto const data = process.Dump();
        auto const data_span = std::span<char const>(data);

        auto const match_ptr_CreateFileA = find_ptr_CreateFileA(data_span, base);
        lol_throw_if_msg(!match_ptr_CreateFileA, "Failed to find ref to ptr to CreateFileA!");

        config.get<"ptr_CreateFileA">() = process.Debase((PtrStorage)std::get<1>(*match_ptr_CreateFileA));
        config.get<"checksum">() = expected_checksum;
        config_str = config.to_string();
    }

    auto scan_file(Process const& process, fs::path expected_game_path) -> std::uint32_t {
        lol_trace_func();
        auto process_path = fs::absolute(process.Path().parent_path());

        // verify game path
        if (!expected_game_path.empty()) {
            lol_trace_func(lol_trace_var("{}", process_path), lol_trace_var("{}", expected_game_path));
            expected_game_path = fs::absolute(expected_game_path);
            lol_throw_if_msg(expected_game_path != process_path, "Wrong game directory!");
        }

        // read in executable
        auto file_data = std::vector<char>{};
        {
            std::ifstream file(process.Path(), std::ios::binary);
            file.seekg(0, std::ios::end);
            auto end = file.tellg();
            file.seekg(0, std::ios::beg);
            auto beg = file.tellg();
            file_data.resize((std::size_t)(end - beg));
            file.read(file_data.data(), file_data.size());
            lol_throw_if_msg(!file.good(), "Failed to read game executable");
        }

        // parse PE executable
        auto peex = PeEx{};
        try {
            peex.parse_data(file_data.data(), file_data.size());
        } catch (std::exception const& error) {
            lol_throw_msg("Failed to parse game executable: {}", error.what());
        }

        // find crypto offset
        auto const match_ptr_CRYPTO_free = find_ptr_CRYPTO_free(file_data, 0);
        lol_throw_if_msg(!match_ptr_CRYPTO_free, "Failed to find ref to ptr to match_ptr_CRYPTO_free!");

        // convert file offset to virtual offset
        auto const ptr_CRYPTO_free = peex.raw_to_virtual(std::get<1>(*match_ptr_CRYPTO_free));
        lol_throw_if_msg(!ptr_CRYPTO_free, "Failed to rebase file_off_CRYPTO_free!");

        // store pointer and debug string
        config.get<"ptr_CRYPTO_free">() = (PtrStorage)ptr_CRYPTO_free;
        config_str = config.to_string();

        return peex.checksum();
    }

    auto is_patchable(Process const& process) const noexcept -> bool {
        auto const is_valid_ptr = [](PtrStorage ptr) { return ptr > 0x10000 && ptr < (1ull << 48); };
        auto const ptr_CRYPTO_free = process.Rebase<PtrStorage>(config.get<"ptr_CreateFileA">());
        if (auto result = process.TryRead(ptr_CRYPTO_free); !result || !is_valid_ptr(*result)) {
            return false;
        } else if (!process.TryRead(Ptr<PtrStorage>(*result))) {
            return false;
        }

        if (!kernel32.ptr_CreateFileA_iat.storage) [[unlikely]] {
            auto const ptr_CreateFileA = process.Rebase<PtrStorage>(config.get<"ptr_CreateFileA">());
            if (auto result = process.TryRead(ptr_CreateFileA); !result || !is_valid_ptr(*result)) {
                return false;
            } else if (!process.TryRead(Ptr<PtrStorage>(*result))) {
                return false;
            }
        }
        return true;
    }

    auto patch(Process const& process) const -> void {
        lol_trace_func();

        // Prepare pointers
        auto ptr_payload = process.Allocate<CodePayload>();
        auto ptr_CRYPTO_free = Ptr<Ptr<uint8_t>>(process.Rebase(config.get<"ptr_CRYPTO_free">()));

        // Prepare payload
        auto payload = CodePayload{};
        payload.ptr_GetProcessHeap = kernel32.ptr_GetProcessHeap;
        payload.ptr_HeapFree = kernel32.ptr_HeapFree;
        payload.ptr_CreateFileA = Ptr(ptr_payload->org_CreateFileA.data);
        if (kernel32.ptr_CreateFileA_iat.storage) [[likely]] {
            payload.org_CreateFileA = ImportTrampoline::make_ind(kernel32.ptr_CreateFileA_iat);
        } else {
            payload.org_CreateFileA = ImportTrampoline::make(kernel32.ptr_CreateFileA.storage);
        }
        payload.ptr_CreateFileW = kernel32.ptr_CreateFileW;
        std::copy_n(prefix.data(), prefix.size(), payload.prefix_open_data);

        // Write payload
        process.Write(ptr_payload, payload);
        process.MarkExecutable(ptr_payload);

        // Write hooks
        process.Write(ptr_CRYPTO_free, Ptr(ptr_payload->hook_CRYPTO_free));
        if (kernel32.ptr_CreateFileA_iat.storage) [[likely]] {
            process.Write(kernel32.ptr_CreateFileA, ImportTrampoline::make(ptr_payload->hook_CreateFileA));
        } else {
            auto ptr_CreateFileA = Ptr<Ptr<ImportTrampoline>>(process.Rebase(config.get<"ptr_CreateFileA">()));
            auto ptr_code_CreateFileA = process.Read(ptr_CreateFileA);
            process.Write(ptr_code_CreateFileA, ImportTrampoline::make(ptr_payload->hook_CreateFileA));
        }
    }
};

// I know it is tempting to remove this but:
// - modskinpro hooks some of the functions we hook as well
// - skin changers asume default skin which most of proper custom skins use
//   this in turns make any people go "wHy No WeRk??"
static auto skinhack_detected() -> char const* {
    static constexpr char const* forbiden_files[] = {
        "C:/Fraps/LOLPRO.exe",
        "C:/Fraps/data",
    };
    static constexpr char const* forbiden_titles[] = {
        "R3nzSkin",
    };
    for (auto item : forbiden_files) {
        if (std::error_code ec = {}; fs::exists(item, ec)) {
            return item;
        }
    }
    //   turns out people are working-around this check by starting patcher before
    //   remove the check for now as its just wasting CPU cycles
    //    for (auto item : forbiden_titles) {
    //        if (FindWindowExA(nullptr, nullptr, nullptr, item)) {
    //            return item;
    //        }
    //    }
    return nullptr;
}

[[noreturn]] static void newpatch_detected() {
    lol_trace_func("Skipping first game on a new patch for config...");
    lol_trace_func("Patching should work normally next game!");
    lol_throw_msg("NEW PATCH DETECTED, THIS IS NOT AN ERROR!\n");
}

auto patcher::run(std::function<void(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path,
                  fs::names const& opts) -> void {
    auto is_configless = false;
    for (auto const& o: opts) {
        if (o == "configless") {
            is_configless = true;
        }
    }

    lol_throw_if(skinhack_detected());
    auto ctx = Context{};
    ctx.set_prefix(profile_path);
    ctx.load_config(config_path);
    (void)game_path;
    for (;;) {
        auto pid = run_until_or(
            30s,
            Intervals{10ms},
            [&] {
                update(M_WAIT_START, "");
                return Process::FindPid("League of Legends.exe");
            },
            []() -> std::uint32_t {
                if (auto fallback_pid = Process::FindPidWindow("League of Legends (TM) Client")) {
                    auto pid = Process::FindPid("League of Legends.exe");
                    // if we can find pid by window but not by normal means then league is running as admin
                    lol_throw_if_msg(!pid, "OpenProcess: League running as ADMIN!");
                    return pid;
                }
                return 0;
            });
        if (!pid) {
            continue;
        }

        // We can drop the handle to the process after this.
        {
            // Signal that process has been found.
            update(M_FOUND, "");
            auto process = Process::Open(pid);

            // Wait until process is actually loaded and has its base and data mapped.
            update(M_WAIT_INIT, "");
            run_until_or(
                1min,
                Intervals{1ms, 10ms, 100ms},
                [&] { return ctx.is_initialized(process); },
                []() -> bool { throw PatcherTimeout(std::string("Timed out initiliazation")); });

            // Find necessary offsets and ensure game path.
            update(M_SCAN, "");
            auto checksum = ctx.scan_file(process, game_path);

            // fallback for config based patcher unless explicitly told
            if (!is_configless) {
                ctx.kernel32.ptr_CreateFileA_iat = {};
            }

            if (!ctx.kernel32.ptr_CreateFileA_iat.storage) [[unlikely]] {
                if (!ctx.config.check() || ctx.config.get<"checksum">() != checksum) {
                    process.WaitInitialized((std::uint32_t)-1);
                    ctx.scan_runtime(process, checksum);
                    update(M_NEED_SAVE, ctx.config_str.c_str());
                    ctx.save_config(config_path);
                    newpatch_detected();
                }
            }

            // Wait until process is "patchable".
            update(M_WAIT_PATCHABLE, "");
            run_until_or(
                2min,
                Intervals{1ms, 10ms, 100ms},
                [&] { return ctx.is_patchable(process); },
                []() -> bool { throw PatcherTimeout(std::string("Timed out patchable")); });

            // Perform paching.
            update(M_PATCH, ctx.config_str.c_str());
            ctx.patch(process);
        }

        // This uses the dumb way of re-quering for pid to detect when process is finally shutdown.
        // It used to use WaitForSingleObject but it turns out that is completly broken and unreliable piece of shit.
        // First of it spuriously can "fail" which leaves us wondering what the fuck happend.
        // Second it can spuriously succeed which means the process is still somewhat partially alive but not really.
        // Thirdly it forces us to keep the handle alive for duration of process lifetime.
        update(M_WAIT_EXIT, "");
        run_until_or(
            3h,
            Intervals{5s, 10s, 15s},
            [&, pid] { return Process::FindPid("League of Legends.exe") != pid; },
            []() -> bool { throw PatcherTimeout(std::string("Timed out exit")); });

        // Signal done.
        update(M_DONE, "");
    }
}

#endif
