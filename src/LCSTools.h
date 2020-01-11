#ifndef QMODMANAGER_H
#define QMODMANAGER_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QString>
#include <QList>
#include <QStringList>
#include <string>
#include <QProcess>
#include <QQmlEngine>
#include <QJsonArray>
#include <QJsonObject>
#include "LCSToolsImpl.h"


class LCSTools : public QObject
{
    Q_OBJECT
    Q_PROPERTY(LCSToolsImpl::LCSState state READ getState NOTIFY stateChanged)
    Q_PROPERTY(QString status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(QString leaguePath READ getLeaguePath WRITE changeLeaguePath NOTIFY leaguePathChanged)
public:
    using LCSState = LCSToolsImpl::LCSState;
    explicit LCSTools(QObject *parent = nullptr);
    ~LCSTools();

signals:
    void stateChanged(LCSState state);
    void statusChanged(QString status);
    void leaguePathChanged(QString leaguePath);

    void progressStart(quint32 itemsTotal, quint64 dataTotal);
    void progressItems(quint32 itemsDone);
    void progressData(quint64 dataDone);
    void progressEnd();

    void initialized(QJsonObject mods, QJsonObject savedProfiles);
    void modDeleted(QString name);
    void installedMod(QString fileName, QJsonObject infoData);
    void profileSaved(QString name, QJsonObject mods);
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


    void changeLeaguePath(QString newLeaguePath);
    void init(QJsonObject profiles);
    void deleteMod(QString name);
    void exportMod(QString name, QString dest);
    void installFantomeZip(QString path);
    void saveProfile(QString name, QJsonObject mods, bool run, bool whole);
    void deleteProfile(QString name);
    void runProfile(QString name);
    void stopProfile();
    void makeMod(QString fileName, QString image, QJsonObject infoData, QJsonArray items);
    void startEditMod(QString fileName);
    void changeModInfo(QString fileName, QJsonObject infoData);
    void changeModImage(QString fileName, QString image);
    void removeModImage(QString fileName);
    void addModWads(QString modFileName, QJsonArray wads);
    void removeModWads(QString modFileName, QJsonArray wads);
public slots:
    LCSState getState();
    QString getStatus();
    QString getLeaguePath();

private slots:
    void setState(LCSState value);
    void setStatus(QString status);
    void setLeaguePath(QString value);

private:
    QThread* thread_ = nullptr;
    LCSToolsImpl* worker_ = nullptr;
    QString leaguePath_;
    LCSState state_ = LCSState::StateUnitialized;
    QString status_;
};

#endif // QMODMANAGER_H
