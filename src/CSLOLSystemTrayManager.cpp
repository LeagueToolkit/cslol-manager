#include "CSLOLSystemTrayManager.h"
#include <QQmlContext>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QSystemTrayIcon>
#include "CSLOLUtils.h"

CSLOLSystemTrayManager::CSLOLSystemTrayManager(QObject *parent)
    : QObject(parent), m_engine(nullptr), m_available(false), m_utils(new CSLOLUtils(this))
{
}

void CSLOLSystemTrayManager::initialize(QQmlApplicationEngine *engine)
{
    m_engine = engine;
    m_available = QSystemTrayIcon::isSystemTrayAvailable();
    m_engine->rootContext()->setContextProperty("systemTrayManager", this);
    emit availableChanged();
}

bool CSLOLSystemTrayManager::isAvailable() const
{
    return m_available;
}

void CSLOLSystemTrayManager::showWindow()
{
    emit windowVisibilityChanged(true);
}

void CSLOLSystemTrayManager::hideWindow()
{
    emit windowVisibilityChanged(false);
}

void CSLOLSystemTrayManager::runProfile()
{
    emit profileStateChanged(true);
}

void CSLOLSystemTrayManager::stopProfile()
{
    emit profileStateChanged(false);
}

void CSLOLSystemTrayManager::openLogs()
{
    QString logPath = m_utils->getLogFilePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
}

void CSLOLSystemTrayManager::setUpdateUrl(const QString &url)
{
    if (m_updateUrl != url) {
        m_updateUrl = url;
        emit updateUrlChanged();
    }
}

void CSLOLSystemTrayManager::openUpdateUrl()
{
    if (!m_updateUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_updateUrl));
    }
}

void CSLOLSystemTrayManager::quit()
{
    QCoreApplication::quit();
}

void CSLOLSystemTrayManager::setPatcherRunning(bool running) {
    if (m_patcherRunning != running) {
        m_patcherRunning = running;
        emit patcherRunningChanged(running);
    }
}

void CSLOLSystemTrayManager::setSystemTrayIconVisible(bool visible)
{
    if (m_systemTrayIconVisible != visible) {
        m_systemTrayIconVisible = visible;
        emit systemTrayIconVisibleChanged(visible);
    }
}