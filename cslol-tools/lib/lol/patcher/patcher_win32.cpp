#ifdef _WIN32
#    include <algorithm>
#    include <array>
#    include <chrono>
#    include <fstream>
#    include <lol/error.hpp>
#    include <lol/patcher/patcher.hpp>
#    include <thread>

// do not reorder
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
        "48 83 C4 28 "
        "C3 "
        "C7 05 r[?? ?? ?? ??] 00 00 00 00 "
        "48 83 C4 28 "
        "E9 "_pattern,
        "81 00 00 00 "
        "00 00 00 00 "
        "o[08] 00 00 00 "
        "00 00 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00"_pattern,
        "80 00 00 00 "
        "01 00 00 00 "
        "o[FF] FF FF FF "
        "01 00 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00 "
        "?? ?? ?? ?? ?? ?? 00 00"_pattern>;

constexpr auto const find_ret_CRYPTO_free =
    &ppp::any<
        "41 B9 ?? 00 00 00 "
        "48 8B ?? "
        "E8 ?? ?? ?? ?? "
        "u[48 8B 7C 24 ?? 8B C3 ??]"_pattern>;
// clang-format on

struct ImportTrampoline {
    uint8_t data[64] = {};

    static ImportTrampoline make(Ptr<uint8_t> where) {
        ImportTrampoline result = {{0x48, 0xB8u, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0}};
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

    // Hooking CreateFileA import trampoline
    uint8_t hook_CreateFileA[0x100] = {
        0xc8, 0x00, 0x10, 0x00, 0x53, 0x57, 0x56, 0x48,
        0x83, 0xec, 0x48, 0x4c, 0x89, 0x4d, 0x28, 0x4c,
        0x89, 0x45, 0x20, 0x48, 0x89, 0x55, 0x18, 0x48,
        0x89, 0x4d, 0x10, 0x48, 0x8d, 0xbd, 0x00, 0xf0,
        0xff, 0xff, 0x48, 0x8d, 0x35, 0xe7, 0x00, 0x00,
        0x00, 0x66, 0xad, 0x66, 0xab, 0x66, 0x85, 0xc0,
        0x75, 0xf7, 0x48, 0x83, 0xef, 0x02, 0x48, 0x8b,
        0x75, 0x10, 0xb4, 0x00, 0xac, 0x3c, 0x2f, 0x75,
        0x02, 0xb0, 0x5c, 0x66, 0xab, 0x84, 0xc0, 0x75,
        0xf3, 0x48, 0x8b, 0x45, 0x48, 0x48, 0x89, 0x44,
        0x24, 0x30, 0x48, 0x8b, 0x45, 0x38, 0x48, 0x89,
        0x44, 0x24, 0x28, 0x48, 0x8b, 0x45, 0x30, 0x48,
        0x89, 0x44, 0x24, 0x20, 0x4c, 0x8b, 0x4d, 0x28,
        0x4c, 0x8b, 0x45, 0x20, 0x48, 0x8b, 0x55, 0x18,
        0x48, 0x8d, 0x8d, 0x00, 0xf0, 0xff, 0xff, 0x48,
        0x8b, 0x05, 0x8a, 0x00, 0x00, 0x00, 0xff, 0xd0,
        0x48, 0x83, 0xf8, 0xff, 0x75, 0x34, 0x48, 0x8b,
        0x45, 0x48, 0x48, 0x89, 0x44, 0x24, 0x30, 0x48,
        0x8b, 0x45, 0x38, 0x48, 0x89, 0x44, 0x24, 0x28,
        0x48, 0x8b, 0x45, 0x30, 0x48, 0x89, 0x44, 0x24,
        0x20, 0x4c, 0x8b, 0x4d, 0x28, 0x4c, 0x8b, 0x45,
        0x20, 0x48, 0x8b, 0x55, 0x18, 0x48, 0x8b, 0x4d,
        0x10, 0x48, 0x8b, 0x05, 0x48, 0x00, 0x00, 0x00,
        0xff, 0xd0, 0x48, 0x83, 0xc4, 0x48, 0x5e, 0x5f,
        0x5b, 0xc9, 0xc3, 0x90, 0x90, 0x90, 0x90, 0x90,
    };
    Ptr<uint8_t> ptr_CreateFileA = {};
    Ptr<uint8_t> ptr_CreateFileW = {};
    char16_t prefix_open_data[0x400] = {};
    // clang-format on
};

extern "C" {
extern void* GetModuleHandleA(char const* name);
extern void* GetProcAddress(void* module, char const* name);
}

struct Kernel32 {
    Ptr<uint8_t> ptr_CreateFileA;
    Ptr<uint8_t> ptr_CreateFileW;
    Ptr<uint8_t> ptr_GetProcessHeap;
    Ptr<uint8_t> ptr_HeapFree;

