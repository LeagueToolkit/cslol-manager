#define WIN32_LEAN_AND_MEAN
/// Basic windows header
#include <windows.h>
/// Extra windows header
#include <psapi.h>
#include <tlhelp32.h>
/// End windows headers
#include "process.hpp"
#include <chrono>
#include <stdexcept>
#include <thread>
#include <utility>
#include <filesystem>

using namespace LCS;

static struct Imports {
    DWORD (NTAPI *NtWriteVirtualMemory)(HANDLE ProcessHandle, LPVOID BaseAddress,
                                        LPCVOID Buffer, ULONG BytesToWrite,
                                        PULONG BytesWritten) = {};
    Imports() {
#define resolve(from, name) name = (decltype (name))GetProcAddress(from, #name)
        auto ntdll = LoadLibraryA("ntdll.dll");
        resolve(ntdll, NtWriteVirtualMemory);
#undef resolve
    }
} imports {};


namespace {
    static inline constexpr DWORD PROCESS_NEEDED_ACCESS = PROCESS_VM_OPERATION | PROCESS_VM_READ |
                                                          PROCESS_VM_WRITE |
                                                          PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
    static inline constexpr size_t DUMP_SIZE = 0x4000000;

    struct deleter {
        inline void operator()(HANDLE handle) const noexcept {
            if (handle) {
                CloseHandle(handle);
            }
        }
    };

    static inline auto SafeWinHandle(HANDLE handle) noexcept {
        if (handle == INVALID_HANDLE_VALUE) {
            handle = nullptr;
        }
        using handle_value = std::remove_pointer_t<HANDLE>;
        return std::unique_ptr<handle_value, deleter>(handle);
    }
}

Process::Process(uint32_t pid) : handle_(OpenProcess(PROCESS_NEEDED_ACCESS, false, pid)) {
    if (handle_ == INVALID_HANDLE_VALUE) {
        handle_ = nullptr;
    }
}

Process::~Process() noexcept {
    if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
}

std::optional<Process> Process::Find(char const *name) {
    PROCESSENTRY32 entry = {};
    entry.dwSize = sizeof(PROCESSENTRY32);
    auto handle = SafeWinHandle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (!handle) {
        return {};
    }
    HMODULE mod = {};
    DWORD modSize = {};
    char dump[1024] = {};
    for (bool i = Process32First(handle.get(), &entry); i;
        i = Process32Next(handle.get(), &entry)) {
        std::filesystem::path ExeFile = entry.szExeFile;
        if (ExeFile.filename().generic_string() == name) {
            auto process = Process(entry.th32ProcessID);
            if (!process.handle_) {
                return {};
            }
            if (!EnumProcessModules(process.handle_, &mod, sizeof(mod), &modSize)) {
                return {};
            }
            if (!ReadProcessMemory(process.handle_, mod, dump, sizeof(dump), nullptr)) {
                return {};
            }
            return std::move(process);
        }
    }
    return {};
}

PtrStorage Process::Base() const {
    if (!base_) {
        HMODULE mod = {};
        DWORD modSize = {};
        if (!EnumProcessModules(handle_, &mod, sizeof(mod), &modSize)) {
            throw std::runtime_error("Failed to get base!");
        }
        base_ = (PtrStorage)(uintptr_t)(mod);
    }
    return base_;
}

std::filesystem::path Process::Path() const {
    wchar_t pathbuf[32767];
    if (auto const ret = QueryFullProcessImageNameW(handle_, 0, pathbuf, 32767)) {
        throw std::runtime_error("Failed to get image path!");
    } else {
        path_ = std::string { pathbuf, pathbuf + ret };
    }
    return path_;
}

uint32_t Process::Checksum() const {
    if (!checksum_) {
        char raw[1024] = {};
        auto base = Base();
        if (!ReadProcessMemory(handle_, (void const *)(uintptr_t)base, raw, sizeof(raw), nullptr)) {
            throw std::runtime_error("Failed to read PE header");
        }
        auto const dos = (PIMAGE_DOS_HEADER)(raw);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            throw std::runtime_error("Failed to get dos header from process");
        }
        auto const nt = (PIMAGE_NT_HEADERS32)(raw + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            throw std::runtime_error("Failed to get nt header from process");
        }
        checksum_ = (uint32_t)(nt->OptionalHeader.CheckSum);
    }
    return checksum_;
}

std::vector<char> Process::Dump() const {
    auto base = Base();
    std::vector<char> buffer = {};
    buffer.resize(DUMP_SIZE);
    for (size_t p = 0; p != DUMP_SIZE; p += 0x1000) {
        ReadProcessMemory(handle_, (void *)(base + p), buffer.data() + p, 0x1000, nullptr);
    }
    return buffer;
}

bool Process::WaitExit(uint32_t delay, uint32_t timeout) const {
    switch (WaitForSingleObject(handle_, timeout)) {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        return false;
    default:
        throw std::runtime_error("Failed to WaitExit!");
    }
}

bool Process::WaitInitialized(uint32_t delay, uint32_t timeout) const {
    return WaitForInputIdle(handle_, timeout) == 0;
}

void Process::ReadMemory(void* address, void *dest, size_t size) const {
    if (!address) {
        throw std::runtime_error("Can not read memory from nullptr");
    }
    if (!ReadProcessMemory(handle_, address, dest, size, nullptr)) {
        throw std::runtime_error("Failed to read memory");
    }
}

void Process::WriteMemory(void* address, void const* src, size_t sizeBytes) const {
    if (!address) {
        throw std::runtime_error("Can not read memory from nullptr");
    }
    if (imports.NtWriteVirtualMemory(handle_, address, src, sizeBytes, nullptr)) {
        throw std::runtime_error("Failed to write memory");
    }
}

void Process::MarkMemoryWritable(void* address, size_t size) const {
    DWORD old = 0;
    if (!VirtualProtectEx(handle_, address, sizeBytes, PAGE_EXECUTE_READWRITE, &old)) {
        throw std::runtime_error("Failed to change protection");
    }
}

void Process::MarkMemoryExecutable(void* address, size_t sizeBytes) const {
    DWORD old = 0;
    if (!VirtualProtectEx(handle_, address, sizeBytes, PAGE_EXECUTE, &old)) {
        throw std::runtime_error("Failed to change protection");
    }
}

void* Process::AllocateMemory(size_t sizeBytes) const {
    auto ptr =
        VirtualAllocEx(handle_, nullptr, sizeBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!ptr) {
        throw std::runtime_error("Failed to allocate memory");
    }
    return ptr;
}
