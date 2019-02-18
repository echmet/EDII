#ifndef ECHMET_EDII_BACKENDHELPERS_P_H
#define ECHMET_EDII_BACKENDHELPERS_P_H

#ifdef Q_OS_WIN
#include <Windows.h>
#include <QApplication>
#else
#endif // Q_OS_WIN

#include <QWidget>

namespace plugin {

class PluginHelpers {
public:
  static void showWindowOnTop(QWidget *widget)
  {
  #ifdef Q_OS_WIN
    RECT rect;

    HWND hWnd = (HWND)widget->winId();
    if (hWnd == NULL)
      return;

    if (GetWindowRect(hWnd, &rect) == FALSE)
      return;

    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    qApp->processEvents();
    SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    widget->raise();
    widget->show();
    widget->activateWindow();
    qApp->processEvents();

  #else
    Q_UNUSED(widget)
  #endif // Q_OS_
  }
};

} //namespace backend

#endif // ECHMET_EDII_PLUGINHELPERS_P_H