    static auto load() -> Kernel32 {
        static auto kernel32 = GetModuleHandleA("KERNEL32.dll");
        static auto kernel32_ptr_CreateFileA = GetProcAddress(kernel32, "CreateFileA");
        static auto kernel32_ptr_CreateFileW = GetProcAddress(kernel32, "CreateFileW");
        static auto kernel32_ptr_GetProcessHeap = GetProcAddress(kernel32, "GetProcessHeap");
        static auto kernel32_ptr_HeapFree = GetProcAddress(kernel32, "HeapFree");
        lol_throw_if_msg(!kernel32_ptr_CreateFileA, "Failed to find kernel32 ptr CreateFileA");
        lol_throw_if_msg(!kernel32_ptr_CreateFileW, "Failed to find kernel32 ptr CreateFileW");
        lol_throw_if_msg(!kernel32_ptr_GetProcessHeap, "Failed to find kernel32 ptr GetProcessHeap");
        lol_throw_if_msg(!kernel32_ptr_HeapFree, "Failed to find kernel32 ptr HeapFree");
        return Kernel32{
            .ptr_CreateFileA = Ptr<uint8_t>(kernel32_ptr_CreateFileA),
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

    auto scan(Process const& process) -> void {
        lol_trace_func();
        auto const base = process.Base();
        auto const data = process.Dump();
        auto const data_span = std::span<char const>(data);

        auto const match_ptr_CreateFileA = find_ptr_CreateFileA(data_span, base);
        lol_throw_if_msg(!match_ptr_CreateFileA, "Failed to find ref to ptr to CreateFileA!");

        auto const match_ptr_CRYPTO_free = find_ptr_CRYPTO_free(data_span, base);
        lol_throw_if_msg(!match_ptr_CRYPTO_free, "Failed to find ref to ptr to match_ptr_CRYPTO_free!");

        config.get<"ptr_CreateFileA">() = process.Debase((PtrStorage)std::get<1>(*match_ptr_CreateFileA));
        config.get<"ptr_CRYPTO_free">() = process.Debase((PtrStorage)std::get<1>(*match_ptr_CRYPTO_free)) + 0x18;
        config.get<"checksum">() = process.Checksum();
    }

    auto check(Process const& process) const -> bool {
        return config.check() && process.Checksum() == config.get<"checksum">();
    }

    auto is_patchable(const Process& process) const noexcept -> bool {
        auto is_valid_ptr = [](PtrStorage ptr) { return ptr > 0x10000 && ptr < (1ull << 48); };

        auto const ptr_CRYPTO_free = process.Rebase<PtrStorage>(config.get<"ptr_CRYPTO_free">());
        if (auto result = process.TryRead(ptr_CRYPTO_free); !result || !is_valid_ptr(*result)) {
            return false;
        } else if (!process.TryRead(Ptr<PtrStorage>(*result))) {
            return false;
        }

        auto const ptr_CreateFileA = process.Rebase<PtrStorage>(config.get<"ptr_CreateFileA">());
        if (auto result = process.TryRead(ptr_CreateFileA); !result || !is_valid_ptr(*result)) {
            return false;
        } else if (!process.TryRead(Ptr<PtrStorage>(*result))) {
            return false;
        }
        return true;
    }

    auto patch(Process const& process) const -> void {
        lol_trace_func();
        lol_throw_if_msg(!config.check(), "Config invalid");

        // Prepare pointers
        auto ptr_payload = process.Allocate<CodePayload>();
        auto ptr_CreateFileA = Ptr<Ptr<ImportTrampoline>>(process.Rebase(config.get<"ptr_CreateFileA">()));
        auto ptr_CRYPTO_free = Ptr<Ptr<uint8_t>>(process.Rebase(config.get<"ptr_CRYPTO_free">()));
        auto ptr_code_CreateFileA = process.Read(ptr_CreateFileA);

        // Prepare payload
        auto payload = CodePayload{};
        payload.ptr_GetProcessHeap = kernel32.ptr_GetProcessHeap;
        payload.ptr_HeapFree = kernel32.ptr_HeapFree;
        payload.ptr_CreateFileA = kernel32.ptr_CreateFileA;
        payload.ptr_CreateFileW = kernel32.ptr_CreateFileW;
        std::copy_n(prefix.data(), prefix.size(), payload.prefix_open_data);

        // Write payload
        process.Write(ptr_payload, payload);
        process.MarkExecutable(ptr_payload);

        // Write hooks
        process.Write(ptr_CRYPTO_free, Ptr(ptr_payload->hook_CRYPTO_free));
        process.Write(ptr_code_CreateFileA, ImportTrampoline::make(ptr_payload->hook_CreateFileA));
    }

    auto verify_path(Process const& process, fs::path game_path) -> void {
        if (game_path.empty()) return;
        auto process_path = fs::absolute(process.Path().parent_path());
        game_path = fs::absolute(game_path);
        lol_trace_func(lol_trace_var("{}", process_path), lol_trace_var("{}", game_path));
        lol_throw_if_msg(game_path != process_path, "Wrong game directory!");
    }
};

static bool skinhack_detected() {
    std::error_code ec = {};
    return fs::exists("C:/Fraps/LOLPRO.exe", ec);
}

[[noreturn]] static void newpatch_detected() {
    lol_trace_func("Skipping first game on a new patch for config...");
    lol_trace_func("Patching should work normally next game!");
    lol_throw_msg("NEW PATCH DETECTED, THIS IS NOT AN ERROR!\n");
}

auto patcher::run(std::function<bool(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path) -> void {
    lol_throw_if(skinhack_detected());
    auto ctx = Context{};
    ctx.set_prefix(profile_path);
    ctx.load_config(config_path);
    (void)game_path;
    for (;;) {
        auto process = Process::Find("League of Legends.exe", "League of Legends (TM) Client");
        if (!process) {
            if (!update(M_WAIT_START, "")) return;
            std::this_thread::sleep_for(250ms);
            continue;
        }

        ctx.verify_path(*process, game_path);

        if (!update(M_FOUND, "")) return;

        auto patchable = false;
        if (!ctx.check(*process)) {
            for (std::uint32_t timeout = 3 * 60 * 1000; timeout; timeout -= 1) {
                if (!update(M_WAIT_INIT, "")) return;
                if (process->WaitInitialized(1)) {
                    break;
                }
            }

            if (!update(M_SCAN, "")) return;
            ctx.scan(*process);

            if (!update(M_NEED_SAVE, "")) return;
            ctx.save_config(config_path);

            patchable = true;
            // newpatch_detected();
        } else {
            if (!update(M_WAIT_PATCHABLE, "")) return;
            // Fast patch this will pin core to 100% for a few seconds.
            for (auto const start = std::chrono::high_resolution_clock::now();;) {
                if (ctx.is_patchable(*process)) {
                    patchable = true;
                    break;
                }
                auto const end = std::chrono::high_resolution_clock::now();
                auto const diff = std::chrono::duration_cast<std::chrono::seconds>(end - start);
                if (diff.count() > 10) {
                    break;
                }
            }
            for (std::uint32_t timeout = 3 * 60 * 1000; !patchable && timeout; timeout -= 1) {
                if (!update(M_WAIT_PATCHABLE, "")) return;
                if (ctx.is_patchable(*process)) {
                    patchable = true;
                    break;
                }
                std::this_thread::sleep_for(1ms);
            }
        }

        lol_throw_if_msg(!patchable, "Patchable timeout!");
        if (!update(M_PATCH, ctx.config_str.c_str())) return;
        ctx.patch(*process);

        for (std::uint32_t timeout = 3 * 60 * 60 * 1000; timeout; timeout -= 250) {
            if (!update(M_WAIT_EXIT, "")) return;
            if (process->WaitExit(250)) {
                break;
            }
        }

        if (!update(M_DONE, "")) return;
    }
}

#endif
