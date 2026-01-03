#ifndef CSLOLSYSTEMTRAYMANAGER_H
#define CSLOLSYSTEMTRAYMANAGER_H

#include <QObject>
#include <QQmlApplicationEngine>
#include "CSLOLUtils.h"

class CSLOLSystemTrayManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString updateUrl READ updateUrl WRITE setUpdateUrl NOTIFY updateUrlChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(bool patcherRunning READ isPatcherRunning NOTIFY patcherRunningChanged)
    Q_PROPERTY(bool systemTrayIconVisible READ isSystemTrayIconVisible NOTIFY systemTrayIconVisibleChanged)

public:
    explicit CSLOLSystemTrayManager(QObject *parent = nullptr);
    void initialize(QQmlApplicationEngine *engine);
    bool isAvailable() const;
    QString updateUrl() const { return m_updateUrl; }
    void setUpdateUrl(const QString &url);
    bool isPatcherRunning() const { return m_patcherRunning; }
    bool isSystemTrayIconVisible() const { return m_systemTrayIconVisible; }

public slots:
    void showWindow();
    void hideWindow();
    void runProfile();
    void stopProfile();
    void openLogs();
    void openUpdateUrl();
    void quit();
    void setPatcherRunning(bool running);
    void setSystemTrayIconVisible(bool visible);

signals:
    void windowVisibilityChanged(bool visible);
    void profileStateChanged(bool running);
    void availableChanged();
    void updateUrlChanged();
    void patcherRunningChanged(bool running);
    void systemTrayIconVisibleChanged(bool visible);

private:
    QQmlApplicationEngine *m_engine;
    QString m_updateUrl;
    bool m_available;
    CSLOLUtils *m_utils;
    bool m_patcherRunning = false;
    bool m_systemTrayIconVisible = false;
};

#endif // CSLOLSYSTEMTRAYMANAGER_H