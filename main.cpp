#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winnt.h>
#include <Psapi.h>
#include "process.hpp"
#include "patscanner.hpp"
#include "def.hpp"

using namespace std;

BOOL CALLBACK FindWindow(HWND hwnd, LPARAM lParam) {
    char exename[256] = {0};
    if(GetWindowTextA(hwnd, exename, sizeof(exename) - 1)) {
        for(auto const& target: classnames){
            if(!strcmp(exename, target)) {
                DWORD pid = 0;
                if(GetWindowThreadProcessId(hwnd, &pid)){
                    *reinterpret_cast<DWORD*>(lParam) = pid;
                    return FALSE;
                };
            }
        }
    }
    return TRUE;
}

struct Config {
    uint32_t checksum = 0;
    uintptr_t off_fp = 0;
    uintptr_t off_pmeth = 0;
    uint32_t enable_fp = 1;
    uint32_t enable_wad = 0;
    bool needsave;

    void print() const {
        puts("==================================================");
        printf("Checksum: 0x%08X\n", checksum);
        printf("FileProvider(%u): 0x%08X\n", enable_fp,off_fp);
        printf("Wad(%u): 0x%08X\n",enable_wad, off_pmeth);
        puts("==================================================");
    }

    void save() const {
        if(FILE* file = nullptr;
                !fopen_s(&file, "lolskinmod.txt", "w") && file) {
            fprintf_s(file, "0x%08X 0x%08X 0x%08X %u %u\n",
                      checksum, off_fp, off_pmeth, enable_fp, enable_wad);
            fclose(file);
        }
    }

    bool load() {
        if(FILE* file = nullptr;
                !fopen_s(&file, "lolskinmod.txt", "r") && file) {
            fscanf_s(file, "0x%08X 0x%08X 0x%08X %u %u\n",
                      &checksum, &off_fp, &off_pmeth, &enable_fp, &enable_wad);
            fclose(file);
            return true;
        }
        return false;
    }

    bool need_scan(Process const& process) {
        if(checksum != process.checksum) {
            off_fp = 0;
            off_pmeth = 0;
            checksum = process.checksum;
            needsave = true;
        }
        if((!off_fp && enable_fp) || (!off_pmeth && enable_wad)) {
            auto const dump = process.Dump();
            auto const base = process.base;
            if(!off_fp && enable_fp) {
                auto const r = fp_pat(dump);
                if(auto const x = std::get<1>(r); x) {
                    off_fp = *reinterpret_cast<uint32_t const*>(x) - base;
                }
            }
            if(!off_pmeth && enable_wad) {
                auto const r = pmeth_pat(dump);
                if(auto const x = std::get<1>(r); x) {
                    off_pmeth = *reinterpret_cast<uint32_t const*>(x) - base;
                }
            }
            return true;
        }
        return false;
    }
};

static void InjectFP(Process const& process, Config& config) {
    auto rList = process.OffBase<FileProvider::List>(config.off_fp);
    auto lList = process.Read(rList);
    auto rExe = process.Allocate<Shellcode>();
    auto rFP = process.Allocate<FileProvider>();
    FileProvider lFP = {
        &rFP->impl,
        rList,
        {
            rExe->Open,
            rExe->CheckAccess,
            rExe->CreateIterator,
            rExe->VectorDeleter,
            rExe->Destructor,
        },
    };
    lList.arr[3] = lList.arr[2];
    lList.arr[2] = lList.arr[1];
    lList.arr[1] = lList.arr[0];
    lList.arr[0] = rFP;
    lList.size++;

    process.Write(shellcode, rExe);
    process.MarkExecutable(rExe);
    process.Write(lFP, rFP);
    process.Write(lList, rList);
}

static void InjectWad(Process const& process, Config& config) {
    auto rMethods = process.OffBase<EVP_PKEY_METHOD*>(config.off_pmeth);
    auto rVtableOrg = process.Read(rMethods);
    auto lVtable = process.Read(rVtableOrg);
    auto rVtable = process.Allocate<EVP_PKEY_METHOD>();
    auto rVerify = process.Allocate<EVP_Verify>();
    lVtable.verify = rVerify->Verify;
    process.Write(lVtable, rVtable);
    process.Write(verifycode, rVerify);
    process.MarkExecutable(rVerify);
    process.Write(rVtable, rMethods);
}

int main(int argc, char** argv)
{
    DWORD pid = 0;
    Config config = {};
    if(!config.load()) {
        config.needsave = true;
    }
    config.print();
    // .wad's not officialy support
    // but if you can figure how to pass args to cli its here to use
    for(int a = 1; a < argc; a++) {
        if(strstr(argv[a], "-fp")) {
            config.enable_wad = true;
            config.needsave = true;
        }
        if(strstr(argv[a], "+fp")) {
            config.enable_wad = false;
            config.needsave = true;
        }
        if(strstr(argv[a], "-wad")) {
            config.enable_wad = true;
            config.needsave = true;
        }
        if(strstr(argv[a], "+wad")) {
            config.enable_wad = false;
            config.needsave = true;
        }
    }
    for(; !pid ; EnumWindows(FindWindow, reinterpret_cast<LPARAM>(&pid))) {
        Sleep(50);
    };
    try {
        auto const process = Process(pid, "League of Legends.exe");
        config.need_scan(process);
        if(config.enable_fp) {
            if(config.off_fp) {
                InjectFP(process, config);
                printf("FileProvider Injected!\n");
            } else {
                printf("No file provider offset!");
            }
        }
        if(config.enable_wad) {
            if(config.off_pmeth) {
                InjectWad(process, config);
                printf("Wad Injected!\n");
            } else {
                printf("No pmeth offset!");
            }
        }
    } catch(const std::runtime_error& err) {
        printf("Error: %s\n", err.what());
    }
    config.print();
    if(config.needsave) {
        config.save();
    }
    getc(stdin);
    return 0;
}
