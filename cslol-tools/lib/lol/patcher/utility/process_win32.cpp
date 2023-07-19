#ifdef _WIN32
#    include <lol/error.hpp>

#    include "process.hpp"
// do not reorder
#    define WIN32_LEAN_AND_MEAN
// do not reorder
#    include <windows.h>
// do not reorder
#    include <process.h>
#    include <psapi.h>
#    include <tlhelp32.h>
// do not reorder
#    define last_error() std::error_code((int)GetLastError(), std::system_category())

namespace {
    static inline constexpr DWORD PROCESS_NEEDED_ACCESS =
        PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION;
    static inline constexpr size_t DUMP_SIZE = 0x4000000;
}

using namespace lol;
using namespace lol::patcher;

Process::Process() noexcept = default;

Process::Process(Process&& other) noexcept {
    std::swap(handle_, other.handle_);
    std::swap(base_, other.base_);
    std::swap(path_, other.path_);
    std::swap(pid_, other.pid_);
}

Process& Process::operator=(Process&& other) noexcept {
    if (this == &other) return *this;
    std::swap(handle_, other.handle_);
    std::swap(base_, other.base_);
    std::swap(path_, other.path_);
    std::swap(pid_, other.pid_);
    return *this;
}

Process::~Process() noexcept {
    if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
}

auto Process::FindPid(char const* name, char const* window) -> std::uint32_t {
    if (name) {
        auto entry = PROCESSENTRY32{.dwSize = sizeof(PROCESSENTRY32)};
        auto handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        for (bool i = Process32First(handle, &entry); i; i = Process32Next(handle, &entry)) {
            std::filesystem::path ExeFile = entry.szExeFile;
            if (ExeFile.filename().generic_string() == name) {
                CloseHandle(handle);
                return entry.th32ProcessID;
            }
        }
        CloseHandle(handle);
    }
    if (window) {
        if (auto hwnd = FindWindowExA(nullptr, nullptr, nullptr, window)) {
            auto pid = DWORD{};
            GetWindowThreadProcessId(hwnd, &pid);
            return pid;
        }
    }
    return 0;
}

auto Process::Open(std::uint32_t pid) -> Process {
    auto handle = OpenProcess(PROCESS_NEEDED_ACCESS, false, pid);
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        lol_throw_msg("OpenProcess: {}", last_error());
    }
    return Process(handle, pid);
}

auto Process::Base() const -> PtrStorage {
    lol_trace_func();
    if (!base_) {
        HMODULE mod = {};
        DWORD modSize = {};
        if (!EnumProcessModules(handle_, &mod, sizeof(mod), &modSize)) {
            lol_throw_msg("EnumProcessModules: {}", last_error());
        }
        base_ = (PtrStorage)(uintptr_t)(mod);
    }
    return base_;
}

auto Process::TryBase() const noexcept -> std::optional<PtrStorage> {
    if (!base_) {
        HMODULE mod = {};
        DWORD modSize = {};
        if (!EnumProcessModules(handle_, &mod, sizeof(mod), &modSize)) {
            return std::nullopt;
        }
        base_ = (PtrStorage)(uintptr_t)(mod);
    }
    return base_;
}

auto Process::Path() const -> fs::path {
    lol_trace_func();
    if (!path_.empty()) return path_;
    wchar_t pathbuf[32767];
    DWORD size = 32767;
    if (auto const ret = QueryFullProcessImageNameW(handle_, 0, pathbuf, &size)) {
        path_ = std::wstring{pathbuf, pathbuf + size};
    } else {
        lol_throw_msg("QueryFullProcessImageNameW: {}", last_error());
    }
    return path_;
}

auto Process::Dump() const -> std::vector<char> {
    lol_trace_func();
    auto base = this->Base();
    auto buffer = std::vector<char>(DUMP_SIZE);
    for (size_t p = 0; p != DUMP_SIZE; p += 0x1000) {
        TryReadMemory((void*)(base + p), buffer.data() + p, 0x1000);
    }
    return buffer;
}

auto Process::IsExited() const noexcept -> bool {
    switch (WaitForSingleObject(handle_, 1)) {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_TIMEOUT:
            return false;
        default:
            return true;
    }
}

auto Process::TryReadMemory(void* address, void* dest, size_t size) const noexcept -> bool {
    if (!ReadProcessMemory(handle_, address, dest, size, nullptr)) return false;
    return true;
}

auto Process::ReadMemory(void* address, void* dest, size_t size) const -> void {
    lol_trace_func(lol_trace_var("{:p}", address), lol_trace_var("{:p}", dest), lol_trace_var("{:#x}", size));
    lol_throw_if(address == nullptr);
    lol_throw_if(size != 0 && dest == nullptr);
    if (!ReadProcessMemory(handle_, address, dest, size, nullptr)) {
        lol_throw_msg("ReadProcessMemory: {}", last_error());
    }
}

auto Process::WriteMemory(void* address, void const* src, size_t size) const -> void {
    lol_trace_func(lol_trace_var("{:p}", address), lol_trace_var("{:p}", src), lol_trace_var("{:#x}", size));
    lol_throw_if(address == nullptr);
    lol_throw_if(size != 0 && src == nullptr);
#    if defined(CSLOL_TRASH_PC_SUPPORT)
    static DWORD(NTAPI * NtWriteVirtualMemory)(HANDLE ProcessHandle,
                                               LPVOID BaseAddress,
                                               LPCVOID Buffer,
                                               ULONG BytesToWrite,
                                               PULONG BytesWritten) =
        (decltype(NtWriteVirtualMemory))GetProcAddress(LoadLibraryA("ntdll.dll"), "NtWriteVirtualMemory");
    if (auto error = NtWriteVirtualMemory(handle_, address, src, size, nullptr)) {
        lol_throw_msg("WriteProcessMemory: 0x{:08X}", error);
    }
#    else
    if (!WriteProcessMemory(handle_, address, src, size, nullptr)) {
        lol_throw_msg("WriteProcessMemory: {}", last_error());
    }
#    endif
}

auto Process::MarkMemoryWritable(void* address, size_t size) const -> void {
    lol_trace_func(lol_trace_var("{:p}", address), lol_trace_var("{:#x}", size));
    lol_throw_if(address == nullptr);
    DWORD old = 0;
    if (!VirtualProtectEx(handle_, address, size, PAGE_EXECUTE_READWRITE, &old)) {
        lol_throw_msg("VirtualProtectEx: {}", last_error());
    }
}

auto Process::MarkMemoryExecutable(void* address, size_t size) const -> void {
    lol_trace_func(lol_trace_var("{:p}", address), lol_trace_var("{:#x}", size));
    lol_throw_if(address == nullptr);
    DWORD old = 0;
    if (!VirtualProtectEx(handle_, address, size, PAGE_EXECUTE, &old)) {
        lol_throw_msg("VirtualProtectEx: {}", last_error());
    }
}

auto Process::AllocateMemory(size_t size) const -> void* {
    lol_trace_func(lol_trace_var("{:#x}", size));
    auto ptr = VirtualAllocEx(handle_, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!ptr) {
        lol_throw_msg("VirtualAllocEx: {}", last_error());
    }
    return ptr;
}

#endif
