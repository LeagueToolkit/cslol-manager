#ifndef QMODMANAGERWORKER_H
#define QMODMANAGERWORKER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QProcess>
#include <QJsonArray>
#include <QJsonObject>
#include "lcs/common.hpp"
#include "lcs/modindex.hpp"


class LCSToolsImpl : public QObject, public LCS::ProgressMulti
{
    Q_OBJECT
public:
    enum LCSState {
        StateCriticalError = -1,
        StateUnitialized = 0,
        StateIdle = 1,
        StateBussy = 2,
        StateExternalRunning = 3,
    };
    Q_ENUM(LCSState)

    explicit LCSToolsImpl(QObject *parent = nullptr);
    ~LCSToolsImpl();
signals:
    void stateChanged(LCSState state);
    void statusChanged(QString message);
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

public slots:
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
    void removeModWads(QString modFileName, QJsonArray wadNames);
    void addModWads(QString modFileName, QJsonArray wadPaths);

    LCSState getState();
    QString getLeaguePath();
public:
    void startItem(std::filesystem::path const& path, size_t) noexcept override;
    void consumeData(size_t ammount) noexcept override;
    void finishItem() noexcept override;
    void startMulti(size_t itemCount, size_t dataTotal) noexcept override;
    void finishMulti() noexcept override;

private:
    std::unique_ptr<LCS::ModIndex> modIndex_;
    QProcess* process_ = nullptr;
    QString leaguepath_;
    LCS::fs::path leaguePathStd_;
    LCS::fs::path progDirPath_;
    LCSState state_ = LCSState::StateUnitialized;
    QString status_ = "";
    void setState(LCSState state);
    void setStatus(QString status);

/// ProgressMulti impl
private:
    size_t progressItemDone_;
    size_t progressDataDone_;

private slots:
    void readyReadStandardOutput();
    void errorOccured(QProcess::ProcessError error);
    void finished(int exitCode);
    void started();
};


#endif // QMODMANAGERWORKER_H
