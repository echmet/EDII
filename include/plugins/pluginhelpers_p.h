#ifndef ECHMET_EDII_BACKENDHELPERS_P_H
#define ECHMET_EDII_BACKENDHELPERS_P_H

#ifdef Q_OS_WIN
#include <Windows.h>
#include <QTimer>
#include <QDialog>
#else
#endif // Q_OS_WIN

#include <QWidget>

namespace plugin {

class PluginHelpers {
public:
  static void showWindowOnTop(QWidget *widget, bool &firstDisplay)
  {
  #ifdef Q_OS_WIN
    if (firstDisplay) {
      auto dlg = qobject_cast<QDialog *>(widget);
      if (dlg != nullptr) {
        QTimer::singleShot(1, [widget]() { widget->close(); });
        dlg->exec();
      }
      firstDisplay = false;
    }

    RECT rect;

    HWND hWnd = (HWND)widget->winId();
    if (hWnd == NULL)
      return;

    if (GetWindowRect(hWnd, &rect) == FALSE)
      return;

    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    widget->raise();
    widget->show();
    widget->activateWindow();

    SetActiveWindow(hWnd);

  #else
    Q_UNUSED(widget)
    Q_UNUSED(firstDisplay)
  #endif // Q_OS_
  }
};

} //namespace backend

#endif // ECHMET_EDII_PLUGINHELPERS_P_H
