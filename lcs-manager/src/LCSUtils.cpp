#include "LCSUtils.h"
#include <filesystem>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

void LCSUtils::disableSS(QQuickWindow* window, bool disable) {
    auto const hwnd = (HWND)window->winId();
    SetWindowDisplayAffinity(hwnd, disable ? 1 : 0);
}

void LCSUtils::allowDD() {
    ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
    ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
    ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
//    auto const hwnd = (HWND)window->winId();
//    ChangeWindowMessageFilterEx(hwnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
//    ChangeWindowMessageFilterEx(hwnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
//    ChangeWindowMessageFilterEx(hwnd, 0x0049, MSGFLT_ALLOW, nullptr);
}
#else
void LCSUtils::disableSS(QQuickWindow* window, bool disable) {}
void LCSUtils::allowDD() {}
#endif


namespace fs = std::filesystem;

LCSUtils::LCSUtils(QObject *parent) : QObject(parent) {}

QString LCSUtils::fromFile(QString file) {
    if (file.isNull() || file.isEmpty()) {
        return "";
    }
    QUrl url = file;
    return url.toLocalFile();
}

QString LCSUtils::toFile(QString file) {
    if (file.isNull() || file.isEmpty()) {
        return "";
    }
    QUrl url = QUrl::fromLocalFile(file);
    return url.toString();
}


static QString try_game_path(fs::path path) {
    path = path.lexically_normal();
    if (!fs::exists(path / "League of Legends.exe") && !fs::exists(path / "League of Legends.app")) {
        path = "";
    }
    return QString::fromStdU16String(path.generic_u16string());
}

QString LCSUtils::checkGamePath(QString pathRaw) {
    if (pathRaw.isEmpty()) {
        return pathRaw;
    }
    fs::path path = pathRaw.toStdU16String();
    if (pathRaw = try_game_path(path); !pathRaw.isEmpty()) {
        return pathRaw;
    }
    if (pathRaw = try_game_path(path / "Game"); !pathRaw.isEmpty()) {
        return pathRaw;
    }
    if (pathRaw = try_game_path(path / ".."); !pathRaw.isEmpty()) {
        return pathRaw;
    }
    return "";
}
