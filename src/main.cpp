#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include "LCSTools.h"

int main(int argc, char *argv[])
{
    // qmlRegisterType<LCSToolsImpl>("lolcustomskin.tools", 1, 0, "LCSToolsImpl");
    qmlRegisterType<LCSTools>("lolcustomskin.tools", 1, 0, "LCSTools");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);

    app.setOrganizationName("moonshadow565");
    app.setOrganizationDomain("lcs");
    app.setApplicationName("LCS-tools");
    QSettings::setPath(QSettings::Format::IniFormat, QSettings::Scope::SystemScope, QCoreApplication::applicationDirPath());
    QSettings::setDefaultFormat(QSettings::Format::IniFormat);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
