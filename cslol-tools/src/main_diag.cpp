#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
// do not reorder
#include <shellapi.h>
#include <softpub.h>
#include <wbemcli.h>
#include <wintrust.h>
// do not reorder
#include <stdint.h>
#include <stdio.h>
// do not reorder
#include <array>
#include <string>
#include <vector>

#define REP_OK "OK ✅ "
#define REP_SUS "SUS ⚠ "
#define REP_BAD "BAD ❌ "
#define HKLM HKEY_LOCAL_MACHINE
#define HKCU HKEY_CURRENT_USER

static constexpr std::wstring_view COMPAT_BAD[] = {L"League", L"Riot"};

static constexpr std::wstring_view COMPAT_SUS[] = {L"cslol-"};

static constexpr auto KB = 1 * uint64_t{1024};
static constexpr auto MB = KB * uint64_t{1024};
static constexpr auto GB = MB * uint64_t{1024};
static constexpr auto TB = GB * uint64_t{1024};

template <typename T, size_t N>
struct estr_t {
    T data[N]{};

    struct rt {
        volatile T* data;
        operator T const*() const { return data; }
    };

    consteval estr_t(T const (&src)[N]) {
        for (size_t i = 0; i != N - 1; ++i) data[i] = src[i] - 1;
    }

    operator T const*() && {
        const auto cv = reinterpret_cast<T volatile*>(data);
        for (size_t i = 0; i != N - 1; ++i) {
            data[i] += 1;
        }
        return const_cast<T*>(cv);
    }
};

template <estr_t E>
auto operator""_estr() {
    return E;
}

static std::wstring reg_read_str(HKEY key, LPCWSTR subkey, LPCWSTR value) {
    auto buf = std::wstring{};
    auto buf_size = DWORD{};
    auto status = LSTATUS{};
    do {
        buf_size = buf.size() * sizeof(wchar_t);
        status = RegGetValueW(key, subkey, value, RRF_RT_REG_SZ | RRF_ZEROONFAILURE, NULL, buf.data(), &buf_size);
        buf.resize(buf_size / sizeof(wchar_t));
    } while (status == ERROR_MORE_DATA);
    return buf;
}

static DWORD64 reg_read_num(HKEY key, LPCWSTR subkey, LPCWSTR value) {
    auto buf = DWORD64{};
    auto buf_size = DWORD{sizeof(buf)};
    RegGetValueW(key, subkey, value, RRF_RT_REG_DWORD | RRF_RT_REG_QWORD | RRF_ZEROONFAILURE, NULL, &buf, &buf_size);
    return buf;
}

static std::vector<std::wstring> reg_list(HKEY key, LPCWSTR subkey) {
    auto kkey = HKEY{};
    auto results = std::vector<std::wstring>{};
    auto buf = std::array<wchar_t, 0x8000>{};
    auto buf_size = DWORD{buf.size()};
    auto status = RegOpenKeyExW(key, subkey, 0, KEY_QUERY_VALUE, &kkey);
    for (auto i = DWORD{}; status == NO_ERROR; ++i) {
        buf_size = DWORD{buf.size()};
        status = RegEnumValueW(kkey, i, buf.data(), &buf_size, NULL, NULL, NULL, NULL);
        if (!status) results.emplace_back(buf.data(), buf.data() + buf_size);
    }
    RegCloseKey(kkey);
    return results;
}

static std::wstring exe_path() {
    auto name = std::wstring((size_t)0x8000, L'\0');
    const auto len = GetModuleFileNameW(GetModuleHandleW(NULL), name.data(), name.size());
    name.resize(len);
    name.shrink_to_fit();
    return name;
}

static std::wstring basedir(std::wstring const& filename) { return filename.substr(0, filename.find_last_of(L"\\/")); }

static uint64_t free_space(std::wstring const& dir) {
    ULARGE_INTEGER result = {};
    GetDiskFreeSpaceExW(dir.empty() ? NULL : dir.c_str(), &result, NULL, NULL);
    return result.QuadPart;
}

