#ifdef __APPLE__
#    include <algorithm>
#    include <chrono>
#    include <cstdio>
#    include <cstring>
#    include <functional>
#    include <thread>

#    include "utility/delay.hpp"
#    include "utility/macho.hpp"
#    include "utility/process.hpp"

// do not reorder
#    include <lol/error.hpp>
#    include <lol/patcher/patcher.hpp>

using namespace lol;
using namespace lol::patcher;
using namespace std::chrono_literals;

struct Payload {
    unsigned char ret_true[8] = {0xb8, 0x01, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x00};
    PtrStorage fopen_hook_ptr = {};
    unsigned char fopen_hook[0x80] = {
        0x55, 0x48, 0x89, 0xe5, 0x41, 0x54, 0x41, 0x55, 0x48, 0x81, 0xec, 0x00, 0x08, 0x00, 0x00, 0x49,
        0x89, 0xfc, 0x49, 0x89, 0xf5, 0x4d, 0x85, 0xe4, 0x74, 0x4a, 0x4d, 0x85, 0xed, 0x74, 0x45, 0x41,
        0x80, 0x7d, 0x00, 0x72, 0x75, 0x3e, 0x41, 0x80, 0x7d, 0x01, 0x62, 0x75, 0x37, 0x41, 0x80, 0x7d,
        0x02, 0x00, 0x75, 0x30, 0x48, 0x89, 0xe7, 0x48, 0x8d, 0x35, 0x4a, 0x00, 0x00, 0x00, 0xac, 0xaa,
        0x84, 0xc0, 0x75, 0xfa, 0x48, 0xff, 0xcf, 0x4c, 0x89, 0xe6, 0xac, 0xaa, 0x84, 0xc0, 0x75, 0xfa,
        0x48, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0x48, 0x8b, 0x05, 0x23, 0x00, 0x00, 0x00, 0xff, 0x10, 0x48,
        0x85, 0xc0, 0x75, 0x0f, 0x4c, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0x48, 0x8b, 0x05, 0x0f, 0x00, 0x00,
        0x00, 0xff, 0x10, 0x48, 0x81, 0xc4, 0x00, 0x08, 0x00, 0x00, 0x41, 0x5d, 0x41, 0x5c, 0x5d, 0xc3,
    };
    PtrStorage fopen_org_ptr = {};
    char prefix[0x200] = {};
};

static uint8_t const PAT_INT_RSA_VERIFY[] = {
    0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41, 0x54, 0x53,
    0x48, 0x83, 0xEC, 0x38, 0x4D, 0x89, 0xCD, 0x4D, 0x89, 0xC4, 0x48, 0x89, 0xCB,
};

struct Context {
    std::uint64_t off_rsa_verify = {};
    std::uint64_t off_fopen_org = {};
    std::uint64_t off_fopen_ref = {};
    std::string prefix;

    auto set_prefix(fs::path const& profile_path) -> void {
        prefix = fs::absolute(profile_path.lexically_normal()).generic_string();
        if (!prefix.ends_with('/')) {
            prefix.push_back('/');
        }
        if (prefix.size() > sizeof(Payload::prefix) - 1) {
            lol_throw_msg("Prefix path too big!");
        }
    }

    auto scan(Process const& process) -> void {
        auto data = process.Dump();
        auto macho = MachO{};
        macho.parse_data((MachO::data_t)data.data(), data.size());

        const auto i_rsa_verify = std::search((uint8_t const*)data.data(),
                                              (uint8_t const*)data.data() + data.size(),
                                              std::begin(PAT_INT_RSA_VERIFY),
                                              std::end(PAT_INT_RSA_VERIFY));
        if (i_rsa_verify == (uint8_t const*)data.data() + data.size()) {
            throw std::runtime_error("Failed to find int_rsa_verify");
        }
        off_rsa_verify = (std::uint64_t)(i_rsa_verify - (uint8_t const*)data.data()) + 0x100000000ull;

        if (!(off_fopen_org = macho.find_import_ptr("_fopen"))) {
            throw std::runtime_error("Failed to find fopen org");
        }
        if (!(off_fopen_ref = macho.find_stub_refs(off_fopen_org))) {
            throw std::runtime_error("Failed to find fopen ref");
        }
    }

    auto patch(Process const& process) -> void {
        auto payload = Payload{};
        memcpy(payload.prefix, prefix.c_str(), prefix.size() + 1);
        payload.fopen_org_ptr = process.Rebase(off_fopen_org);

        auto const ptr_rsa_verify = process.Rebase<Payload>(off_rsa_verify);
        auto const ptr_fopen_ref = process.Rebase<int32_t>(off_fopen_ref);

        // fopen_hook_ptr stores pointer to fopen_hook, fopen_ref uses relative addressing to call it
        payload.fopen_hook_ptr = (PtrStorage)ptr_rsa_verify + offsetof(Payload, fopen_hook);
        auto const fopen_hook_ref =
            (std::int32_t)(+(std::int64_t)((PtrStorage)ptr_rsa_verify + offsetof(Payload, fopen_hook_ptr))  //
                           - (std::int64_t)((PtrStorage)ptr_fopen_ref + 4));

        // Write payload
        process.MarkWritable(ptr_rsa_verify);
        process.Write(ptr_rsa_verify, payload);
        process.MarkExecutable(ptr_rsa_verify);

        // Write fopen ref
        process.MarkWritable(ptr_fopen_ref);
        process.Write(ptr_fopen_ref, fopen_hook_ref);
        process.MarkExecutable(ptr_fopen_ref);
    }
};

auto patcher::run(std::function<void(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path,
                  fs::names const& opts) -> void {
    auto ctx = Context{};
    ctx.set_prefix(profile_path);
    (void)config_path;
    (void)game_path;
    (void)opts;
    for (;;) {
        auto pid = Process::FindPid("/LeagueofLegends");
        if (!pid) {
            update(M_WAIT_START, "");
            sleep_ms(10);
            continue;
        }

        update(M_FOUND, "");
        auto process = Process::Open(pid);

        update(M_SCAN, "");
        ctx.scan(process);

        update(M_PATCH, "");
        ctx.patch(process);

        update(M_WAIT_EXIT, "");
        run_until_or(
            3h,
            Intervals{5s, 10s, 15s},
            [&] { return process.IsExited(); },
            []() -> bool { throw PatcherTimeout(std::string("Timed out exit")); });

        update(M_DONE, "");
    }
}

#endif
