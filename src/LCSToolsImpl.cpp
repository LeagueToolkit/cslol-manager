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
    progDirPath_(QCoreApplication::applicationDirPath().toStdString())
{
    patcher_.configpath = (progDirPath_ / "lolcustomskin.txt").generic_string();
}

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
QJsonArray LCSToolsImpl::listProfiles() {
    QJsonArray profiles;
    auto profilesPath = progDirPath_ / "profiles";
    if (!LCS::fs::exists(profilesPath)) {
        LCS::fs::create_directories(profilesPath);
    }
    for(auto const& entry: LCS::fs::directory_iterator(profilesPath)) {
        auto path = entry.path();
        if (entry.is_directory() && !LCS::fs::exists(path.generic_string() + ".profile")) {
            std::error_code error = {};
            LCS::fs::remove_all(path, error);
        } else if (entry.is_regular_file() && path.extension() == ".profile") {
            auto name = path.filename();
            name.replace_extension();
            profiles.push_back(QString::fromStdString(name.generic_string()));
        }
    }
    if (!profiles.contains("Default Profile")) {
        profiles.push_front("Default Profile");
    }
    return profiles;
}

QJsonObject LCSToolsImpl::readProfile(QString profileName) {
    QJsonObject profile;
    std::ifstream infile(progDirPath_ / "profiles" / (profileName + ".profile").toStdString());
    std::string line;
    while(std::getline(infile, line)) {
        if (line.empty()) {
            continue;
        }
        profile.insert(QString::fromStdString(line), true);
    }
    return profile;
}

void LCSToolsImpl::writeProfile(QString profileName, QJsonObject profile) {
    std::ofstream outfile(progDirPath_ / "profiles" / (profileName + ".profile").toStdString());
    for(auto mod: profile.keys()) {
        auto modname = mod.toStdString();
        if (modname.empty()) {
            continue;
        }
        outfile << modname << '\n';
    }
}

QString LCSToolsImpl::readCurrentProfile() {
    QJsonObject profile;
    std::ifstream infile(progDirPath_ / "current.profile");
    std::string line;
    if (!std::getline(infile, line) || line.empty()) {
        line = "Default Profile";
    }
    return QString::fromStdString(line);
}

void LCSToolsImpl::writeCurrentProfile(QString profile) {
    std::ofstream outfile(progDirPath_ / "current.profile");
    outfile << profile.toStdString() << '\n';
}

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

void LCSToolsImpl::init() {
    if (state_ == LCSState::StateUnitialized) {
        setState(LCSState::StateBussy);
        setStatus("Load mods");
        try {
            patcher_.load();
            modIndex_ = std::make_unique<LCS::ModIndex>(progDirPath_ / "installed");
            QJsonObject mods;
            for(auto const& [rawFileName, rawMod]: modIndex_->mods()) {
                mods.insert(QString::fromStdString(rawFileName),
                            parseInfoData(rawFileName, rawMod->info()));
            }
            auto profiles = listProfiles();
            auto profileName = readCurrentProfile();
            if (!profiles.contains(profileName)) {
                profileName = "Default Profile";
                writeCurrentProfile(profileName);
            }
            auto profileMods = readProfile(profileName);
            emit initialized(mods, profiles, profileName, profileMods);
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
            emit installedMod(QString::fromStdString(mod->filename()), infoData);
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
        if (name.isEmpty() || name.isNull()) {
            name = "Default Profile";
        }
        try {
            if (leaguepath_.isNull() || leaguepath_.isEmpty()) {
                throw std::runtime_error("League path not set!");
            }
            LCS::WadIndex index(leaguePathStd_);
            LCS::WadMergeQueue queue(progDirPath_ / "profiles" / name.toStdString(), index);
            for(auto key: mods.keys()) {
                auto fileName = key.toStdString();
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
            writeCurrentProfile(name);
            writeProfile(name, mods);
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

void LCSToolsImpl::loadProfile(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Save profile");
        if (name.isEmpty() || name.isNull()) {
            name = "Default Profile";
        }
        try {
            auto profileMods = readProfile(name);
            emit profileLoaded(name, profileMods);
        } catch(std::runtime_error const& error) {
            emit reportError("Load profile", QString(error.what()));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::deleteProfile(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBussy);
        setStatus("Delete profile");
        try {
            LCS::fs::remove(progDirPath_ / "profiles" / (name + ".profile").toStdString());
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
            auto profilePath = (progDirPath_ / "profiles" / name.toStdString()).generic_string() + "/";
            memcpy(patcher_.prefix.data(), profilePath.c_str(), profilePath.size() + 1);
            std::thread thread([this] {
                try {
                    setStatus("Wait for League");
                    setState(LCSState::StateRunning);
                    while(state_ == LCSState::StateRunning) {
                        uint32_t pid =  LCS::Process::Find("League of Legends.exe");
                        if (pid == 0) {
                            LCS::SleepMiliseconds(250);
                            continue;
                        }
                        setState(LCSState::StatePatching);
                        {
                            auto process = LCS::Process(pid);
                            if (!patcher_.good(process)) {
                                setStatus("Scan offsets");
                                process.WaitWindow("League of Legends (TM) Client", 10);
                                if (!patcher_.rescan(process)) {
                                    throw std::runtime_error("Failed to find offsets");
                                }
                                patcher_.save();
                                setStatus("Patch late");
                            } else {
                                setStatus("Patch early");
                            }
                            patcher_.patch(process);
                            setStatus("Wait for league to exit");
//                            while (!process.Exited()) {
//                                LCS::SleepMiliseconds(1000);
//                            }
                            process.WaitExit();
                        }
                        setStatus("League exited");
                        setStatus("Wait for League");
                        setState(LCSState::StateRunning);
                    }
                    setStatus("Patcher stoped");
                    setState(LCSState::StateIdle);
                } catch(std::runtime_error const& error) {
                    emit reportError("Patch League", QString(error.what()));
                    setState(LCSState::StateIdle);
                }
            });
            thread.detach();
        }
    }
}

void LCSToolsImpl::stopProfile() {
    if (state_ == LCSState::StateRunning) {
        setState(LCSState::StateStoping);
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
