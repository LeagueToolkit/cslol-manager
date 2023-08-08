#include "CSLOLToolsImpl.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMap>
#include <QMetaEnum>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QThread>
#include <QVersionNumber>
#include <fstream>

#include "CSLOLUtils.h"
#include "CSLOLVersion.h"
#ifdef _WIN32
#define MOD_TOOLS_EXE "/cslol-tools/mod-tools.exe"
#else
#define MOD_TOOLS_EXE "/cslol-tools/mod-tools"
#endif

CSLOLToolsImpl::CSLOLToolsImpl(QObject* parent) : QObject(parent), prog_(QCoreApplication::applicationDirPath()) {
    logFile_ = new QFile(prog_ + "/log.txt", this);
    logFile_->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
    logFile_->write("Version: " + QByteArray(CSLOL::VERSION) + "\n");
}

CSLOLToolsImpl::~CSLOLToolsImpl() {
    if (lockfile_) {
        delete lockfile_;
    }
}

CSLOLToolsImpl::CSLOLState CSLOLToolsImpl::getState() { return state_; }

void CSLOLToolsImpl::setState(CSLOLState value) {
    if (state_ != value) {
        state_ = value;
        emit stateChanged(value);
    }
}

void CSLOLToolsImpl::setStatus(QString status) {
    if (status_ != status) {
        logFile_->write((status.toUtf8() + "\n"));
        if (!status.startsWith("[WRN] ")) {
            status_ = status;
            emit statusChanged(status);
        }
    }
}

QString CSLOLToolsImpl::getLeaguePath() { return game_; }

/// util

static QJsonObject modInfoFixup(QString modName, QJsonObject object) {
    if (!object.contains("Name") || !object["Name"].isString() || object["Name"].toString().isEmpty()) {
        object["Name"] = modName;
    }
    if (!object.contains("Version") || !object["Version"].isString()) {
        object["Version"] = "0.0.0";
    }
    if (!object.contains("Author") || !object["Author"].isString()) {
        object["Author"] = "UNKNOWN";
    }
    if (!object.contains("Description") || !object["Description"].isString()) {
        object["Description"] = "";
    }
    if (!object.contains("Home") || !object["Home"].isString()) {
        object["Home"] = "";
    }
    if (!object.contains("Heart") || !object["Heart"].isString()) {
        object["Heart"] = "";
    }
    return object;
}

