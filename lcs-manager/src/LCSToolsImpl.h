#ifndef QMODMANAGERWORKER_H
#define QMODMANAGERWORKER_H
#include <QObject>
#include <QString>
#include <QMap>
#include <QProcess>
#include <QJsonArray>
#include <QJsonObject>
#include <QLockFile>
#include "lcs/common.hpp"
#include "lcs/error.hpp"
#include "lcs/progress.hpp"
#include "lcs/conflict.hpp"
#include "lcs/mod.hpp"
#include "lcs/modindex.hpp"
#include "lcs/wadindex.hpp"
#include "lcs/wadmakequeue.hpp"
#include "lcs/wadmerge.hpp"
#include "lcs/wadmergequeue.hpp"
#ifdef WIN32
#include "process.hpp"
#include "modoverlay.hpp"
#endif

class LCSToolsImpl : public QObject, public LCS::ProgressMulti
{
    Q_OBJECT
public:
    enum LCSState {
        StateCriticalError = -1,
        StateUnitialized = 0,
        StateIdle = 1,
        StateBussy = 2,
        StateRunning = 3,
        StatePatching = 4,
        StateStoping = 5,
    };
    Q_ENUM(LCSState)

    explicit LCSToolsImpl(QObject *parent = nullptr);
    ~LCSToolsImpl() override;
signals:
    void stateChanged(LCSState state);
    void blacklistChanged(bool blacklist);
    void ignorebadChanged(bool ignorebad);
    void statusChanged(QString message);
    void leaguePathChanged(QString leaguePath);

    void progressStart(quint32 itemsTotal, quint64 dataTotal);
    void progressItems(quint32 itemsDone);
    void progressData(quint64 dataDone);
    void progressEnd();

    void initialized(QJsonObject mods, QJsonArray profiles, QString profileName, QJsonObject profileMods);
    void modDeleted(QString name);
    void installedMod(QString fileName, QJsonObject infoData);
    void profileSaved(QString name, QJsonObject mods);
    void profileLoaded(QString name, QJsonObject profileMods);
    void profileDeleted(QString name);
    void updateProfileStatus(QString message);
    void modEditStarted(QString fileName, QJsonObject infoData, QString image, QJsonArray wads);
    void modInfoChanged(QString fileName, QJsonObject infoData);
    void modImageChanged(QString fileName, QString image);
    void modImageRemoved(QString fileName);
    void modWadsAdded(QString modFileName, QJsonArray wads);
    void modWadsRemoved(QString modFileName, QJsonArray wads);
    void reportWarning(QString category, QString message);
    void reportError(QString category, QString message);

public slots:
    void changeLeaguePath(QString newLeaguePath);
    void changeBlacklist(bool blacklist);
    void changeIgnorebad(bool blacklist);
    void init();
    void deleteMod(QString name);
    void exportMod(QString name, QString dest);
    void installFantomeZip(QString path);
    void saveProfile(QString name, QJsonObject mods, bool run);
    void loadProfile(QString name);
    void deleteProfile(QString name);
    void runProfile(QString name);
    void stopProfile();
    void makeMod(QString fileName, QString image, QJsonObject infoData, QJsonArray items);

    void startEditMod(QString fileName);
    void changeModInfo(QString fileName, QJsonObject infoData);
    void changeModImage(QString fileName, QString image);
    void removeModImage(QString fileName);
    void removeModWads(QString modFileName, QJsonArray wadNames);
    void addModWads(QString modFileName, QJsonArray wadPaths);

    LCSState getState();
    QString getLeaguePath();
private:
    QLockFile* lockfile_ = nullptr;
    LCS::fs::path progDirPath_;
    QString leaguepath_ = {};
    LCS::fs::path leaguePathStd_;
    std::string patcherConfig_ = {};
    std::unique_ptr<LCS::ModIndex> modIndex_ = nullptr;
    std::unique_ptr<LCS::WadIndex> wadIndex_ = nullptr;
#ifdef WIN32
    LCS::ModOverlay patcher_ = {};
#endif
    LCSState state_ = LCSState::StateUnitialized;
    bool blacklist_ = true;
    bool ignorebad_ = false;
    QString status_ = "";
    void setState(LCSState state);
    void setStatus(QString status);

    QJsonArray listProfiles();
    QJsonObject readProfile(QString profileName);
    void writeProfile(QString profileName, QJsonObject profile);
    QString readCurrentProfile();
    void writeCurrentProfile(QString profile);
    LCS::WadIndex const& wadIndex();

/// ProgressMulti impl
public:
    void startItem(LCS::fs::path const& path, std::uint64_t) noexcept override;
    void consumeData(std::uint64_t ammount) noexcept override;
    void finishItem() noexcept override;
    void startMulti(size_t itemCount, std::uint64_t dataTotal) noexcept override;
    void finishMulti() noexcept override;

private:
    std::uint64_t progressItemDone_;
    std::uint64_t progressDataDone_;
};


#endif // QMODMANAGERWORKER_H
