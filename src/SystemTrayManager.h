#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QObject>
#include <QQmlApplicationEngine>
#include "CSLOLUtils.h"

class SystemTrayManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString updateUrl READ updateUrl WRITE setUpdateUrl NOTIFY updateUrlChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(bool patcherRunning READ isPatcherRunning NOTIFY patcherRunningChanged)

public:
    explicit SystemTrayManager(QObject *parent = nullptr);
    void initialize(QQmlApplicationEngine *engine);
    bool isAvailable() const;
    QString updateUrl() const { return m_updateUrl; }
    void setUpdateUrl(const QString &url);
    bool isPatcherRunning() const { return m_patcherRunning; }

public slots:
    void showWindow();
    void hideWindow();
    void runProfile();
    void stopProfile();
    void openLogs();
    void openUpdateUrl();
    void quit();
    void setPatcherRunning(bool running);

signals:
    void windowVisibilityChanged(bool visible);
    void profileStateChanged(bool running);
    void availableChanged();
    void updateUrlChanged();
    void patcherRunningChanged(bool running);

private:
    QQmlApplicationEngine *m_engine;
    QString m_updateUrl;
    bool m_available;
    CSLOLUtils *m_utils;
    bool m_patcherRunning = false;
};

#endif // SYSTEMTRAYMANAGER_H