QStringList CSLOLToolsImpl::modList() {
    auto result = QStringList();
    for (auto it = QDirIterator(prog_ + "/installed", QDir::Dirs); it.hasNext();) {
        auto path = it.next();
        if (path.endsWith(".tmp")) continue;
        auto name = QFileInfo(path).fileName();
        if (name == "." || name == "..") continue;
        if (auto meta = QFileInfo(path + "/META/info.json"); !meta.exists()) continue;
        result.push_back(name);
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

QStringList CSLOLToolsImpl::modWadsList(QString modName) {
    auto result = QStringList();
    for (QDirIterator it(prog_ + "/installed/" + modName + "/WAD", {"*.wad.client"}, QDir::Files); it.hasNext();) {
        auto path = it.next();
        if (path.endsWith(".tmp")) continue;
        auto name = QFileInfo(path).fileName();
        result.push_back(name);
    }
    result.sort(Qt::CaseInsensitive);
    return result;
}

QJsonObject CSLOLToolsImpl::modInfoRead(QString modName) {
    auto data = QByteArray("{}", 2);
    if (QFile file(prog_ + "/installed/" + modName + "/META/info.json"); file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
    }
    QJsonParseError error;
    auto document = QJsonDocument::fromJson(data, &error);
    if (!document.isObject()) {
        return modInfoFixup(modName, QJsonObject());
    }
    return modInfoFixup(modName, document.object());
}

bool CSLOLToolsImpl::modInfoWrite(QString modName, QJsonObject object) {
    QDir(prog_ + "/installed/" + modName).mkpath("META");
    auto data = QJsonDocument(modInfoFixup(modName, object)).toJson();
    if (QFile file(prog_ + "/installed/" + modName + "/META/info.json"); file.open(QIODevice::WriteOnly)) {
        file.write(data);
        return true;
    }
    return false;
}

QString CSLOLToolsImpl::modImageGet(QString modName) {
    auto path = prog_ + "/installed/" + modName + "/META/image.png";
    if (QFileInfo info(prog_); !info.exists()) {
        return "";
    }
    return path;
}

QString CSLOLToolsImpl::modImageSet(QString modName, QString image) {
    QDir(prog_ + "/installed/" + modName).mkpath("META");
    auto path = prog_ + "/installed/" + modName + "/META/image.png";
    if (image.isEmpty()) {
        QFile::remove(path);
        return "";
    }
    if (path == image) return path;
    if (QFile src(image); src.open(QIODevice::ReadOnly)) {
        if (QFile dst(path); dst.open(QIODevice::WriteOnly)) {
            dst.write(src.readAll());
            return path;
        }
    }
    return "";
}

QStringList CSLOLToolsImpl::listProfiles() {
    if (QDir dir(prog_); !dir.exists()) {
        dir.mkpath("profiles");
    }
    QStringList profiles;
    for (QDirIterator it(prog_ + "/profiles", QDir::Dirs); it.hasNext();) {
        auto info = QFileInfo(it.next());
        auto name = info.fileName();
        if (name == "." || name == "..") continue;
        profiles.push_back(name);
    }
    if (!profiles.contains("Default Profile")) {
        profiles.push_front("Default Profile");
    }
    return profiles;
}

QJsonObject CSLOLToolsImpl::readProfile(QString profileName) {
    QJsonObject profile;
    auto data = QString("");
    if (QFile file(prog_ + "/profiles/" + profileName + ".profile"); file.open(QIODevice::ReadOnly)) {
        data = QString::fromUtf8(file.readAll());
    }
    for (auto line : data.split('\n', Qt::SkipEmptyParts)) {
        profile.insert(line.remove('\n'), true);
    }
    return profile;
}

void CSLOLToolsImpl::writeProfile(QString profileName, QJsonObject profile) {
    QDir profilesDir(prog_);
    profilesDir.mkpath("profiles");
    if (QFile file(prog_ + "/profiles/" + profileName + ".profile"); file.open(QIODevice::WriteOnly)) {
        for (auto mod : profile.keys()) {
            auto data = mod.toUtf8();
            if (data.size() == 0) {
                continue;
            }
            data.push_back('\n');
            file.write(data);
        }
    }
}

QString CSLOLToolsImpl::readCurrentProfile() {
    auto data = QString("");
    if (QFile file(prog_ + "/current.profile"); file.open(QIODevice::ReadOnly)) {
        data = QString::fromUtf8(file.readAll()).remove('\n');
    }
    if (data.isEmpty()) {
        data = "Default Profile";
    }
    return data;
}

void CSLOLToolsImpl::writeCurrentProfile(QString profile) {
    if (QFile file(prog_ + "/current.profile"); file.open(QIODevice::WriteOnly)) {
        auto data = profile.toUtf8();
        data.push_back('\n');
        file.write(data);
    }
}

void CSLOLToolsImpl::doReportError(QString name, QString message, QString trace) {
    if (!name.isEmpty()) {
        logFile_->write("Error while: " + name.toUtf8() + "\n");
    }
    if (!message.isEmpty()) {
        logFile_->write(message.toUtf8() + "\n");
    }
    if (!trace.isEmpty()) {
        logFile_->write(trace.toUtf8() + "\n");
    }
    if (message.contains("OpenProcess: ") || trace.contains("OpenProcess: ")) {
        QFile file(prog_ + "/admin_allowed.txt");
        file.open(QIODevice::WriteOnly);
        file.close();
        trace += '\n';
        trace += ">>Run as administrator<< is now enabled!";
    }
    emit reportError(name, message, trace);
}

/// impl

void CSLOLToolsImpl::changeLeaguePath(QString newLeaguePath) {
    if (state_ == CSLOLState::StateIdle || state_ == CSLOLState::StateUnitialized) {
        if (state_ != CSLOLState::StateUnitialized) {
            setState(CSLOLState::StateBusy);
            setStatus("Change League Path");
        }

        if (auto info = QFileInfo(newLeaguePath + "/League of Legends.exe"); info.exists()) {
            newLeaguePath = info.canonicalPath();
        }
        if (auto info = QFileInfo(newLeaguePath + "/LeagueofLegends.app"); info.exists()) {
            newLeaguePath = info.canonicalPath();
        }
        if (game_ != newLeaguePath) {
            game_ = newLeaguePath;
            emit leaguePathChanged(newLeaguePath);
        }

        if (state_ != CSLOLState::StateUnitialized) {
            setState(CSLOLState::StateIdle);
        }
    }
}

void CSLOLToolsImpl::changeBlacklist(bool blacklist) {
    if (blacklist_ != blacklist) {
        blacklist_ = blacklist;
        emit blacklistChanged(blacklist);
    }
}

void CSLOLToolsImpl::changeIgnorebad(bool ignorebad) {
    if (ignorebad_ != ignorebad) {
        ignorebad_ = ignorebad;
        emit ignorebadChanged(ignorebad);
    }
}

void CSLOLToolsImpl::init() {
    if (state_ == CSLOLState::StateUnitialized) {
        setState(CSLOLState::StateBusy);

        if (QString error = CSLOLUtils::isPlatformUnsuported(); !error.isEmpty()) {
            doReportError("Unsupported platform",
                          error,
                          "Application launched on unsupported machine or configuration.");
            setState(CSLOLState::StateCriticalError);
            return;
        }

        patcherProcess_ = nullptr;
        setStatus("Acquire lock");
        lockfile_ = new QLockFile(prog_ + "/lockfile");
        if (!lockfile_->tryLock()) {
            auto lockerror = QString::number((int)lockfile_->error());
            doReportError("Acquire lock", "Can not run multiple instances", lockerror);
            setState(CSLOLState::StateCriticalError);
            return;
        }

        setStatus("Check mod-tools");
        if (QFileInfo modtools(prog_ + MOD_TOOLS_EXE); !modtools.exists()) {
            doReportError("Check mod-tools",
                          "Make sure you installed properly and that anti-virus isn't blocking any executables.",
                          "cslol-tools/mod-tools.exe is missing");
            setState(CSLOLState::StateCriticalError);
            return;
        }

        setStatus("Load mods");
        QJsonObject mods;
        for (auto name : modList()) {
            auto info = modInfoRead(name);
            mods.insert(name, info);
        }

        setStatus("Load profiles");
        auto profiles = listProfiles();
        auto profileName = readCurrentProfile();
        if (!profiles.contains(profileName)) {
            profileName = "Default Profile";
            writeCurrentProfile(profileName);
        }

        setStatus("Read profile");
        auto profileMods = readProfile(profileName);

        emit initialized(mods, QJsonArray::fromStringList(profiles), profileName, profileMods);

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::deleteMod(QString name) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Delete mod");
        if (QDir dir(prog_ + "/installed/" + name); dir.removeRecursively()) {
            emit modDeleted(name);
        }

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::exportMod(QString name, QString dest) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Export mod");
        runTool(
            {
                "export",
                prog_ + "/installed/" + name,
                dest,
                "--game:" + game_,
                blacklist_ ? "--noTFT" : "",
            },
            [this](int code, QProcess* process) { setState(CSLOLState::StateIdle); });
    }
}

void CSLOLToolsImpl::installFantomeZip(QString path) {
    if (state_ == CSLOLState::StateIdle && !path.isEmpty()) {
        setState(CSLOLState::StateBusy);

        setStatus("Installing Mod");
        auto name = QFileInfo(path)
                        .fileName()
                        .replace(".zip", "")
                        .replace(".fantome", "")
                        .replace(".wad", "")
                        .replace(".client", "");
        auto dst = prog_ + "/installed/" + name;
        if (QDir old(dst); old.exists()) {
            auto info = modInfoRead(name);
            doReportError("Install mod", "Already exists", "");
            setState(CSLOLState::StateIdle);
            return;
        }

        runTool(
            {
                "import",
                path,
                dst,
                "--game:" + game_,
                blacklist_ ? "--noTFT" : "",
            },
            [=, this](int code, QProcess* process) {
                if (code == 0) {
                    auto info = modInfoRead(name);
                    emit installedMod(name, info);
                }
                setState(CSLOLState::StateIdle);
            });
    }
}

void CSLOLToolsImpl::makeMod(QString fileName, QJsonObject infoData, QString image) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Make mod");
        if (!modInfoWrite(fileName, infoData)) {
            doReportError("Make mod", "Failed to write mod info", "");
        } else {
            infoData = modInfoFixup(fileName, infoData);
            image = modImageSet(fileName, image);
            emit modCreated(fileName, infoData, image);
        }

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::refreshMods() {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        QJsonObject mods;
        for (auto name : modList()) {
            auto info = modInfoRead(name);
            mods.insert(name, info);
        }
        emit refreshed(mods);

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::doUpdate(QString urls) {
    if (!networkManager_) {
        this->networkManager_ = new QNetworkAccessManager(this);
        this->networkManager_->setTransferTimeout(30000);
        connect(this->networkManager_, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
            if (reply->error()) {
                auto error = reply->errorString();
                setStatus("[WRN] Failed to fetch update index: " + error);
                this->networkResults_.push_back(QJsonDocument::fromJson("[]"));
            } else {
                auto result = reply->readAll();
                this->networkResults_.push_back(QJsonDocument::fromJson(result));
            }

            if (this->networkResults_.size() < this->networkRequests_.size()) {
                return;
            }

            auto lookup = QMap<QString, QVersionNumber>{};
            for (auto name : modList()) {
                auto info = modInfoRead(name);
                auto key = info["Name"].toString().toLower();
                auto version = QVersionNumber::fromString(info["Version"].toString());
                lookup[key] = version;
            }

            auto mods = QJsonArray{};
            for (auto const& result : this->networkResults_) {
                if (!result.isArray()) {
                    continue;
                }
                for (auto const& value : result.array()) {
                    auto info = modInfoFixup("", value.toObject());
                    if (info["Download"].toString().isEmpty() || !info["Download"].isString()) {
                        info["Download"] = info["Home"];
                    }
                    auto key = info["Name"].toString().toLower();
                    if (lookup.contains(key)) {
                        auto const& old_version = lookup[key];
                        auto new_version = QVersionNumber::fromString(info["Version"].toString());
                        if (old_version < new_version) {
                            mods.append(info);
                        }
                    }
                }
            }

            emit updatedMods(mods);

            setState(CSLOLState::StateIdle);
        });
    }

    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);
        setStatus("Updating mods");

        this->networkResults_.clear();
        this->networkRequests_.clear();
        for (auto url : urls.split('\n')) {
            url = url.trimmed();
            if (url.isEmpty() || url.startsWith("#")) {
                continue;
            }
            this->networkRequests_.emplace_back(QUrl(url));
        }

        if (this->networkRequests_.empty()) {
            doReportError("Update mods", "Make sure to set update urls in settings!", "");
            setState(CSLOLState::StateIdle);
            return;
        }

        for (auto& req : this->networkRequests_) {
            this->networkManager_->get(req);
        }
    }
}

