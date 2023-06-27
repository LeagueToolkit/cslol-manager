#include "CSLOLUtils.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QSysInfo>
#include <QUrl>
#include <QUrlQuery>

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

bool CSLOLUtils::isUnnecessaryAdmin() {
    return false;
    if (QFileInfo info(QCoreApplication::applicationDirPath() + "/admin_allow.txt"); info.exists()) {
        return false;
    }
    bool result = 0;
    HANDLE token = {};
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation = {};
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            result = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return result;
}
#elif defined(__APPLE__)
#    include <libproc.h>
#    include <string.h>

QString CSLOLUtils::detectGamePath() {
    pid_t *pid_list;

    //
    //  Get necessary size for `pid_list`
    //
    int n_pids = proc_listallpids(nullptr, 0);

    //
    //  Something went wrong trying
    //  to find all the processes.
    //
    if (n_pids <= 0) {
        return "";
    }

    pid_list = (pid_t *)malloc(n_pids * sizeof(pid_t));

    n_pids = proc_listallpids(pid_list, n_pids * sizeof(pid_t));

    for (int i = 0; i < n_pids; i++) {
        struct proc_bsdinfo proc;

        //
        //  Info about this PID.
        //
        int st = proc_pidinfo(pid_list[i], PROC_PIDTBSDINFO, 0, &proc, PROC_PIDTBSDINFO_SIZE);

        //
        //  We're looking for a LeagueClient PID.
        //
        if (strcmp(proc.pbi_name, "LeagueClient") == 0) {
            char game_path[PROC_PIDPATHINFO_MAXSIZE];

            //
            //  Find the parent path
            //  of the LeagueClient PID.
            //
            int len = proc_pidpath(proc.pbi_pid, game_path, sizeof(game_path));

            //
            //  Something went wrong.
            //
            if (len == 0)
                qDebug() << "UNABLE TO LOCATE PARENT PATH OF LeagueClient PID!";

            else {
                //
                //  `strtok()` will return the original
                //  string we passed in, but it'll be modified.
                //
                //  We'll have the first part of the “split”,
                //  in this case that's `/Applications/League of Legends`,
                //  so we need to add the rest of the path to it.
                //
                //  This modification will persist for `game_path`.
                //
                strcat(strtok(game_path, "."), ".app/Contents/LoL/Game");

                //
                //  Ensure that /Applications/League of Legends.app/Contents/LoL/Game
                //  is a valid directory.
                //
                if (QFileInfo info(game_path); info.isDir()) {
                    free(pid_list);

                    return game_path;
                }
            }
        }
    }

    free(pid_list);

    return "";
}

// TODO: macos implementation
bool CSLOLUtils::isUnnecessaryAdmin() { return false; }

#else
QString CSLOLUtils::detectGamePath() { return ""; }

bool CSLOLUtils::isUnnecessaryAdmin() { return false; }
#endif
