#ifndef LCSUTILS_H
#define LCSUTILS_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

class LCSUtils : public QObject
{
    Q_OBJECT
public:
    explicit LCSUtils(QObject* engine = nullptr);
    Q_INVOKABLE QString fromFile(QString file);
    Q_INVOKABLE QString toFile(QString file);
    Q_INVOKABLE QString checkGamePath(QString path);
    Q_INVOKABLE void disableSS(QQuickWindow* window, bool disable);
    static void allowDD();
private:

};

#endif // LCSUTILS_H