void CSLOLToolsImpl::saveProfile(QString name, QJsonObject mods, bool run, bool skipConflict, bool experimentalConfigLess) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Save profile");
        if (name.isEmpty() || name.isNull()) {
            name = "Default Profile";
        }
        writeCurrentProfile(name);
        writeProfile(name, mods);
        emit profileSaved(name, mods);

        setStatus("Write profile");
        runTool(
            {
                "mkoverlay",
                prog_ + "/installed",
                prog_ + "/profiles/" + name,
                "--game:" + game_,
                "--mods:" + mods.keys().join('/'),
                blacklist_ ? "--noTFT" : "",
                skipConflict ? "--ignoreConflict" : "",
            },
            [=, this](int code, QProcess* process) {
                if (run && code == 0) {
                    setStatus("Starting patcher...");
                    runPatcher({
                        "runoverlay",
                        prog_ + "/profiles/" + name,
                        prog_ + "/profiles/" + name + ".config",
                        "--game:" + game_,
                        "--opts:" + QString(experimentalConfigLess ? "configless" : "none"),
                    });
                } else {
                    setState(CSLOLState::StateIdle);
                }
            });
    }
}

void CSLOLToolsImpl::loadProfile(QString name) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Save profile");
        if (name.isEmpty() || name.isNull()) {
            name = "Default Profile";
        }
        auto profileMods = readProfile(name);
        emit profileLoaded(name, profileMods);

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::deleteProfile(QString name) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Delete profile");
        if (QDir dir(prog_ + "/profiles/" + name); dir.removeRecursively()) {
            emit profileDeleted(name);
        }

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::stopProfile() {
    if (state_ == CSLOLState::StateRunning) {
        // setState(CSLOLState::StateStoping);
        if (patcherProcess_ != nullptr) {
            patcherProcess_->write("\n");
        }
    }
}

