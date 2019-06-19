#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winnt.h>
#include <Psapi.h>
#include "process.hpp"
#include "patscanner.hpp"
#include "def.hpp"
#include "config.hpp"

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

static bool InjectFP(Process const& process, Config const& config) {
    if(!config.enable_fp) {
        return false;
    }
    if(!config.off_fp) {
        return false;
    }
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
    return true;
}

static bool InjectWad(Process const& process, Config const& config) {
    if(!config.enable_wad) {
        return false;
    }
    if(!config.off_pmeth) {
        return false;
    }
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
    return true;
}

static bool Rescan(Config& config, Process const& process) {
    if((!config.off_pmeth && config.enable_wad) ||
       (!config.off_fp && config.enable_fp)) {
        auto const dump = process.Dump();
        auto const base = process.base;
        if(config.enable_fp) {
            auto const r = fp_pat(dump);
            if(auto const x = std::get<1>(r); x) {
                config.off_fp = *reinterpret_cast<uint32_t const*>(x) - base;
            }
        }
        if(config.enable_wad) {
            auto const r = pmeth_pat(dump);
            if(auto const x = std::get<1>(r); x) {
                config.off_pmeth = *reinterpret_cast<uint32_t const*>(x) - base;
            }
        }
        config.needsave = true;
        return true;
    }
    return false;
}


int main()
{
    puts("Put your moded files into <LoL Folder>/Game/MOD");
    puts("==================================================");
    Config config = {};
    if(!config.load()) {
        config.needsave = true;
    }
    config.print();
    for(;;) {
        puts("==================================================");
        puts("Waiting for league to start...");
        DWORD pid = 0;
        for(; !pid ; EnumWindows(FindWindow, reinterpret_cast<LPARAM>(&pid))) {
            Sleep(50);
        };
        try {
            auto const process = Process(pid, "League of Legends.exe");
            if(config.checksum != process.checksum) {
                config.off_fp = 0;
                config.off_pmeth = 0;
                config.checksum = process.checksum;
            }
            Rescan(config, process);
            auto const wad_res = InjectWad(process, config);
            auto const fp_res = InjectFP(process, config);
            printf("WadVerify added: %u\n", wad_res);
            printf("FileProvider added: %u\n", fp_res);
            if(config.needsave) {
                puts("Offsets are updated!");
                config.print();
                config.save();
            }
            puts("Waiting for league to exit...");
            process.Wait();
        } catch(const std::runtime_error& err) {
            printf("Error: %s\n", err.what());
            if(config.needsave) {
                puts("Offsets are updated!");
                config.print();
                config.save();
            }
            getc(stdin);
            return 0;
        }
    }
}
