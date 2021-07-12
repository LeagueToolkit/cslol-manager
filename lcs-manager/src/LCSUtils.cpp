#include "LCSUtils.h"

#ifdef WIN32
extern "C" {
    extern int __stdcall SetWindowDisplayAffinity(WId hWnd, unsigned dwAffinity);
}
#endif

LCSUtils::LCSUtils(QQmlApplicationEngine *engine) : QObject(engine), engine_(engine) {}

void LCSUtils::disableSS(QQuickWindow* window, bool disable) {
#ifdef WIN32
    auto const hwnd = window->winId();
    SetWindowDisplayAffinity(hwnd, disable ? 1 : 0);
#endif
}
