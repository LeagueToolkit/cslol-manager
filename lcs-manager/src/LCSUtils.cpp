#include "LCSUtils.h"
#include "lcs/common.hpp"
#include <filesystem>
#include <QUrl>
#include <QUrlQuery>
#include <QSysInfo>
#include <QCryptographicHash>

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

QString LCSUtils::statsUrl() {
    if (!LCS::STATS_HOST || !*LCS::STATS_HOST) {
        return "";
    }
    constexpr auto md5_hex = [] (QByteArray data) { return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex(); };
    QUrl result(QString("http://%1/add").arg(LCS::STATS_HOST));
    result.setQuery({
                        {"id", md5_hex(QSysInfo::machineUniqueId()) },
                        {"ver", LCS::VERSION},
                        {"kernel", QSysInfo::kernelType() + " " + QSysInfo::kernelVersion() },
                        {"os", QSysInfo::prettyProductName() },
                    });
    return result.toString();
}
