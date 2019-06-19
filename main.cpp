#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winnt.h>
#include <Psapi.h>
#include "process.hpp"
#include "patscanner.hpp"
#include "def.hpp"
#include "config.hpp"

using namespace std;

static BOOL CALLBACK FindLoL(HWND hwnd, LPARAM lParam) {
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

static bool Scan(Process const& process, Config& config) {
    if(config.checksum != process.checksum) {
        config.checksum = process.checksum;
        config.off_fp = 0;
        config.off_pmeth = 0;
    }
    if(!config.off_pmeth || !config.off_fp) {
        auto const dump = process.Dump();
        auto const base = process.base;
        if(auto const r = fp_pat(dump); auto const x = std::get<1>(r)) {
            config.off_fp = *reinterpret_cast<uint32_t const*>(x) - base;
        }
        if(auto const r = pmeth_pat(dump); auto const x = std::get<1>(r)) {
            config.off_pmeth = *reinterpret_cast<uint32_t const*>(x) - base;
        }
        return true;
    }
    return false;
}

static bool Register(Process const& process, Config const& config) {
    if(!config.off_fp || !config.off_pmeth) {
        return false;
    }
    auto rFPList = process.OffBase<FileProvider::List>(config.off_fp);
    auto rMethods = process.OffBase<EVP_PKEY_METHOD*>(config.off_pmeth);
    auto rPmethOrgPtr = process.Read(rMethods);
    auto lPmeth = process.Read(rPmethOrgPtr);
    auto lFPList = process.Read(rFPList);
    auto rData = process.Allocate<Data>();
    auto rExec = process.Allocate<Shellcode>();

    lPmeth.verify = rExec->Verify;
    lFPList.arr[3] = lFPList.arr[2];
    lFPList.arr[2] = lFPList.arr[1];
    lFPList.arr[1] = lFPList.arr[0];
    lFPList.arr[0] = &rData->fp;
    lFPList.size++;

    Data lData = {
        FileProvider {
            &rData->fpvtbl,
            rFPList
        },
        FileProvider::Vtable {
            rExec->Open,
            rExec->CheckAccess,
            rExec->CreateIterator,
            rExec->VectorDeleter,
            rExec->Destructor,
        },
        lPmeth,
    };
    process.Write(shellcode, rExec);
    process.MarkExecutable(rExec);
    process.MarkExecutable(rExec);
    process.Write(lData, rData);
    process.Write(&rData->pmeth, rMethods);
    process.Write(lFPList, rFPList);
    return true;
}

int main()
{
    puts("Source: https://github.com/moonshadow565/lolskinmod");
    puts("Put your moded files into <LoL Folder>/Game/MOD");
    puts("==================================================");
    Config config = {};
    config.load();
    config.print();
    for(;;) {
        puts("==================================================");
        puts("Waiting for league to start...");
        DWORD pid = 0;
        for(; !pid ; EnumWindows(FindLoL, reinterpret_cast<LPARAM>(&pid))) {
            Sleep(50);
        };
        try {
            auto const process = Process(pid, "League of Legends.exe");
            auto const needsave = Scan(process, config);
            auto const good = Register(process, config);
            if(needsave) {
                puts("Offsets are updated!");
                config.print();
                config.save();
            }
            if(good) {
                puts("Registered!");
            } else {
                puts("No valid offsets!");
            }
            puts("Waiting for league to exit...");
            process.Wait();
        } catch(const std::runtime_error& err) {
            printf("Error: %s\n", err.what());
            config.print();
            getc(stdin);
            return 0;
        }
    }
}
