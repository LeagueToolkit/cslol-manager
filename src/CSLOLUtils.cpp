#include <QCryptographicHash>
#include <QFileInfo>
#include <QSysInfo>
#include <QUrl>
#include <QUrlQuery>

#include "CSLOLUtils.h"
#include "CSLOLVersion.h"

CSLOLUtils::CSLOLUtils(QObject *parent) : QObject(parent) {}

QString CSLOLUtils::fromFile(QString file) {
    if (file.isNull() || file.isEmpty()) {
        return "";
    }
    QUrl url = file;
    return url.toLocalFile();
}

QString CSLOLUtils::toFile(QString file) {
    if (file.isNull() || file.isEmpty()) {
        return "";
    }
    QUrl url = QUrl::fromLocalFile(file);
    return url.toString();
}

static QString try_game_path(QString path) {
    if (auto info = QFileInfo(path + "/League of Legends.exe"); info.exists()) {
        return info.canonicalPath();
    }
    if (auto info = QFileInfo(path + "/LeagueofLegends.app"); info.exists()) {
        return info.canonicalPath();
    }
    return "";
}

QString CSLOLUtils::checkGamePath(QString pathRaw) {
    if (pathRaw.isEmpty()) {
        return pathRaw;
    }
    if (auto result = try_game_path(pathRaw + "/Contents/LoL/Game"); !result.isEmpty()) {
        return result;
    }
    if (auto result = try_game_path(pathRaw); !result.isEmpty()) {
        return result;
    }
    if (auto result = try_game_path(pathRaw + "/Game"); !result.isEmpty()) {
        return result;
    }
    if (auto result = try_game_path(pathRaw + "/.."); !result.isEmpty()) {
        return result;
    }
    if (auto result = try_game_path(pathRaw + "/../Game"); !result.isEmpty()) {
        return result;
    }
    return "";
}

#ifdef _WIN32
// do not reorder
#    define WIN32_LEAN_AND_MEAN
// do not reorder
#    include <windows.h>
// do not reorder
#    include <process.h>
#    include <psapi.h>
#    include <tlhelp32.h>
QString CSLOLUtils::detectGamePath() {
    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == NULL || snapshot == INVALID_HANDLE_VALUE) {
        return "";
    }

    auto entry = PROCESSENTRY32W{.dwSize = sizeof(PROCESSENTRY32W)};
    for (bool i = Process32FirstW(snapshot, &entry); i; i = Process32NextW(snapshot, &entry)) {
        auto process_name = std::wstring_view{entry.szExeFile};
        if (process_name != L"LeagueClient.exe" && process_name != L"League of Legends.exe") {
            continue;
        }

        auto handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, entry.th32ProcessID);
        if (handle == NULL || handle == INVALID_HANDLE_VALUE) {
            continue;
        }

        wchar_t buffer[260];
        DWORD size = 32767;
        if (QueryFullProcessImageNameW(handle, 0, buffer, &size) != 0) {
            if (auto result = checkGamePath(QString::fromWCharArray(buffer, size)); !result.isEmpty()) {
                CloseHandle(handle);
                return result;
            }
        }

        CloseHandle(handle);
    }

    CloseHandle(snapshot);
    return "";
}
#else
// TODO: macos implementation
QString CSLOLUtils::detectGamePath() { return ""; }
#endif