static std::wstring bytes_to_str(uint64_t x) {
    if (x >= TB) return std::to_wstring(x / TB) + L"TB";
    if (x >= GB) return std::to_wstring(x / GB) + L"GB";
    if (x >= MB) return std::to_wstring(x / MB) + L"MB";
    if (x >= KB) return std::to_wstring(x / KB) + L"KB";
    return std::to_wstring(x) + L"B";
}

static void check_basic_info() {
    auto const ver_major = *(uint32_t const*)(0x7ffe0000 + 0x26c);
    auto const ver_minor = *(uint32_t const*)(0x7ffe0000 + 0x270);
    auto const ver_build = *(uint32_t const*)(0x7ffe0000 + 0x260);
    auto const flags = *(uint32_t const*)(0x7ffe0000 + 0x2f0);  // SharedDataFlags
    auto const cpu =
        reg_read_str(HKLM, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"_estr, L"ProcessorNameString"_estr);
    auto const long_path =
        reg_read_num(HKLM, L"SYSTEM\\CurrentControlSet\\Control\\FileSystem"_estr, L"LongPathsEnabled"_estr);
    auto const directory = basedir(exe_path());
    auto const free_bytes = free_space(L"");
    auto const dx_min = reg_read_num(HKLM, L"SOFTWARE\\Microsoft\\DirectX", L"MinFeatureLevel");
    auto const dx_max = reg_read_num(HKLM, L"SOFTWARE\\Microsoft\\DirectX", L"MaxFeatureLevel");

    wprintf(L"Windows version: %u.%u.%u [%hs]\n",
            ver_major,
            ver_minor,
            ver_build,
            // Anything before 2022 BAD: https://en.wikipedia.org/wiki/List_of_Microsoft_Windows_versions
            ver_build < 19045 || ver_build == 22000 ? REP_BAD : REP_OK);
    wprintf(L"Windows flags: 0x%08x\n", flags);
    wprintf(L"Windows lang: 0x%x 0x%x\n", GetUserDefaultLangID(), GetUserDefaultUILanguage());
    wprintf(L"CPU: %s\n", cpu.c_str());
    wprintf(L"DirectX feature level min(0x%x) max(0x%x)\n", dx_min, dx_max);
    wprintf(L"Long Path: %d\n", long_path);
    wprintf(L"Directory: %s: [%hs]\n", directory.c_str(), directory.size() > 128 ? REP_BAD : REP_OK);
    wprintf(L"Free space: %s: [%hs]\n", bytes_to_str(free_bytes).c_str(), free_bytes < GB ? REP_SUS : REP_OK);
}

static void check_patcher_signature(bool interactive) {
    auto const dll = basedir(exe_path()) + L"//cslol-dll.dll";
    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"Patcher dll: missing [%hs]\n", REP_SUS);
        return;
    }
    auto file = WINTRUST_FILE_INFO{
        .cbStruct = sizeof(WINTRUST_FILE_INFO),
        .pcwszFilePath = dll.c_str(),
    };
    auto trust = WINTRUST_DATA{
        .cbStruct = sizeof(WINTRUST_DATA),
        .dwUIChoice = WTD_UI_NONE,
        .dwUnionChoice = WTD_CHOICE_FILE,
        .pFile = &file,
        .dwStateAction = WTD_STATEACTION_VERIFY,
    };
    GUID guid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    HWND hwnd = (HWND)(interactive ? NULL : INVALID_HANDLE_VALUE);
    LONG result = WinVerifyTrust(hwnd, &guid, &trust);
    wprintf(L"Patcher dll: 0x%lx [%hs]\n", result, result != NO_ERROR ? REP_BAD : REP_OK);
}

