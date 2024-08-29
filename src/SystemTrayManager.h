#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QObject>
#include <QQmlApplicationEngine>
#include "CSLOLUtils.h"

class SystemTrayManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    explicit SystemTrayManager(QObject *parent = nullptr);
    void initialize(QQmlApplicationEngine *engine);
    bool isAvailable() const;

public slots:
    void showWindow();
    void hideWindow();
    void runProfile();
    void stopProfile();
    void openLogs();
    void quit();

signals:
    void windowVisibilityChanged(bool visible);
    void profileStateChanged(bool running);
    void availableChanged();

private:
    QQmlApplicationEngine *m_engine;
    bool m_available;
    CSLOLUtils *m_utils;
};

#endif // SYSTEMTRAYMANAGER_H