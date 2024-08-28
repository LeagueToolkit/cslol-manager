#include "SystemTrayManager.h"
#include <QQmlContext>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QSystemTrayIcon>

SystemTrayManager::SystemTrayManager(QObject *parent)
    : QObject(parent), m_engine(nullptr), m_available(false)
{
}

void SystemTrayManager::initialize(QQmlApplicationEngine *engine)
{
    m_engine = engine;
    m_available = QSystemTrayIcon::isSystemTrayAvailable();
    m_engine->rootContext()->setContextProperty("systemTrayManager", this);
    emit availableChanged();
}

bool SystemTrayManager::isAvailable() const
{
    return m_available;
}

void SystemTrayManager::showWindow()
{
    emit windowVisibilityChanged(true);
}

void SystemTrayManager::hideWindow()
{
    emit windowVisibilityChanged(false);
}

void SystemTrayManager::runProfile()
{
    emit profileStateChanged(true);
}

void SystemTrayManager::stopProfile()
{
    emit profileStateChanged(false);
}

void SystemTrayManager::openLogs()
{
    // Implement log opening logic here
}

void SystemTrayManager::quit()
{
    QCoreApplication::quit();
}