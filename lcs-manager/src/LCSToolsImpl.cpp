#include "LCSToolsImpl.h"
#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QMetaEnum>
#include <QThread>
#include <fstream>

static QString get_stack_trace(std::runtime_error const& err) noexcept {
    QString result = {};
    auto trace = LCS::error_stack_trace();
    result = QString::fromUtf8((char const*)trace.data(), (int)trace.size());
    if (auto conflict_err = dynamic_cast<LCS::ConflictError const*>(&err)) {
        result += QString::fromUtf8((char const*)conflict_err->message.data(), (int)conflict_err->message.size());
    } else {
        result += QString::fromStdString(err.what());
    }
    return result;
}

LCSToolsImpl::LCSToolsImpl(QObject *parent)
    : QObject(parent), progDirPath_(QCoreApplication::applicationDirPath().toStdU16String()),
      patcherConfig_(progDirPath_ / "lolcustomskin.txt") {}

LCSToolsImpl::~LCSToolsImpl() {
    if (lockfile_) {
        delete lockfile_;
    }
}

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
        if (entry.is_directory() && !LCS::fs::exists(path.generic_u8string() + u8".profile")) {
            std::error_code error = {};
            LCS::fs::remove_all(path, error);
        } else if (entry.is_regular_file() && path.extension() == ".profile") {
            auto name = path.filename();
            name.replace_extension();
            profiles.push_back(QString::fromStdU16String(name.generic_u16string()));
        }
    }
    if (!profiles.contains("Default Profile")) {
        profiles.push_front("Default Profile");
    }
    return profiles;
}

QJsonObject LCSToolsImpl::readProfile(QString profileName) {
    QJsonObject profile;
    LCS::fs::path profile_name = progDirPath_ / "profiles" / (profileName + ".profile").toStdU16String();
    std::ifstream infile(profile_name, std::ios::binary);
    std::string line;
    while(std::getline(infile, line)) {
        if (line.empty()) {
            continue;
        }
        profile.insert(QString::fromUtf8(line.data(), (int)line.size()), true);
    }
    return profile;
}

void LCSToolsImpl::writeProfile(QString profileName, QJsonObject profile) {
    LCS::fs::path profile_name = progDirPath_ / "profiles" / (profileName + ".profile").toStdU16String();
    std::ofstream outfile(profile_name, std::ios::binary);
    for(auto mod: profile.keys()) {
        auto modname = mod.toUtf8();
        if (modname.size() == 0) {
            continue;
        }
        outfile.write(modname.data(), modname.size());
        outfile << '\n';
    }
}

QString LCSToolsImpl::readCurrentProfile() {
    QJsonObject profile;
    std::ifstream infile(progDirPath_ / "current.profile", std::ios::binary);
    std::string line;
    if (!std::getline(infile, line) || line.empty()) {
        line = "Default Profile";
    }
    return QString::fromUtf8(line.data(), (int)line.size());
}

void LCSToolsImpl::writeCurrentProfile(QString profile) {
    std::ofstream outfile(progDirPath_ / "current.profile", std::ios::binary);
    auto profiledata = profile.toUtf8();
    outfile.write(profiledata.data(), profiledata.size());
    outfile << '\n';
}

LCS::WadIndex const& LCSToolsImpl::wadIndex() {
    if (leaguepath_.isNull() || leaguepath_.isEmpty()) {
        throw std::runtime_error("League path not set!");
    }

    while (wadIndex_ == nullptr || !wadIndex_->is_uptodate()) {
        wadIndex_ = std::make_unique<LCS::WadIndex>(leaguePathStd_, blacklist_, ignorebad_);
        QThread::msleep(250);
    }

    return *wadIndex_;
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

    QJsonObject parseInfoData(QString fileName, std::u8string const& str) {
        QByteArray data = QByteArray((char const*)str.data(), (int)str.size());
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(data, &error);
        if (!document.isObject()) {
            return validateAndCorrect(fileName, QJsonObject());
        }
        return validateAndCorrect(fileName, document.object());
    }

    QJsonObject parseInfoData(LCS::fs::path const& fileName, std::u8string const& str) {
        return parseInfoData(QString::fromStdU16String(fileName.generic_u16string()), str);
    }

    std::u8string dumpInfoData(QJsonObject info) {
        QJsonDocument document(info);
        auto data = document.toJson();
        return { data.begin(), data.end() };
    }
}

