#ifndef QMODMANAGER_H
#define QMODMANAGER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QProcess>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QThread>
#include <string>

#include "CSLOLToolsImpl.h"

class CSLOLTools : public QObject {
    Q_OBJECT
    Q_PROPERTY(CSLOLToolsImpl::CSLOLState state READ getState NOTIFY stateChanged)
    Q_PROPERTY(QString status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(QString leaguePath READ getLeaguePath WRITE changeLeaguePath NOTIFY leaguePathChanged)
public:
    using CSLOLState = CSLOLToolsImpl::CSLOLState;
    explicit CSLOLTools(QObject* parent = nullptr);
    ~CSLOLTools();

signals:
    void stateChanged(CSLOLToolsImpl::CSLOLState state);
    void statusChanged(QString status);
    void leaguePathChanged(QString leaguePath);
    void blacklistChanged(bool blacklist);
    void ignorebadChanged(bool ignorebad);

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

    void changeLeaguePath(QString newLeaguePath);
    void changeBlacklist(bool blacklist);
    void changeIgnorebad(bool ignorebad);
    void init();
    void deleteMod(QString name);
    void exportMod(QString name, QString dest);
    void installFantomeZip(QString path);
    void saveProfile(QString name, QJsonObject mods, bool run, bool skipConflict, bool debugPatcher);
    void loadProfile(QString name);
    void deleteProfile(QString name);
    void stopProfile();
    void makeMod(QString fileName, QJsonObject infoData, QString image);
    void startEditMod(QString fileName);
    void changeModInfo(QString fileName, QJsonObject infoData, QString image);
    void addModWad(QString modFileName, QString wad, bool removeUnknownNames);
    void removeModWads(QString modFileName, QJsonArray wads);
    void refreshMods();
    void runDiag();

public slots:
    CSLOLToolsImpl::CSLOLState getState();
    QString getStatus();
    QString getLeaguePath();

private slots:
    void setState(CSLOLToolsImpl::CSLOLState value);
    void setStatus(QString status);
    void setLeaguePath(QString value);

private:
    QThread* thread_ = nullptr;
    CSLOLToolsImpl* worker_ = nullptr;
    QString leaguePath_;
    CSLOLState state_ = CSLOLState::StateUnitialized;
    QString status_;
};

#endif  // QMODMANAGER_H
