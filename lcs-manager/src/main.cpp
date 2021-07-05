#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QFontDatabase>
#include <QFile>
#include "LCSTools.h"
#include "lcs/common.hpp"

int main(int argc, char *argv[])
{
    // qmlRegisterType<LCSToolsImpl>("lolcustomskin.tools", 1, 0, "LCSToolsImpl");
    qmlRegisterType<LCSTools>("lolcustomskin.tools", 1, 0, "LCSTools");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    app.setOrganizationName("moonshadow565");
    app.setOrganizationDomain("lcs");
    app.setApplicationName("lolcustomskin-manager");
    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::SystemScope, QCoreApplication::applicationDirPath());
    QSettings::setDefaultFormat(QSettings::Format::IniFormat);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LCS_VERSION", LCS::VERSION);
    engine.rootContext()->setContextProperty("LCS_COMMIT", LCS::COMMIT);
    engine.rootContext()->setContextProperty("LCS_DATE", LCS::DATE);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QFile fontfile(":/fontawesome-webfont.ttf");
    fontfile.open(QFile::OpenModeFlag::ReadOnly);
    QFontDatabase::addApplicationFontFromData(fontfile.readAll());
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
