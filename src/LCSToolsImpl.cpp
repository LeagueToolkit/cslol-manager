#include "LCSToolsImpl.h"
#include "lcs/wadindex.hpp"
#include "lcs/wadmerge.hpp"
#include "lcs/wadmergequeue.hpp"
#include "lcs/wadmakequeue.hpp"
#include "lcs/modindex.hpp"
#include "lcs/mod.hpp"
#include <QDebug>
#include <QMetaEnum>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QThread>

LCSToolsImpl::LCSToolsImpl(QObject *parent) :
    QObject(parent),
    process_(nullptr),
    progDirPath_(QCoreApplication::applicationDirPath().toStdString())
{}

LCSToolsImpl::~LCSToolsImpl() {}

LCSToolsImpl::LCSState LCSToolsImpl::getState() {
    return state_;
}

void LCSToolsImpl::setState(LCSState value) {
    if (state_ != value) {
        state_ = value;
        emit stateChanged(value);
    }
}

void LCSToolsImpl::setStatus(QString status) {
    if (status_ != status) {
        status_ = status;
        emit statusChanged(status);
    }
}

QString LCSToolsImpl::getLeaguePath() {
    return leaguepath_;
}

/// util
namespace {
    QJsonObject validateAndCorrect(QString fileName, QJsonObject object) {
        if (!object.contains("Name") || !object["Name"].isString() || object["Name"].toString().isEmpty()) {
            object["Name"] = fileName;
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
        return object;
    }

    QJsonObject parseInfoData(QString fileName, std::string const& str) {
        QByteArray data;
        data.append(str.data(), (int)str.size());
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(data, &error);
        if (!document.isObject()) {
            return validateAndCorrect(fileName, QJsonObject());
        }
        return validateAndCorrect(fileName, document.object());
    }

    QJsonObject parseInfoData(std::string const& fileName, std::string const& str) {
        return parseInfoData(QString::fromStdString(fileName), str);
    }

    std::string dumpInfoData(QJsonObject info) {
        QJsonDocument document(info);
        return document.toJson().toStdString();
    }
}

/// impl

void LCSToolsImpl::changeLeaguePath(QString newLeaguePath) {
    if (state_ == LCSState::StateIdle || state_ == LCSState::StateUnitialized) {
        if (state_ != LCSState::StateUnitialized) {
            setState(LCSState::StateBussy);
            setStatus("Change League Path");
        }
        if (newLeaguePath.isEmpty() || newLeaguePath.isNull()) {
            return;
        }
        LCS::fs::path path = newLeaguePath.toStdString();
        if (path.extension() == ".exe") {
            path.remove_filename();
        }
        if (LCS::fs::exists(path / "Game" / "League of Legends.exe")) {
            path /= "Game";
        }
        if (LCS::fs::exists(path / "League of Legends" / "Game" / "League of Legends.exe")) {
            path /= "League of Legends";
            path /= "Game";
        }
        if (LCS::fs::exists(path / "League of Legends.exe")) {
            newLeaguePath = QString::fromStdString(path.generic_string());
            if (newLeaguePath != leaguepath_) {
                leaguePathStd_ = path;
                leaguepath_ = newLeaguePath;
                emit leaguePathChanged(leaguepath_);
            }
        } else {
            emit reportWarning("Change League Path", "League of Legends.exe could not be found!");
        }
        if (state_ != LCSState::StateUnitialized) {
            setState(LCSState::StateIdle);
        }
    }
}

void LCSToolsImpl::init(QJsonObject savedProfiles) {
    if (state_ == LCSState::StateUnitialized) {
        setState(LCSState::StateBussy);
        setStatus("Load mods");
        try {
            modIndex_ = std::make_unique<LCS::ModIndex>(progDirPath_ / "installed");
            QJsonObject mods;
            for(auto const& [rawFileName, rawMod]: modIndex_->mods()) {
                mods.insert(QString::fromStdString(rawFileName),
                            parseInfoData(rawFileName, rawMod->info()));
            }
            try {
                for(auto const& entry: LCS::fs::directory_iterator(progDirPath_ / "profiles")) {
                    auto path = entry.path();
                    auto name = QString::fromStdString(path.filename().generic_string());
                    if (!savedProfiles.contains(name)) {
                        //LCS::fs::remove_all(entry.path());
                    }
                }
            } catch(std::runtime_error const& error) {
                emit reportWarning("Clean Profiles", QString(error.what()));
            }
            emit initialized(mods, savedProfiles);
            setState(LCSState::StateIdle);
        } catch(std::runtime_error const& error) {
            emit reportError("Load mods", QString(error.what()));
            setState(LCSState::StateCriticalError);
        }
    }
}

void LCSToolsImpl::deleteMod(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Delete mod");
        try {
            modIndex_->remove(name.toStdString());
            emit modDeleted(name);
        } catch(std::runtime_error const& error) {
            emit reportError("", QString(error.what()));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::exportMod(QString name, QString dest) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Export mod");
        try {
            modIndex_->export_zip(name.toStdString(), dest.toStdString(), *this);
        } catch(std::runtime_error const& error) {
            emit reportError("Export mod", QString(error.what()));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::installFantomeZip(QString path) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Install Fantome .zip");
        try {
            if (leaguepath_.isNull() || leaguepath_.isEmpty()) {
                throw std::runtime_error("League path not set!");
            }
            auto mod = modIndex_->install_from_zip(path.toStdString(), *this);
            emit installedMod(QString::fromStdString(mod->filename()),
                              parseInfoData(mod->filename(), mod->info()));
        } catch(std::runtime_error const& error) {
            emit reportError("Install Fantome .zip", QString(error.what()));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::makeMod(QString fileName, QString image, QJsonObject infoData, QJsonArray items) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Make mod");
        try {
            if (leaguepath_.isNull() || leaguepath_.isEmpty()) {
                throw std::runtime_error("League path not set!");
            }
            LCS::WadIndex index(leaguePathStd_);
            LCS::WadMakeQueue queue(index);
            for(auto item: items) {
                queue.addItem(LCS::fs::path(item.toString().toStdString()), LCS::Conflict::Abort);
            }
            infoData = validateAndCorrect(fileName, infoData);
            auto mod = modIndex_->make(fileName.toStdString(),
                                       dumpInfoData(infoData),
                                       image.toStdString(),
                                       queue,
                                       *this);
            emit installedMod(QString::fromStdString(mod->filename()),
                              infoData);
        } catch(std::runtime_error const& error) {
            emit reportError("Make mod", QString(error.what()));
        }
        setState(LCSState::StateIdle);
    }
}


void LCSToolsImpl::saveProfile(QString name, QJsonObject mods, bool run, bool whole) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Save profile");
        try {
            if (leaguepath_.isNull() || leaguepath_.isEmpty()) {
                throw std::runtime_error("League path not set!");
            }
            LCS::WadIndex index(leaguePathStd_);
            LCS::WadMergeQueue queue(progDirPath_ / "profiles" / name.toStdString(), index);
            for (auto m = mods.begin(); m != mods.end(); m++) {
                if (!m.value().toBool()) {
                    continue;
                }
                auto fileName = m.key().toStdString();
                if (auto i = modIndex_->mods().find(fileName); i != modIndex_->mods().end()) {
                    queue.addMod(i->second.get(), LCS::Conflict::Abort);
                }
            }
            if (whole) {
                queue.write_whole(*this);
            } else {
                queue.write(*this);
            }
            queue.cleanup();
            emit profileSaved(name, mods);
        } catch(std::runtime_error const& error) {
            emit reportError("Save profile", QString(error.what()));
        }
        setState(LCSState::StateIdle);
    }
    if (run) {
        emit runProfile(name);
    }
}

void LCSToolsImpl::deleteProfile(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Delete profile");
        try {
            LCS::fs::remove_all(progDirPath_ / "profiles" / name.toStdString());
            emit profileDeleted(name);
        } catch(std::runtime_error const& error) {
            emit reportError("Delete Profile", QString(error.what()));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::runProfile(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Run profile");
        if (leaguepath_.isNull() || leaguepath_.isEmpty()) {
            emit reportError("Run profile", "League Path not set");
            setState(LCSState::StateIdle);
        } else {
            auto exePath = progDirPath_ / "lolcustomskin.exe";
            if (!LCS::fs::exists(exePath)) {
                emit reportError("Run profile", "lolcustomskin.exe not found!");
                setState(LCSState::StateIdle);
                return;
            }

            if (!LCS::fs::is_regular_file(exePath)) {
                emit reportError("Run profile", "lolcustomskin.exe is not a file!");
                setState(LCSState::StateIdle);
                return;
            }

            auto profilePath = progDirPath_ / "profiles" / name.toStdString();
            auto pathName = QString::fromStdString(profilePath.generic_string());
            auto exeName = QString::fromStdString(exePath.generic_string());

            if (!process_) {
                process_ = new QProcess(this);
                process_->moveToThread(this->thread());
                connect(process_, &QProcess::readyReadStandardOutput, this, &LCSToolsImpl::readyReadStandardOutput);
                connect(process_, &QProcess::errorOccurred, this, &LCSToolsImpl::errorOccured);
                connect(process_, QOverload<int>::of(&QProcess::finished), this, &LCSToolsImpl::finished);
                connect(process_, &QProcess::started, this, &LCSToolsImpl::started);
                connect(this, &LCSToolsImpl::destroyed, process_, &QProcess::kill);
            }

            process_->start(exeName, QStringList({
                                                     pathName
                                                     //, "--exit-error"
                                                 }), QIODevice::OpenModeFlag::ReadOnly);
        }
    }
}

void LCSToolsImpl::stopProfile() {
    if (state_ == LCSState::StateExternalRunning) {
        if (process_->isOpen()){
            setStatus("Stop profile");
            process_->kill();
        }
    }
}


void LCSToolsImpl::startEditMod(QString fileName) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Edit mod");
        try {
            std::string nameStd = fileName.toStdString();
            if (auto i = modIndex_->mods().find(nameStd); i != modIndex_->mods().end()) {
                auto const& mod = i->second;
                QJsonArray wads;
                for(auto const& [name, wad]: mod->wads()) {
                    wads.push_back(QString::fromStdString(name));
                }
                emit modEditStarted(fileName,
                                    parseInfoData(fileName, mod->info()),
                                    QString::fromStdString(mod->image().generic_string()),
                                    wads);
            } else {
                throw std::runtime_error("No such mod found!");
            }
        } catch(std::runtime_error error) {
            emit reportError("Edit mod", error.what());
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::changeModInfo(QString fileName, QJsonObject infoData) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Change mod info");
        try {
            infoData = validateAndCorrect(fileName, infoData);
            modIndex_->change_mod_info(fileName.toStdString(), dumpInfoData(infoData));
            emit modInfoChanged(fileName, infoData);
        } catch(std::runtime_error error) {
            emit emit reportError("Change mod info", error.what());
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::changeModImage(QString fileName, QString image) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Change mod image");
        try {
            modIndex_->change_mod_image(fileName.toStdString(), image.toStdString());
            emit modImageChanged(fileName, image);
        } catch(std::runtime_error error) {
            emit reportWarning("Change mod image", error.what());
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::removeModImage(QString fileName) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Remove mod image");
        try {
            modIndex_->remove_mod_image(fileName.toStdString());
            emit modImageRemoved(fileName);
        } catch(std::runtime_error error) {
            emit reportWarning("Remove mod image", error.what());
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::removeModWads(QString fileName, QJsonArray wads) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Remove mod wads");
        try {
            for(auto wadName: wads) {
                modIndex_->remove_mod_wad(fileName.toStdString(), wadName.toString().toStdString());
            }
            emit modWadsRemoved(fileName, wads);
        } catch(std::runtime_error error) {
            emit reportError("Remove mod image", error.what());
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::addModWads(QString fileName, QJsonArray wads) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Add mod wads");
        try {
            if (leaguepath_.isNull() || leaguepath_.isEmpty()) {
                throw std::runtime_error("League path not set!");
            }
            LCS::WadIndex wadIndex(leaguePathStd_);
            LCS::WadMakeQueue wadMake(wadIndex);
            for(auto wadPath: wads) {
                wadMake.addItem(LCS::fs::path(wadPath.toString().toStdString()), LCS::Conflict::Abort);
            }
            auto added = modIndex_->add_mod_wads(fileName.toStdString(), wadMake, *this, LCS::Conflict::Abort);
            QJsonArray names;
            for(auto const& wad: added) {
                names.push_back(QString::fromStdString(wad->name()));
            }
            emit modWadsAdded(fileName, names);
        } catch(std::runtime_error error) {
            emit reportError("Add mod wads", error.what());
        }
        setState(LCSState::StateIdle);
    }
}



/// Private slots

void LCSToolsImpl::readyReadStandardOutput() {
    auto data = QString::fromUtf8(process_->readAllStandardOutput());
    for(auto line: data.split("\n")) {
        if (line.startsWith("ERROR: ")) {
            process_->kill();
        } else if(line.startsWith("INFO: ")) {
        } else {
            emit updateProfileStatus(line);
        }
    }
}

void LCSToolsImpl::errorOccured(QProcess::ProcessError error) {
    if (error == QProcess::ProcessError::FailedToStart) {
        emit reportError("Run profile", "Failed to start lolcustomskin.exe");
        setState(LCSState::StateIdle);
    } else if (error != QProcess::ProcessError::Crashed) {
        emit reportError("Run profile", "Unknown error in lolcustomskin.exe");
    }
    setState(LCSState::StateIdle);
}

void LCSToolsImpl::finished(int) {
    setState(LCSState::StateIdle);
}

void LCSToolsImpl::started() {
    setState(LCSState::StateExternalRunning);
}

/// Interface implementations
void LCSToolsImpl::startItem(LCS::fs::path const& path, size_t bytes) noexcept {
    auto name = QString::fromStdString(path.filename().generic_string());
    auto size = QString::number(bytes / 1024.0 / 1024.0, 'f', 2);
    setStatus("Processing " + name + "(" + size + "MB)");
}

void LCSToolsImpl::consumeData(size_t ammount) noexcept {
    progressDataDone_ += ammount;
    emit progressData((quint64)progressDataDone_);
}

void LCSToolsImpl::finishItem() noexcept {
    progressItemDone_++;
    emit progressItems((quint32)progressItemDone_);
}

void LCSToolsImpl::startMulti(size_t itemCount, size_t dataTotal) noexcept {
    progressItemDone_ = 0;
    progressDataDone_ = 0;
    emit progressStart((quint32)itemCount, (quint64)dataTotal);
}

void LCSToolsImpl::finishMulti() noexcept {
    emit progressEnd();
}
