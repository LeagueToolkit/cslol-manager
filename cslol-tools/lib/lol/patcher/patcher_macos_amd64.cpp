#if defined(__APPLE__) && (defined(__x86_64__) || defined(__amd64__))
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

struct Payload_fopen_hook {
    unsigned char fopen_hook[0x100] = {};
    PtrStorage fopen_org_ptr = {};
    char prefix[0x100] = {};
};

__asm__(R"(
.text

.global _fopen_hook_shellcode_beg
.global _fopen_hook_shellcode_end

.set buffer_size, 0x200

# Register assignments:
# %r12 = filename
# %r13 = mode  
# %r14 = filename_len

.p2align 8
_fopen_hook_shellcode_beg:
Lsetup:
    push    %rbp
    mov     %rsp, %rbp
    push    %r12
    push    %r13
    push    %r14
    sub     $buffer_size, %rsp
    mov     %rdi, %r12              # filename = arg0
    mov     %rsi, %r13              # mode = arg1

Lcheck_args_not_null:
    test    %r12, %r12              # if (!filename)
    je      Lcall_with_filename
    test    %r13, %r13              # if (!mode)
    je      Lcall_with_filename

Lcheck_mode_eq_rb:
    cmpb    $'r', (%r13)            # if (mode[0] != 'r')
    jne     Lcall_with_filename
    cmpb    $'b', 1(%r13)           # if (mode[1] != 'b')
    jne     Lcall_with_filename
    cmpb    $0, 2(%r13)             # if (mode[2] != '\0')
    jne     Lcall_with_filename

Lget_filename_length:
    xor     %r14, %r14              # filename_len = 0
    mov     %r12, %rdi              # ptr = filename
    Lget_filename_length_continue:
        movzbl  (%rdi), %eax
        test    %al, %al
        je      Lget_filename_length_break
        inc     %r14                # filename_len++
        inc     %rdi
        cmp     $0x80, %r14         # if (filename_len >= 128)
        jge     Lcall_with_filename
        jmp     Lget_filename_length_continue
Lget_filename_length_break:

Lcheck_suffix:
    cmp     $7, %r14                # if (filename_len < 7) // strlen(".client")
    jl      Lcall_with_filename

    lea     (%r12, %r14, 1), %rdi   # ptr = filename + filename_len - 7
    sub     $7, %rdi
    mov     (%rdi), %rax            # load 8 bytes
    movabs  $0x00746E65696C632E, %rcx   # ".client\0" in little-endian
    cmp     %rcx, %rax
    jne     Lcall_with_filename

Lwrite_prefix:
    mov     %rsp, %rdi              # dst = buffer
    lea     Lprefix(%rip), %rsi     # src = prefix
    Lwrite_prefix_continue:
        lodsb
        stosb
        test    %al, %al
        jne     Lwrite_prefix_continue

Lwrite_filename:
    dec     %rdi                    # dst = buffer[strlen(buffer)]
    mov     %r12, %rsi              # src = filename
    Lwrite_filename_continue:
        lodsb
        stosb
        test    %al, %al
        jne     Lwrite_filename_continue

Lcall_with_buffer:
    mov     %rsp, %rdi              # arg0 = buffer
    mov     %r13, %rsi              # arg1 = mode
    mov     Lfopen_org_ref(%rip), %rax
    call    *(%rax)                 # Stack is 16-byte aligned here
    test    %rax, %rax
    jne     Lreturn

Lcall_with_filename:
    mov     %r12, %rdi              # arg0 = filename
    mov     %r13, %rsi              # arg1 = mode
    mov     Lfopen_org_ref(%rip), %rax
    call    *(%rax)                 # Stack is 16-byte aligned here

Lreturn:
    add     $buffer_size, %rsp
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbp
    ret

.p2align 8
_fopen_hook_shellcode_end:

Lfopen_org_ref:
    .quad   0x11223344556677

Lprefix:
    .quad   0x11223344556677
)");

extern "C" {
extern unsigned char fopen_hook_shellcode_beg[];
extern unsigned char fopen_hook_shellcode_end[];
}

struct Payload_wad_verify {
    unsigned char ret_true[8] = {
        // clang-format off
        0xb8, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
        0xc2, 0x00, 0x00                      // ret 0
        // clang-format on
    };
    PtrStorage fopen_hook_ptr = {};
};

struct Payload_import_stub {
    uint8_t stub[6] = {};

    static Payload_import_stub create(uint64_t from, uint64_t to) {
        const int64_t offset = (int64_t)to - (from + sizeof(stub));  // RIP-relative offset from end of stub
        if (offset < std::numeric_limits<int32_t>::min() || offset > std::numeric_limits<int32_t>::max()) {
            throw std::runtime_error("Import stub offset too big");
        }
        Payload_import_stub result{};
        const uint32_t offset_as_uint32 = static_cast<uint32_t>(offset);
        result.stub[0] = 0xff;  // jmp [rip+0]
        result.stub[1] = 0x25;
        result.stub[2] = offset_as_uint32 & 0xFF;
        result.stub[3] = (offset_as_uint32 >> 8) & 0xFF;
        result.stub[4] = (offset_as_uint32 >> 16) & 0xFF;
        result.stub[5] = (offset_as_uint32 >> 24) & 0xFF;
        return result;
    }
};