void CSLOLToolsImpl::startEditMod(QString fileName) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Edit mod");
        auto info = modInfoRead(fileName);
        auto image = modImageGet(fileName);
        auto wads = modWadsList(fileName);
        emit modEditStarted(fileName, info, image, QJsonArray::fromStringList(wads));

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::changeModInfo(QString fileName, QJsonObject infoData, QString image) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Change mod info");
        if (!modInfoWrite(fileName, infoData)) {
            doReportError("Change mod info", "Failed to write mod info", "");
        } else {
            infoData = modInfoFixup(fileName, infoData);
            image = modImageSet(fileName, image);
            emit modInfoChanged(fileName, infoData, image);
        }

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::removeModWads(QString fileName, QJsonArray wads) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Remove mod wads");
        auto result = QStringList();
        for (auto wadName : wads) {
            auto name = wadName.toString();
            if (QFile::remove(prog_ + "/installed/" + fileName + "/WAD/" + name)) {
                result.push_back(name);
            }
        }
        emit modWadsRemoved(fileName, QJsonArray::fromStringList(result));

        setState(CSLOLState::StateIdle);
    }
}

void CSLOLToolsImpl::addModWad(QString fileName, QString wad, bool removeUnknownNames) {
    if (state_ == CSLOLState::StateIdle) {
        setState(CSLOLState::StateBusy);

        setStatus("Add mod wads");
        auto before = modWadsList(fileName);
        runTool(
            {
                "addwad",
                wad,
                prog_ + "/installed/" + fileName,
                "--game:" + game_,
                removeUnknownNames ? "--removeUNK" : "",
                blacklist_ ? "--noTFT" : "",
            },
            [=, this](int code, QProcess* process) {
                if (code == 0) {
                    auto after = modWadsList(fileName);
                    auto done = QStringList();
                    for (auto wad : after) {
                        if (!before.contains(wad, Qt::CaseInsensitive)) {
                            done.push_back(wad);
                        }
                    }
                    emit modWadsAdded(fileName, QJsonArray::fromStringList(done));
                }
                setState(CSLOLState::StateIdle);
            });
    }
}

