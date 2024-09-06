#include "SystemTrayManager.h"
#include <QQmlContext>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include "CSLOLUtils.h"

SystemTrayManager::SystemTrayManager(QObject *parent)
    : QObject(parent), m_engine(nullptr), m_available(false), m_utils(new CSLOLUtils(this))
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
    QString logPath = m_utils->getLogFilePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
}

void SystemTrayManager::setUpdateUrl(const QString &url)
{
    if (m_updateUrl != url) {
        m_updateUrl = url;
        emit updateUrlChanged();
    }
}

void SystemTrayManager::openUpdateUrl()
{
    if (!m_updateUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_updateUrl));
    }
}

void SystemTrayManager::quit()
{
    QCoreApplication::quit();
}

void SystemTrayManager::setPatcherRunning(bool running) {
    if (m_patcherRunning != running) {
        m_patcherRunning = running;
        emit patcherRunningChanged(running);
    }
}

void SystemTrayManager::setSystemTrayIconVisible(bool visible)
{
    if (m_systemTrayIconVisible != visible) {
        m_systemTrayIconVisible = visible;
        emit systemTrayIconVisibleChanged(visible);
    }
}