/// impl

void LCSToolsImpl::changeLeaguePath(QString newLeaguePath) {
    if (state_ == LCSState::StateIdle || state_ == LCSState::StateUnitialized) {
        if (state_ != LCSState::StateUnitialized) {
            setState(LCSState::StateBusy);
            setStatus("Change League Path");
        }
        if (newLeaguePath.isEmpty() || newLeaguePath.isNull()) {
            return;
        }
        LCS::fs::path path = newLeaguePath.toStdU16String();
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
            newLeaguePath = QString::fromStdU16String(path.generic_u16string());
            if (newLeaguePath != leaguepath_) {
                leaguePathStd_ = path;
                leaguepath_ = newLeaguePath;
                wadIndex_ = nullptr;
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

void LCSToolsImpl::changeBlacklist(bool blacklist) {
    if (state_ == LCSState::StateIdle || state_ == LCSState::StateUnitialized) {
        if (blacklist_ != blacklist) {
            if (state_ != LCSState::StateUnitialized) {
                setState(LCSState::StateBusy);
                setStatus("Toggle blacklist");
            }
            blacklist_ = blacklist;
            wadIndex_ = nullptr;
            emit blacklistChanged(blacklist);
            if (state_ != LCSState::StateUnitialized) {
                setState(LCSState::StateIdle);
            }
        }
    }
}

void LCSToolsImpl::changeIgnorebad(bool ignorebad) {
    if (state_ == LCSState::StateIdle || state_ == LCSState::StateUnitialized) {
        if (ignorebad_ != ignorebad) {
            if (state_ != LCSState::StateUnitialized) {
                setState(LCSState::StateBusy);
                setStatus("Toggle ignorebad");
            }
            ignorebad_ = ignorebad;
            wadIndex_ = nullptr;
            emit ignorebadChanged(ignorebad);
            if (state_ != LCSState::StateUnitialized) {
                setState(LCSState::StateIdle);
            }
        }
    }
}


void LCSToolsImpl::init() {
    if (state_ == LCSState::StateUnitialized) {
        setState(LCSState::StateBusy);
        setStatus("Acquire lock");
        auto lockpath = QString::fromStdU16String((progDirPath_/ "lockfile").generic_u16string());
        lockfile_ = new QLockFile(lockpath);
        if (!lockfile_->tryLock()) {
            auto lockerror = QString::number((int)lockfile_->error());
            emit reportError("Check already running", QString("Lock error: ") + lockerror);
            setState(LCSState::StateCriticalError);
            return;
        }
        setStatus("Load mods");
        try {
#ifdef WIN32
            patcher_.load(patcherConfig_);
#endif
            modIndex_ = std::make_unique<LCS::ModIndex>(progDirPath_ / "installed");
            QJsonObject mods;
            for(auto const& [rawFileName, rawMod]: modIndex_->mods()) {
                mods.insert(QString::fromStdU16String(rawFileName.generic_u16string()),
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
            emit reportError("Load mods", get_stack_trace(error));
            setState(LCSState::StateCriticalError);
        }
    }
}

void LCSToolsImpl::deleteMod(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Delete mod");
        try {
            modIndex_->remove(name.toStdU16String());
            emit modDeleted(name);
        } catch(std::runtime_error const& error) {
            emit reportError("", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::exportMod(QString name, QString dest) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Export mod");
        try {
            modIndex_->export_zip(name.toStdU16String(), dest.toStdU16String(), *dynamic_cast<LCS::ProgressMulti*>(this));
        } catch(std::runtime_error const& error) {
            emit reportError("Export mod", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::installFantomeZip(QString path) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Installing Mod");
        path = path.replace('\\', '/');
        try {
            auto const& index = wadIndex();
            auto& progress = *dynamic_cast<LCS::ProgressMulti*>(this);
            LCS::Mod* mod = nullptr;
            // TODO: gezi skin
            if (path.endsWith(".wad.client")
                || path.endsWith(".wad.client/")
                || path.endsWith(".wad")
                || path.endsWith(".wad/")) {
                mod = modIndex_->install_from_wad(path.toStdU16String(), index, progress);
            } else if (path.endsWith(".fantome") || path.endsWith(".zip")) {
                mod = modIndex_->install_from_fantome(path.toStdU16String(), index, progress);
            } else if (path.endsWith(".wxy")) {
                mod = modIndex_->install_from_wxy(path.toStdU16String(), index, progress);
            } else {
                mod = modIndex_->install_from_folder(path.toStdU16String(), index, progress);
            }
            emit installedMod(QString::fromStdU16String(mod->filename().generic_u16string()),
                              parseInfoData(mod->filename(), mod->info()));
        } catch(std::runtime_error const& error) {
            emit reportError("Error while installing Mod", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::makeMod(QString fileName, QString image, QJsonObject infoData, QJsonArray items) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Make mod");
        try {
            auto const& index = wadIndex();
            LCS::WadMakeQueue queue(index);
            for(auto item: items) {
                queue.addItem(LCS::fs::path(item.toString().toStdU16String()), LCS::Conflict::Abort);
            }
            infoData = validateAndCorrect(fileName, infoData);
            auto mod = modIndex_->make(fileName.toStdU16String(),
                                       dumpInfoData(infoData),
                                       image.toStdU16String(),
                                       queue,
                                       *dynamic_cast<LCS::ProgressMulti*>(this));
            emit installedMod(QString::fromStdU16String(mod->filename().generic_u16string()), infoData);
        } catch(std::runtime_error const& error) {
            emit reportError("Make mod", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::saveProfile(QString name, QJsonObject mods, bool run) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Save profile");
        if (name.isEmpty() || name.isNull()) {
            name = "Default Profile";
        }
        try {
            auto const& index = wadIndex();
            LCS::WadMergeQueue queue(progDirPath_ / "profiles" / name.toStdU16String(), index);
            for(auto key: mods.keys()) {
                auto fileName = key.toStdU16String();
                if (auto i = modIndex_->mods().find(fileName); i != modIndex_->mods().end()) {
                    queue.addMod(i->second.get(), LCS::Conflict::Abort);
                }
            }
            queue.write(*dynamic_cast<LCS::ProgressMulti*>(this));
            queue.cleanup();
            writeCurrentProfile(name);
            writeProfile(name, mods);
            emit profileSaved(name, mods);
        } catch(std::runtime_error const& error) {
            emit reportError("Save profile", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
    if (run) {
        emit runProfile(name);
    }
}

void LCSToolsImpl::loadProfile(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Save profile");
        if (name.isEmpty() || name.isNull()) {
            name = "Default Profile";
        }
        try {
            auto profileMods = readProfile(name);
            emit profileLoaded(name, profileMods);
        } catch(std::runtime_error const& error) {
            emit reportError("Load profile", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::deleteProfile(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Delete profile");
        try {
            LCS::fs::remove(progDirPath_ / "profiles" / (name + ".profile").toStdU16String());
            LCS::fs::remove_all(progDirPath_ / "profiles" / name.toStdU16String());
            emit profileDeleted(name);
        } catch(std::runtime_error const& error) {
            emit reportError("Delete Profile", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::runProfile(QString name) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Run profile");
        // FIXME: force league to use utf8 ??
        auto profilePath = (progDirPath_ / "profiles" / name.toStdU16String()).generic_string() + "/";
        std::thread thread([this, profilePath = std::move(profilePath)] {
            try {
                setStatus("Waiting for league match to start...");
                setState(LCSState::StateRunning);
                while (state_ == LCSState::StateRunning) {
#ifdef WIN32
                    auto process = LCS::Process::Find("League of Legends.exe");
                    if (!process) {
                        LCS::SleepMS(250);
                        continue;
                    }
                    setState(LCSState::StatePatching);
                    setStatus("Found league");
                    if (!patcher_.check(*process)) {
                        setStatus("Game is starting");
                        process->WaitInitialized();
                        setStatus("Scan offsets");
                        patcher_.scan(*process);
                        patcher_.save(patcherConfig_);
                    } else {
                        setStatus("Wait patchable");
                        patcher_.wait_patchable(*process);
                    }
                    patcher_.patch(*process, profilePath);
                    setStatus("Wait for league to exit");
                    process->WaitExit();
                    setStatus("Waiting for league match to start...");
                    setState(LCSState::StateRunning);
#else
                  QThread::msleep(250);
#endif
                }
                setStatus("Patcher stoped");
                setState(LCSState::StateIdle);
            } catch(std::runtime_error const& error) {
                emit reportError("Patch League", get_stack_trace(error));
                setState(LCSState::StateIdle);
            }
        });
        thread.detach();
    }
}

void LCSToolsImpl::stopProfile() {
    if (state_ == LCSState::StateRunning) {
        setState(LCSState::StateStoping);
    }
}

void LCSToolsImpl::startEditMod(QString fileName) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Edit mod");
        try {
            LCS::fs::path nameStd = fileName.toStdU16String();
            if (auto i = modIndex_->mods().find(nameStd); i != modIndex_->mods().end()) {
                auto const& mod = i->second;
                QJsonArray wads;
                for(auto const& [name, wad]: mod->wads()) {
                    wads.push_back(QString::fromStdU16String(name.generic_u16string()));
                }
                emit modEditStarted(fileName,
                                    parseInfoData(fileName, mod->info()),
                                    QString::fromStdU16String(mod->image().generic_u16string()),
                                    wads);
            } else {
                throw std::runtime_error("No such mod found!");
            }
        } catch(std::runtime_error error) {
            emit reportError("Edit mod", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::changeModInfo(QString fileName, QJsonObject infoData) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Change mod info");
        try {
            infoData = validateAndCorrect(fileName, infoData);
            modIndex_->change_mod_info(fileName.toStdU16String(), dumpInfoData(infoData));
            emit modInfoChanged(fileName, infoData);
        } catch(std::runtime_error error) {
            emit emit reportError("Change mod info", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::changeModImage(QString fileName, QString image) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Change mod image");
        try {
            modIndex_->change_mod_image(fileName.toStdU16String(), image.toStdU16String());
            emit modImageChanged(fileName, image);
        } catch(std::runtime_error error) {
            emit reportWarning("Change mod image", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::removeModImage(QString fileName) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Remove mod image");
        try {
            modIndex_->remove_mod_image(fileName.toStdU16String());
            emit modImageRemoved(fileName);
        } catch(std::runtime_error error) {
            emit reportWarning("Remove mod image", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::removeModWads(QString fileName, QJsonArray wads) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Remove mod wads");
        try {
            for(auto wadName: wads) {
                modIndex_->remove_mod_wad(fileName.toStdU16String(), wadName.toString().toStdU16String());
            }
            emit modWadsRemoved(fileName, wads);
        } catch(std::runtime_error error) {
            emit reportError("Remove mod image", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

void LCSToolsImpl::addModWads(QString fileName, QJsonArray wads) {
    if (state_ == LCSState::StateIdle) {
        setState(LCSState::StateBusy);
        setStatus("Add mod wads");
        try {
            auto const& index = wadIndex();
            LCS::WadMakeQueue wadMake(index);
            for(auto wadPath: wads) {
                wadMake.addItem(LCS::fs::path(wadPath.toString().toStdU16String()), LCS::Conflict::Abort);
            }
            auto added = modIndex_->add_mod_wads(fileName.toStdU16String(), wadMake,
                                                 *dynamic_cast<LCS::ProgressMulti*>(this),
                                                 LCS::Conflict::Abort);
            QJsonArray names;
            for(auto const& wad: added) {
                names.push_back(QString::fromStdU16String(wad->name().generic_u16string()));
            }
            emit modWadsAdded(fileName, names);
        } catch(std::runtime_error error) {
            emit reportError("Add mod wads", get_stack_trace(error));
        }
        setState(LCSState::StateIdle);
    }
}

/// Interface implementations
void LCSToolsImpl::startItem(LCS::fs::path const& path, std::uint64_t bytes) noexcept {
    auto name = QString::fromStdU16String(path.filename().generic_u16string());
    auto size = QString::number(bytes / 1024.0 / 1024.0, 'f', 2);
    setStatus("Processing " + name + "(" + size + "MB)");
}

void LCSToolsImpl::consumeData(std::uint64_t ammount) noexcept {
    progressDataDone_ += ammount;
    emit progressData((quint64)progressDataDone_);
}

void LCSToolsImpl::finishItem() noexcept {
    progressItemDone_++;
    emit progressItems((quint32)progressItemDone_);
}

void LCSToolsImpl::startMulti(size_t itemCount, std::uint64_t dataTotal) noexcept {
    progressItemDone_ = 0;
    progressDataDone_ = 0;
    emit progressStart((quint32)itemCount, (quint64)dataTotal);
}

void LCSToolsImpl::finishMulti() noexcept {
    emit progressEnd();
}
