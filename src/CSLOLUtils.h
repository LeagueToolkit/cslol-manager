#ifndef CSLOLUTILS_H
#define CSLOLUTILS_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

class CSLOLUtils : public QObject {
    Q_OBJECT
public:
    explicit CSLOLUtils(QObject* engine = nullptr);
    Q_INVOKABLE QString fromFile(QString file);
    Q_INVOKABLE QString toFile(QString file);
    Q_INVOKABLE QString checkGamePath(QString path);
    Q_INVOKABLE QString detectGamePath();

    static QString isPlatformUnsuported();
    static void relaunchAdmin(int argc, char *argv[]);
private:
};

#endif  // CSLOLUTILS_H