void CSLOLToolsImpl::runTool(QStringList args, std::function<void(int code, QProcess*)> handle) {
    auto process = new QProcess(this);
    connect(process, &QProcess::readyReadStandardOutput, this, [=, this]() {
        process->setReadChannel(QProcess::ProcessChannel::StandardOutput);
        while (process->canReadLine()) {
            auto line = process->readLine();
            setStatus(line.trimmed());
        }
    });
    connect(process,
            static_cast<void (QProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&QProcess::finished),
            this,
            [=, this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode != 0) {
                    QString trace = process->readAllStandardError().trimmed();
                    doReportError("Run mod-tools", trace.split('\n').last(), trace);
                }
                handle(exitCode, process);
                process->deleteLater();
            });
    connect(process, &QProcess::errorOccurred, this, [=, this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            QString message = process->errorString();
            if (QFileInfo pathInfo(process->program()); !pathInfo.exists()) {
                message = "Make sure to install properly and that anti-virus isn't blocking executable.";
            }
            QString trace = "arguments:\n  " + args.join("\n  ").replace('\\', '/') + "\n";
            doReportError("Run mod-tools", message, trace);
            handle(-1, process);
            process->deleteLater();
        }
    });
    process->start(prog_ + MOD_TOOLS_EXE, args);
}

void CSLOLToolsImpl::runPatcher(QStringList args) {
    if (patcherProcess_ == nullptr) {
        auto process = patcherProcess_ = new QProcess(this);
        connect(process, &QProcess::readyReadStandardOutput, this, [=, this]() {
            process->setReadChannel(QProcess::ProcessChannel::StandardOutput);
            while (process->canReadLine()) {
                auto line = process->readLine();
                setStatus(line.trimmed());
            }
        });
        connect(process, &QProcess::started, this, [this] { setState(CSLOLState::StateRunning); });
        connect(process,
                static_cast<void (QProcess::*)(int exitCode, QProcess::ExitStatus exitStatus)>(&QProcess::finished),
                this,
                [=, this](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitCode != 0) {
                        QString trace = process->readAllStandardError().trimmed();
                        doReportError("Run mod-tools", trace.split('\n').last(), trace);
                    }
                    setState(CSLOLState::StateIdle);
                });
        connect(process, &QProcess::errorOccurred, this, [=, this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                QString message = process->errorString();
                if (QFileInfo pathInfo(process->program()); !pathInfo.exists()) {
                    message = "Make sure to install properly and that anti-virus isn't blocking executable.";
                }
                QString trace = "arguments:\n  " + args.join("\n  ").replace('\\', '/') + "\n";
                doReportError("Run mod-tools", message, trace);
                setState(CSLOLState::StateIdle);
            }
        });
    }
    patcherProcess_->start(prog_ + MOD_TOOLS_EXE, args);
}
