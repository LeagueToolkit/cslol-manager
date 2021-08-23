#include "modoverlay.hpp"
#include "macho.hpp"
#include "process.hpp"
#include <cstring>

using namespace LCS;

char const ModOverlay::SCHEMA[] = "lcs-overlay-osx v5 0x%016llX 0x%016llX 0x%016llX";
char const ModOverlay::INFO[] = "lcs-overlay-osx v5 off_rsa_verify off_fopen_org off_fopen_ref";

struct ModOverlay::Config {
    MachO macho = {};
    std::uint64_t off_rsa_verify = {};
    std::uint64_t off_fopen_org = {};
    std::uint64_t off_fopen_ref = {};

    std::string to_string() const noexcept {
        char buffer[sizeof(SCHEMA) * 4] = {};
        int size = sprintf(buffer, SCHEMA, off_rsa_verify, off_fopen_org, off_fopen_ref);
        return std::string(buffer, (size_t)size);
    }

    void from_string(std::string const &buffer) noexcept {
        off_rsa_verify = 0;
        off_fopen_org = 0;
        off_fopen_ref = 0;
    }

    void scan(LCS::Process const &process) {
        auto data = process.Dump();
        macho.parse_data((MachO::data_t)data.data(), data.size());
        if (!(off_rsa_verify = macho.find_export("_int_rsa_verify"))) {
            throw std::runtime_error("Failed to find int_rsa_verify");
        }
        if (!(off_fopen_org = macho.find_import_ptr("_fopen"))) {
            throw std::runtime_error("Failed to find fopen org");
        }
        if (!(off_fopen_ref = macho.find_stub_refs(off_fopen_org))) {
            throw std::runtime_error("Failed to find fopen ref");
        }
    }

    struct Payload {
        unsigned char ret_true[8] = {
            0xb8, 0x01, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x00,
        };
        PtrStorage fopen_hook_ptr = {};
        unsigned char fopen_hook[0x80] = {
            0x55, 0x48, 0x89, 0xe5, 0x41, 0x54, 0x41, 0x55,
            0x48, 0x81, 0xec, 0x00, 0x08, 0x00, 0x00, 0x49,
            0x89, 0xfc, 0x49, 0x89, 0xf5, 0x4d, 0x85, 0xe4,
            0x74, 0x4a, 0x4d, 0x85, 0xed, 0x74, 0x45, 0x41,
            0x80, 0x7d, 0x00, 0x72, 0x75, 0x3e, 0x41, 0x80,
            0x7d, 0x01, 0x62, 0x75, 0x37, 0x41, 0x80, 0x7d,
            0x02, 0x00, 0x75, 0x30, 0x48, 0x89, 0xe7, 0x48,
            0x8d, 0x35, 0x4a, 0x00, 0x00, 0x00, 0xac, 0xaa,
            0x84, 0xc0, 0x75, 0xfa, 0x48, 0xff, 0xcf, 0x4c,
            0x89, 0xe6, 0xac, 0xaa, 0x84, 0xc0, 0x75, 0xfa,
            0x48, 0x89, 0xe7, 0x4c, 0x89, 0xee, 0x48, 0x8b,
            0x05, 0x23, 0x00, 0x00, 0x00, 0xff, 0x10, 0x48,
            0x85, 0xc0, 0x75, 0x0f, 0x4c, 0x89, 0xe7, 0x4c,
            0x89, 0xee, 0x48, 0x8b, 0x05, 0x0f, 0x00, 0x00,
            0x00, 0xff, 0x10, 0x48, 0x81, 0xc4, 0x00, 0x08,
            0x00, 0x00, 0x41, 0x5d, 0x41, 0x5c, 0x5d, 0xc3,
        };
        PtrStorage fopen_org_ptr = {};
        char prefix[0x200] = {};
    };

    void patch(LCS::Process const &process, std::string prefix) {
        if (!prefix.ends_with('/')) {
            prefix.push_back('/');
        }
        auto payload = Payload {};
        memcpy(payload.prefix, prefix.c_str(), prefix.size() + 1);
        payload.fopen_org_ptr = process.Rebase(off_fopen_org);

        auto const ptr_rsa_verify = process.Rebase<Payload>(off_rsa_verify);
        auto const ptr_fopen_ref = process.Rebase<int32_t>(off_fopen_ref);

        // fopen_hook_ptr stores pointer to fopen_hook, fopen_ref uses relative addressing to call it
        payload.fopen_hook_ptr = (PtrStorage)ptr_rsa_verify + offsetof(Payload, fopen_hook);
        auto const fopen_hook_rel = (std::int32_t)(
                    + (std::int64_t)((PtrStorage)ptr_rsa_verify + offsetof(Payload, fopen_hook_ptr))
                    - (std::int64_t)((PtrStorage)ptr_fopen_ref + 4));

        // Write payload
        process.MarkWritable(ptr_rsa_verify);
        process.Write(ptr_rsa_verify, payload);
        process.MarkExecutable(ptr_rsa_verify);

        // Write fopen ref
        process.MarkWritable(ptr_fopen_ref);
        process.Write(ptr_fopen_ref, fopen_hook_rel);
        process.MarkExecutable(ptr_fopen_ref);
    }
};

ModOverlay::ModOverlay() : config_(std::make_unique<Config>()) {}

ModOverlay::~ModOverlay() = default;

std::string ModOverlay::to_string() const noexcept {
    return config_->to_string();
}

void ModOverlay::from_string(std::string const & str) noexcept {
    config_->from_string(str);
}

int ModOverlay::run(std::function<bool(Message)> update, std::filesystem::path const& profilePath) {
    auto process = Process::Find("/LeagueofLegends");
    if (!process) {
        LCS::SleepMS(50);
        return 0;
    }
    if (!update(M_FOUND)) return -1;
    if (!update(M_SCAN)) return -1;
    config_->scan(*process);
    if (!update(M_PATCH)) return -1;
    config_->patch(*process, profilePath.generic_string());
    if (!update(M_WAIT_EXIT)) return -1;
    process->WaitExit();
    return 1;
}