static void check_compat_mode(bool fix) {
#define REG_COMPAT L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers"_estr

    auto const uac =
        reg_read_num(HKLM, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"_estr, L"EnableLUA"_estr);
    wprintf(L"UAC: %d [%hs]\n", (int)!!uac, !uac ? REP_BAD : REP_OK);

    for (auto const key : {HKCU, HKLM}) {
        for (auto const& path : reg_list(key, REG_COMPAT)) {
            if (auto const i = path.find_last_of(L"\\/"); i != std::wstring_view::npos) {
                auto const name = std::wstring_view{path}.substr(i + 1);
                for (auto const& bad : COMPAT_BAD) {
                    if (name.starts_with(bad)) {
                        const auto clean = fix ? RegDeleteKeyValueW(key, REG_COMPAT, path.c_str()) : -1;
                        wprintf(L"Compat: %s [%hs]\n", path.c_str(), clean == NO_ERROR ? REP_SUS : REP_BAD);
                        break;
                    }
                }
                for (auto const& bad : COMPAT_SUS) {
                    if (name.starts_with(bad)) {
                        wprintf(L"Compat: %s [%hs]\n", path.c_str(), REP_SUS);
                        break;
                    }
                }
            }
        }
    }
}

static void check_bootleg() {
    static HRESULT has_wmi = [] {
        HRESULT res = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
        if (res != NO_ERROR) return res;

        IWbemLocator* pLoc = 0;

        const CLSID CLSID_WbemLocator = {0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24};
        const IID IID_IWbemLocator = {0xdc12a687, 0x737f, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24};

        res = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
        if (res != NO_ERROR) return res;

        IWbemServices* pSvc = 0;
        res = pLoc->ConnectServer(SysAllocString(L"ROOT\\CimV2"_estr), 0, 0, 0, 0, 0, 0, &pSvc);
        if (res != NO_ERROR) return res;

        return NO_ERROR;
    }();
    wprintf(L"WMI: 0x%x [%hs]\n", has_wmi, has_wmi == NO_ERROR ? REP_OK : REP_SUS);
}

static void run_diag(bool interactive) {
    check_basic_info();
    check_patcher_signature(interactive);
    check_bootleg();
    check_compat_mode(interactive);
}

static bool spawn(bool admin = false) {
    auto const exe = exe_path();
    auto const dir = basedir(exe);
    auto const res =
        ShellExecuteW(NULL, admin ? L"runas"_estr : L"open"_estr, exe.c_str(), L"i", dir.c_str(), SW_SHOWDEFAULT);
    return (uintptr_t)res > 32;
}

static void wait_exit() {
    wprintf(L"Press enter to exit...");
    getchar();
}

static bool fuck_machine_learning() {
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);
    LARGE_INTEGER StartingTime;
    QueryPerformanceCounter(&StartingTime);

    const auto answer = MessageBoxW(NULL,
                                    L"Please wait for a second before answering. Are you a robot?",
                                    L"Note",
                                    MB_ICONQUESTION | MB_YESNO);

    LARGE_INTEGER EndingTime;
    QueryPerformanceCounter(&EndingTime);

    const auto timediff = ((EndingTime.QuadPart - StartingTime.QuadPart) * 1000000) / Frequency.QuadPart;

    if (answer != IDNO || timediff < 1000000) {
        MessageBoxW(NULL, L"Wrong aswer.", L"Wrong answer.", MB_ICONEXCLAMATION);
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    switch (argc < 2 ? '\0' : argv[1][0]) {
        case '\0':
            if (!fuck_machine_learning()) return 0;
            SetConsoleOutputCP(CP_UTF8);
            if (spawn(true)) return 0;
            [[fallthrough]];
        case 'i':
            SetConsoleOutputCP(CP_UTF8);
            run_diag(true);
            wait_exit();
            return 0;
        case 'e':
            SetConsoleOutputCP(CP_UTF8);
            if (spawn(true)) return 0;
            if (spawn(false)) return 0;
            return 0;
        case 'd':
            SetConsoleOutputCP(CP_UTF8);
            run_diag(false);
            return 0;
    }
}
