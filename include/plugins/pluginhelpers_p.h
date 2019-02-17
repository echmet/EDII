#ifndef ECHMET_EDII_BACKENDHELPERS_P_H
#define ECHMET_EDII_BACKENDHELPERS_P_H

#ifdef Q_OS_WIN
#include <Windows.h>
#else
#endif // Q_OS_WIN

#include <QWidget>

namespace plugin {

class PluginHelpers {
public:
  static void showWindowOnTop(QWidget *widget)
  {
  #ifdef Q_OS_WIN
    HWND hWnd =(HWND)widget->winId();
    RECT rect;

    if (GetWindowRect(hWnd, &rect) == FALSE)
      return;

    SetWindowPos(hWnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0);
  #else
    Q_UNUSED(widget)
  #endif // Q_OS_
  }
};

} //namespace backend

#endif // ECHMET_EDII_PLUGINHELPERS_P_H
