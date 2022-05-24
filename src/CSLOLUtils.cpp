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
    if (auto info = QFileInfo(path + "/League of Legends.app"); info.exists()) {
        return info.canonicalPath();
    }
    return "";
}

QString CSLOLUtils::checkGamePath(QString pathRaw) {
    if (pathRaw.isEmpty()) {
        return pathRaw;
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
    return "";
}
