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
    explicit LCSUtils(QQmlApplicationEngine* engine = nullptr);
    Q_INVOKABLE void disableSS(QQuickWindow* window, bool disable);
private:
    QQmlApplicationEngine* engine_;

};

#endif // LCSUTILS_H
