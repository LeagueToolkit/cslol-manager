#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>

#include "CSLOLTools.h"
#include "CSLOLUtils.h"
#include "CSLOLVersion.h"

int main(int argc, char *argv[]) {
    CSLOLUtils::relaunchAdmin(argc, argv);

    qmlRegisterType<CSLOLTools>("customskinlol.tools", 1, 0, "CSLOLTools");

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#else
    if (QFileInfo info("opengl.txt"); info.exists()) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    }
#endif
    QGuiApplication app(argc, argv);
    app.setOrganizationName("moonshadow565");
    app.setOrganizationDomain("cslol");
    app.setApplicationName("customskinlol-manager");
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    QSettings::setPath(QSettings::Format::IniFormat,
                       QSettings::Scope::SystemScope,
                       QCoreApplication::applicationDirPath());
    QSettings::setDefaultFormat(QSettings::Format::IniFormat);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("CSLOLUtils", new CSLOLUtils(&engine));
    engine.rootContext()->setContextProperty("CSLOL_VERSION", CSLOL::VERSION);
    engine.rootContext()->setContextProperty("CSLOL_COMMIT", CSLOL::COMMIT);
    engine.rootContext()->setContextProperty("CSLOL_DATE", CSLOL::DATE);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QFile fontfile(":/fontawesome-webfont.ttf");
    fontfile.open(QFile::OpenModeFlag::ReadOnly);
    QFontDatabase::addApplicationFontFromData(fontfile.readAll());
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
