#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include "LCSTools.h"
#include "lcs/common.hpp"

int main(int argc, char *argv[])
{
    // qmlRegisterType<LCSToolsImpl>("lolcustomskin.tools", 1, 0, "LCSToolsImpl");
    qmlRegisterType<LCSTools>("lolcustomskin.tools", 1, 0, "LCSTools");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    app.setOrganizationName("moonshadow565");
    app.setOrganizationDomain("lcs");
    app.setApplicationName("lolcustomskin-manager");
    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::SystemScope, QCoreApplication::applicationDirPath());
    QSettings::setDefaultFormat(QSettings::Format::IniFormat);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LCS_VERSION", QVariant(LCS::VERSION));
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
