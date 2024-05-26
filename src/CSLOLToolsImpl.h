#ifndef QMODMANAGERWORKER_H
#define QMODMANAGERWORKER_H
#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QProcess>
#include <QString>
#include <vector>

class CSLOLToolsImpl : public QObject {
    Q_OBJECT
public:
    enum CSLOLState {
        StateCriticalError = -1,
        StateUnitialized = 0,
        StateIdle = 1,
        StateBusy = 2,
        StateRunning = 3,
        StatePatching = 4,
        StateStoping = 5,
    };
    Q_ENUM(CSLOLState)

    explicit CSLOLToolsImpl(QObject* parent = nullptr);
    ~CSLOLToolsImpl() override;
signals:
    void stateChanged(CSLOLToolsImpl::CSLOLState state);
    void blacklistChanged(bool blacklist);
    void ignorebadChanged(bool ignorebad);
    void statusChanged(QString message);
    void leaguePathChanged(QString leaguePath);

    void initialized(QJsonObject mods, QJsonArray profiles, QString profileName, QJsonObject profileMods);
    void modDeleted(QString name);
    void installedMod(QString fileName, QJsonObject infoData);
    void profileSaved(QString name, QJsonObject mods);
    void profileLoaded(QString name, QJsonObject profileMods);
    void profileDeleted(QString name);
    void modCreated(QString fileName, QJsonObject infoData, QString image);
    void modEditStarted(QString fileName, QJsonObject infoData, QString image, QJsonArray wads);
    void modInfoChanged(QString fileName, QJsonObject infoData, QString image);
    void modWadsAdded(QString modFileName, QJsonArray wads);
    void modWadsRemoved(QString modFileName, QJsonArray wads);
    void refreshed(QJsonObject mods);
    void updatedMods(QJsonArray mods);
    void reportError(QString name, QString message, QString stack_trace);

public slots:
    void changeLeaguePath(QString newLeaguePath);
    void changeBlacklist(bool blacklist);
    void changeIgnorebad(bool blacklist);
    void init();
    void deleteMod(QString name);
    void exportMod(QString name, QString dest);
    void installFantomeZip(QString path);
    void saveProfile(QString name, QJsonObject mods, bool run, bool skipConflict, bool oldPatcher);
    void loadProfile(QString name);
    void deleteProfile(QString name);
    void stopProfile();
    void makeMod(QString fileName, QJsonObject infoData, QString image);
    void refreshMods();
    void runDiag();

    void startEditMod(QString fileName);
    void changeModInfo(QString fileName, QJsonObject infoData, QString image);
    void removeModWads(QString modFileName, QJsonArray wadNames);
    void addModWad(QString modFileName, QString wad, bool removeUnknownNames);

    CSLOLToolsImpl::CSLOLState getState();
    QString getLeaguePath();

private:
    QNetworkAccessManager* networkManager_ = nullptr;
    std::vector<QJsonDocument> networkResults_ = {};
    std::vector<QNetworkRequest> networkRequests_ = {};
    QLockFile* lockfile_ = nullptr;
    QProcess* patcherProcess_ = nullptr;
    QString prog_ = "";
    QString game_ = "";
    CSLOLToolsImpl::CSLOLState state_ = CSLOLState::StateUnitialized;
    bool blacklist_ = true;
    bool ignorebad_ = false;
    QString status_ = "";
    QFile* logFile_ = nullptr;
    void setState(CSLOLToolsImpl::CSLOLState state);
    void setStatus(QString status);

    QStringList modList();
    QStringList modWadsList(QString modName);
    QJsonObject modInfoRead(QString modName);
    bool modInfoWrite(QString modName, QJsonObject object);
    QString modImageGet(QString modName);
    QString modImageSet(QString modName, QString image);
    QStringList listProfiles();
    QJsonObject readProfile(QString profileName);
    void writeProfile(QString profileName, QJsonObject profile);
    QString readCurrentProfile();
    void writeCurrentProfile(QString profile);
    void doReportError(QString name, QString message, QString trace);

    void runPatcher(QStringList args);
    void runTool(QStringList args, std::function<void(int code, QProcess*)> handle);
    void runDiagInternal(bool internal_once);
};

#endif  // QMODMANAGERWORKER_H
