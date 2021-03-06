#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <iostream>
#include <plugins/uiplugin.h>

#include "dataloader.h"
#include "ipcproxy.h"

#ifdef ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED
  #include "dbusipcproxy.h"
#endif // ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED
  #include "localsocketipcproxy.h"

#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
  #define _PID_T qint64
  #include <errno.h>
  #include <signal.h>
  #include <unistd.h>
#elif defined Q_OS_WIN
  #define _PID_T DWORD
  #include <Windows.h>
  #include <QAbstractEventDispatcher>
  #include <QAbstractNativeEventFilter>
#endif // Q_OS_


#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
void catchTermination()
{
  auto handler = [](int sig) -> void {
    Q_UNUSED(sig);
    QMetaObject::invokeMethod(QApplication::instance(), "quit", Qt::QueuedConnection);
  };

  sigset_t blocking_mask;
  sigemptyset(&blocking_mask);
  sigaddset(&blocking_mask, SIGTERM);

  struct sigaction sa;
  sa.sa_handler = handler;
  sa.sa_mask    = blocking_mask;
  sa.sa_flags   = 0;

  sigaction(SIGTERM, &sa, nullptr);
}
#elif defined Q_OS_WIN
class WindowsEventFilter : public QAbstractNativeEventFilter {
  bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
};

bool WindowsEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result) {
  Q_UNUSED(eventType);
  Q_UNUSED(result);

  MSG* msg = (MSG*)(message);
  if((msg->message == WM_CLOSE) && (msg->hwnd == 0)){
    QMetaObject::invokeMethod(QApplication::instance(), "quit", Qt::QueuedConnection);
    return true;
  }

  return false;
}

void catchTermination()
{
  QCoreApplication::eventDispatcher()->installNativeEventFilter(new WindowsEventFilter());
}

#endif // Q_OS_


IPCProxy * provisionIPCInterface(DataLoader *loader)
{
#ifdef ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED
  try {
    return new DBusIPCProxy{loader};
  } catch (std::runtime_error &ex) {
    qWarning() << "Cannot register D-Bus iterface (" << ex.what() << "), falling back to local socket";
  }
#endif // ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED
  return new LocalSocketIPCProxy{loader};
}

void watchForMainProcess(QTimer *&timer, const char *pidStr)
{
  bool ok;
  _PID_T pid = QString(pidStr).trimmed().toLongLong(&ok);

  if (!ok)
    return;

  timer = new QTimer{};

  timer->setInterval(1000);
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
  QObject::connect(timer, &QTimer::timeout, [timer, pid]() {
    int ret = kill(pid, 0);

    if (ret == 0)
      return;
    if (ret == -1 && errno == EPERM)
        return;
    else {
      timer->stop();
      QApplication::quit();
    }
  });
#elif defined Q_OS_WIN
  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (hProcess == NULL)
      return;

  QObject::connect(timer, &QTimer::timeout, [timer, hProcess]() {
    DWORD exitCode = 0;
    BOOL bRet = GetExitCodeProcess(hProcess, &exitCode);
    if (bRet == FALSE || exitCode != STILL_ACTIVE) {
      timer->stop();
      QApplication::quit();
    }
  });
#endif // Q_OS_

  timer->start();
}

int main(int argc, char *argv[])
{
  QApplication a{argc, argv};
  IPCProxy *proxy;
  QTimer *timer = nullptr;

  if (argc > 1)
    watchForMainProcess(timer, argv[1]);

  a.setQuitOnLastWindowClosed(false); /* We want our plugins to be able use Qt GUI and not kill us when they close their UI elements */
  catchTermination();
  UIPlugin::initialize();

  try {
    DataLoader loader{};

    try {
      proxy = provisionIPCInterface(&loader);
    } catch (std::exception &ex) {
      std::cerr << "Cannot start EDII service: " << ex.what() << std::endl;

      return EXIT_FAILURE;
    }

    int ret = a.exec();

    if (timer != nullptr && timer->isActive())
      timer->stop();
    delete timer;

    delete proxy;

    return ret;
  } catch (std::runtime_error &ex) {
    QMessageBox msg{QMessageBox::Critical, QObject::tr("Cannot start EDII service"), ex.what(), QMessageBox::Ok};
    QObject::connect(&msg, &QMessageBox::finished, qApp, &QApplication::quit);

    msg.show();
    a.exec();

    return EXIT_FAILURE;
  }
}
