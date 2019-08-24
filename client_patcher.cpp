#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winnt.h>
#include <Psapi.h>
#include "process.hpp"
#include "patscanner.hpp"

inline constexpr auto verify_pat = Pattern<
    0x53,                       // push ebbx
    0x56,                       // push esi
    0x8B, 0x74, 0x24, 0x0C,     // mov esi, [esp + 8 + arg_0]
    0x57,                       // push edi
    0x8B, 0x46, 0x08,           // mov eax, [esi + 8]
    0x8B, 0x7E, 0x14,           // mov edi, [esi + 0x14 ]
    0x8B, 0x58, 0x18,           // mov ebx, [esi + 0x18 ]
    0x8B, 0x47, 0x18,           // mov eax, [edi + 0x18 ]
    0x85, 0xC0                  // test eax, eax
>{};

struct VerifyPatch {
    uint8_t code[16] = {
        0xB8,0x01,0x00,0x00,0x00,0xC3,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };
};

static void PatchClient(Process const& process) {
    auto const modules = process.GetModules();
    auto const base = ModuleFind(modules, "libcrypto-1_1.dll");
    auto const header = process.GetHeader(base);
    auto const dump = process.Dump(base, header.codeSize);
    auto const data = reinterpret_cast<uint8_t const*>(dump.data());
    
    auto const sig = verify_pat(data, dump.size());
    if (!std::get<0>(sig)) {
        return;
    }
    
    auto const offset = std::get<0>(sig) - data;
    auto const remote = reinterpret_cast<VerifyPatch*>(base + offset);
    auto const oldprotect = process.SetProtection(remote, PAGE_EXECUTE_READWRITE);
    process.Write(VerifyPatch{}, remote);
    process.SetProtection(remote, oldprotect);
}

template<typename F>
static bool ForEachProcess(F&& func) {
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return 0;
    }
    cProcesses = cbNeeded / sizeof(DWORD);

    for (i = 0; i < cProcesses; i++) {
        if (!aProcesses[i]) {
            continue;
        }
        auto const pid = aProcesses[i];
        auto const hProcess = OpenProcess(PROCESS_QUERY_INFORMATION 
            | PROCESS_VM_READ,
            false, pid);
        if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
            continue;
        }
        HMODULE hMod;
        DWORD cbNeeded;
        char szProcessName[MAX_PATH];
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName));
        }
        CloseHandle(hProcess);
        if (func(pid, szProcessName)) {
            return true;
        }
    }
    return false;
}

int main() {
    for (;;) {
        bool found = false;
        printf("Waiting for LeagueClientUx.exe...\n");
        while (!found) {
            found = ForEachProcess([](DWORD pid, char const* name) -> bool {
                if (!strstr(name, "LeagueClientUx.exe")) {
                    return false;
                }
                try {
                    auto const process = Process(pid);
                    PatchClient(process);
                    printf("Found process, PID(%u), name(%s)\n", pid, name);
                    printf("Waiting for exit...\n");
                    process.Wait();
                } catch (std::exception const& e) {
                    printf("Error: %s", e.what());
                }
                return true;
            });
            Sleep(5);
        }
    }
    return 0;
}