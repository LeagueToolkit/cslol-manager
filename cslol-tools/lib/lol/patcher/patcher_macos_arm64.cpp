#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
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

// Fopen hook payload, allocated in target process. This is too big to fit in any code cave.
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

filename .req x19
mode .req x20
filename_len .req x21
fopen_org .req x22

.p2align 8
_fopen_hook_shellcode_beg:
    stp     fp, lr, [sp, #-16]!
    mov     fp, sp
    stp     filename, mode, [sp, #-16]!
    stp     filename_len, fopen_org, [sp, #-16]!
    sub     sp, sp, #buffer_size

    mov     filename, x0
    mov     mode, x1

    adr     fopen_org, Lfopen_org_ref    ; fopen_org = (function**)&fopen_org_ref
    ldr     fopen_org, [fopen_org]       ; fopen_org = *fopen_org
    ldr     fopen_org, [fopen_org]       ; fopen_org

Lcheck_args_not_null:
    cbz     filename, Lcall_with_filename
    cbz     mode, Lcall_with_filename

Lcheck_mode_eq_rb:
    ldrb    w2, [mode]
    cmp     w2, #'r'
    b.ne    Lcall_with_filename
    ldrb    w2, [mode, #1]
    cmp     w2, #'b'
    b.ne    Lcall_with_filename
    ldrb    w2, [mode, #2]
    cbnz    w2, Lcall_with_filename


Lget_filename_length:
    mov     filename_len, #0
    mov     x0, filename
    Lget_filename_length_continue:
        ldrb    w2, [x0], #1
        cbz     w2, Lget_filename_length_break
        add     filename_len, filename_len, #1
        cmp     filename_len, 0x80      ; check filename length
        b.ge    Lcall_with_filename
        b       Lget_filename_length_continue
Lget_filename_length_break:

Lcheck_suffix:
    cmp     filename_len, #7         ; strlen(".client") = 7
    b.lt    Lcall_with_filename

    add     x0, filename, filename_len
    sub     x0, x0, #7               ; ptr to last 7 chars + null
    ldr     x2, [x0]                 ; load 8 bytes
    movz    x3, #0x632E, lsl #0      ; ".c"
    movk    x3, #0x696C, lsl #16     ; "li"
    movk    x3, #0x6E65, lsl #32     ; "en"
    movk    x3, #0x0074, lsl #48     ; "t\0"
    cmp     x2, x3
    b.ne    Lcall_with_filename

Lwrite_prefix:
    mov     x0, sp                   ; dst = buffer
    adr     x1, Lprefix              ; src = &prefix
    Lwrite_prefix_continue:
        ldrb    w2, [x1], #1
        strb    w2, [x0], #1
        cbnz    w2, Lwrite_prefix_continue

Lwrite_filename:
    sub     x0, x0, #1               ; dst = buffer[strlen(buffer)]
    mov     x1, filename             ; src = filename
    Lwrite_filename_continue:
        ldrb    w2, [x1], #1
        strb    w2, [x0], #1
        cbnz    w2, Lwrite_filename_continue

Lcall_with_buffer:
    mov     x0, sp                   ; filename = buffer
    mov     x1, mode
    blr     fopen_org
    cbnz    x0, Lreturn

Lcall_with_filename:
    mov     x0, filename
    mov     x1, mode
    blr     fopen_org

Lreturn:
    add     sp, sp, #buffer_size
    ldp     filename_len, fopen_org, [sp], #16
    ldp     filename, mode, [sp], #16
    ldp     fp, lr, [sp], #16
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

// A function that always returns true, used to bypass wad_verify.
// Followed by a pointer to fopen_hook. We place this in executable because we need to be able
// to address it from ImportStub within 4GB range.
struct Payload_wad_verify {
    unsigned char return_true[0x8] = {
        // clang-format off
        0x20, 0x00, 0x80, 0xD2, 0xC0, 0x03, 0x5F, 0xD6,
        // clang-format on
    };
    PtrStorage fopen_hook_ptr = {};  // Will point to Payload_fopen.
};

// Import stub for fopen, used to call fopen_hook. This will overwrite original fopen stub.
struct Payload_import_stub {
    uint32_t adrp;  // adrp x16, <fopen_hook_page>
    uint32_t ldr;   // ldr x16, [x16, <fopen_hook_page_offset>]
    uint32_t br;    // br x16

    static Payload_import_stub create(uint64_t from, uint64_t to) {
        const int64_t page_diff = (int64_t)((to & ~0xFFF) - (from & ~0xFFF)) >> 12;  // Signed 21-bit page offset
        if (page_diff < -0x100000 || page_diff > 0xFFFFF) {
            throw std::runtime_error("Import stub offset too big");
        }

        // Encode ADRP: split 21-bit immediate into immlo (bits 30:29) and immhi (bits 23:5)
        const uint32_t imm21 = page_diff & 0x1FFFFF;
        const uint32_t immlo = (imm21 & 0x3) << 29;            // Bits 1:0  | 30:29
        const uint32_t immhi = ((imm21 >> 2) & 0x7FFFF) << 5;  // Bits 20:2 | 23:5
        return Payload_import_stub{
            .adrp = (uint32_t)(0x90000010 | immhi | immlo),
            .ldr = (uint32_t)(0xF9400210 | (((to & 0xFFF) >> 3) << 10)),
            .br = (uint32_t)(0xD61F0200),
        };
    }
};

static PtrStorage find_wad_verify(const uint8_t* text_beg, const uint8_t* text_end, uint64_t text_addr) {
    // C3 24 80 52  MOV  W3, #0x126
    // 04 20 80 52  MOV  W4, #0x100
    // 36 03 00 94  BL   wad_verify   <= we want address of wad_verify
    uint8_t const PATTERN[] = {0xC3, 0x24, 0x80, 0x52, 0x04, 0x20, 0x80, 0x52};

    const auto i = std::search(text_beg, text_end, std::begin(PATTERN), std::end(PATTERN));

    // Not found or found at the very end of the section (need 4 more bytes for BL instruction)
    if ((text_end - i) < (sizeof(PATTERN) + 4)) return 0;

    const uint8_t* text_bl = i + sizeof(PATTERN);
    const uint32_t instr = *(uint32_t const*)text_bl;
    const uint32_t opcode = instr & 0xFC000000;
    if (opcode != 0x94000000 && opcode != 0x14000000) return 0;
    const int32_t offset = (int32_t)(instr << 6) >> 6;
    const uint64_t bl_offset = text_bl - text_beg;
    const uint64_t bl_pc = text_addr + bl_offset;
    return bl_pc + (int64_t)offset * 4;
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
        macho.parse_data_arm64((MachO::data_t)data.data(), data.size());

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