static PtrStorage find_wad_verify(const uint8_t* text_beg, const uint8_t* text_end, uint64_t text_addr) {
    // B9 26 01 00 00    mov     ecx, 126h
    // 41 B8 00 01 00 00 mov     r8d, 100h
    // 4C 89 F6          mov     rsi, r14
    // E8 EB 0C 00 00    call    wad_verify <= we want address of wad_verify
    uint8_t const PATTERN[] = {
        // clang-format off
        0xB9, 0x26, 0x01, 0x00, 0x00, 0x41, 0xB8, 0x00, 0x01, 0x00, 0x00, 0x4C, 0x89, 0xF6, 0xE8
        // clang-format on
    };

    const auto i = std::search(text_beg, text_end, std::begin(PATTERN), std::end(PATTERN));

    // Not found or found at the very end of the section (need 4 more bytes for BL instruction)
    if ((text_end - i) < (sizeof(PATTERN) + 4)) return 0;

    const uint8_t* text_disp = i + sizeof(PATTERN);
    const int32_t disp = (int32_t)((uint32_t)text_disp[0] | ((uint32_t)text_disp[1] << 8) |
                                   ((uint32_t)text_disp[2] << 16) | ((uint32_t)text_disp[3] << 24));
    const uint64_t call_offset = text_addr + (text_disp + 4) - text_beg;
    return call_offset + (uint64_t)disp;
}

struct Context {
    std::uint64_t off_wad_verify = {};
    std::uint64_t off_fopen_ptr = {};
    std::uint64_t off_fopen_stub = {};
    std::string prefix;

    auto set_prefix(fs::path const& profile_path) -> void {
        prefix = fs::absolute(profile_path.lexically_normal()).generic_string();
        if (!prefix.ends_with('/')) {
            prefix.push_back('/');
        }
        if (prefix.size() > sizeof(Payload_fopen_hook::prefix) - 1) {
            lol_throw_msg("Prefix path too big!");
        }
    }

    auto scan(Process const& process) -> void {
        auto data = process.Dump();
        auto macho = MachO{};
        macho.parse_data_amd64((MachO::data_t)data.data(), data.size());

        auto const [text_addr, text_data, text_size] = macho.find_section("__text");
        if (!text_data) {
            throw std::runtime_error("Failed to find __text section");
        }

        off_wad_verify = find_wad_verify(text_data, text_data + text_size, text_addr);
        if (!off_wad_verify) {
            throw std::runtime_error("Failed to find wad_verify call");
        }

        if (!(off_fopen_ptr = macho.find_import_ptr("_fopen"))) {
            throw std::runtime_error("Failed to find fopen org");
        }

        if (!(off_fopen_stub = macho.find_stub_refs(off_fopen_ptr))) {
            throw std::runtime_error("Failed to find fopen stub");
        }
    }

    auto patch(Process const& process) -> void {
        auto const ptr_fopen_hook = process.Allocate<Payload_fopen_hook>();
        auto const ptr_wad_verify = process.Rebase<Payload_wad_verify>(off_wad_verify);
        auto const ptr_fopen_ptr = process.Rebase(off_fopen_ptr);
        auto const ptr_fopen_stub = process.Rebase<Payload_import_stub>(off_fopen_stub);

        auto payload_fopen = Payload_fopen_hook{};
        payload_fopen.fopen_org_ptr = ptr_fopen_ptr;
        memcpy(payload_fopen.fopen_hook, fopen_hook_shellcode_beg, sizeof(Payload_fopen_hook::fopen_hook));
        memcpy(payload_fopen.prefix, prefix.c_str(), prefix.size() + 1);

        auto payload_wad_verify = Payload_wad_verify{};
        payload_wad_verify.fopen_hook_ptr = (PtrStorage)ptr_fopen_hook;

        auto payload_import_stub =
            Payload_import_stub::create((PtrStorage)ptr_fopen_stub,
                                        (PtrStorage)ptr_wad_verify + offsetof(Payload_wad_verify, fopen_hook_ptr));

        // Write shellcode first.
        process.MarkWritable(ptr_fopen_hook);
        process.Write(ptr_fopen_hook, payload_fopen);
        process.MarkExecutable(ptr_fopen_hook);

        // Write wad verify payload first.
        process.MarkWritable(ptr_wad_verify);
        process.Write(ptr_wad_verify, payload_wad_verify);
        process.MarkExecutable(ptr_wad_verify);

        // Write fopen import stub hook last.
        process.MarkWritable(ptr_fopen_stub);
        process.Write(ptr_fopen_stub, payload_import_stub);
        process.MarkExecutable(ptr_fopen_stub);
    }
};

auto patcher::run(std::function<void(Message, char const*)> update,
                  fs::path const& profile_path,
                  fs::path const& config_path,
                  fs::path const& game_path,
                  fs::names const& opts) -> void {
    if ((fopen_hook_shellcode_end - fopen_hook_shellcode_beg) != sizeof(Payload_fopen_hook::fopen_hook)) {
        throw std::runtime_error("fopen hook miscompiled!");
    }

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
