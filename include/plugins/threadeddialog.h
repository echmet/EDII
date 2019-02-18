#ifndef ECHMET_EDII_THREADEDDIALOG_H
#define ECHMET_EDII_THREADEDDIALOG_H

#include "uiplugin.h"
#include "pluginhelpers_p.h"

#include <cassert>
#include <functional>
#include <memory>
#include <QApplication>
#include <QMutex>
#include <QWaitCondition>
#include <QMetaMethod>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

class ThreadedDialogBase
{
public:
  explicit ThreadedDialogBase(UIPlugin *plugin) :
    m_plugin{plugin},
    m_firstDisplay{true}
  {}

  ThreadedDialogBase(const ThreadedDialogBase &other) = delete;
  ThreadedDialogBase & operator=(const ThreadedDialogBase &other) = delete;

  virtual ~ThreadedDialogBase() {}

  void create()
  {
    if (qApp->thread() == QThread::currentThread())
      initialize();
    else {
      int mthIdx = m_plugin->metaObject()->indexOfMethod("createInstance(ThreadedDialogBase*)");
      QMetaMethod mth = m_plugin->metaObject()->method(mthIdx);

      m_lock.lock();
      mth.invoke(m_plugin, Qt::QueuedConnection, Q_ARG(void *, this));
      m_barrier.wait(&m_lock);
      m_lock.unlock();
    }
  }
  
  int execute()
  {
    if (qApp->thread() == QThread::currentThread()) {
      process();
      return m_dlgRet;
    }

    int mthIdx = m_plugin->metaObject()->indexOfMethod("display(ThreadedDialogBase*)");
    QMetaMethod mth = m_plugin->metaObject()->method(mthIdx);

    m_lock.lock();
    mth.invoke(m_plugin, Qt::QueuedConnection, Q_ARG(void *, this));
    m_barrier.wait(&m_lock);
    m_lock.unlock();

    return m_dlgRet;
  }

protected:
  virtual void initialize() = 0;
  virtual void process() = 0;

  UIPlugin *const m_plugin;
  int m_dlgRet;
  bool m_firstDisplay;

  QMutex m_lock;
  QWaitCondition m_barrier;

  friend void UIPlugin::createInstance(ThreadedDialogBase *);
  friend void UIPlugin::display(ThreadedDialogBase *);
};

template <typename DialogType>
class ThreadedDialog : public ThreadedDialogBase
{
public:
  using DispFunc = std::function<DialogType * ()>;

  explicit ThreadedDialog(UIPlugin *plugin, const DispFunc &dispFunc) :
    ThreadedDialogBase{plugin},
    m_dispFunc{dispFunc},
    m_dialog{nullptr}
  {}

  explicit ThreadedDialog(UIPlugin *plugin, DispFunc &&dispFunc) :
    ThreadedDialogBase{plugin},
    m_dispFunc(std::move(dispFunc)),
    m_dialog{nullptr}
  {}

  ThreadedDialog(const ThreadedDialog &other) = delete;
  ThreadedDialog & operator=(const ThreadedDialog &other) = delete;

  virtual ~ThreadedDialog() override
  {
    if (m_dialog != nullptr)
      m_dialog->deleteLater();
  }

  DialogType * dialog()
  {
    assert(m_dialog != nullptr);
    return m_dialog;
  }

private:
  virtual void initialize() override
  {
    if (m_dialog == nullptr)
      m_dialog = m_dispFunc();
  }
  
  virtual void process() override
  {
    initialize();
    plugin::PluginHelpers::showWindowOnTop(m_dialog, m_firstDisplay);
    m_dlgRet = m_dialog->exec();
  }

  const DispFunc m_dispFunc;
  DialogType *m_dialog;
};

template <>
class ThreadedDialog<QMessageBox> : ThreadedDialogBase
{
public:
  typedef std::function<QMessageBox * ()> DispFunc;
  typedef std::unique_ptr<ThreadedDialog<QMessageBox>> Ptr;

  explicit ThreadedDialog(UIPlugin *plugin, const DispFunc &dispFunc) :
    ThreadedDialogBase{plugin},
    m_dispFunc{dispFunc},
    m_dialog{nullptr}
  {}

  ThreadedDialog(const ThreadedDialog &other) = delete;
  ThreadedDialog & operator=(const ThreadedDialog &other) = delete;

  virtual ~ThreadedDialog() override
  {
    if (m_dialog != nullptr)
      m_dialog->deleteLater();
  }

  static int displayCritical(UIPlugin *plugin, const QString &title, const QString &message)
  {
    auto dFunc = [=]() {
      return new QMessageBox{QMessageBox::Critical, title, message};
    };

    auto disp = Ptr{new ThreadedDialog<QMessageBox>{plugin, std::move(dFunc)}};
    return disp->execute();
  }

  static int displayInformation(UIPlugin *plugin, const QString &title, const QString &message)
  {
    auto dFunc = [=]() {
      return new QMessageBox{QMessageBox::Information, title, message};
    };

    auto disp = Ptr{new ThreadedDialog<QMessageBox>{plugin, std::move(dFunc)}};
    return disp->execute();
  }

  static int displayWarning(UIPlugin *plugin, const QString &title, const QString &message)
  {
    auto dFunc = [=]() {
      return new QMessageBox{QMessageBox::Warning, title, message};
    };

    auto disp = Ptr{new ThreadedDialog<QMessageBox>{plugin, std::move(dFunc)}};
    return disp->execute();
  }

private:
  virtual void initialize() override
  {
    if (m_dialog == nullptr)
      m_dialog = m_dispFunc();
  }
  
  virtual void process() override
  {
    initialize();
    plugin::PluginHelpers::showWindowOnTop(m_dialog, m_firstDisplay);
    m_dlgRet = m_dialog->exec();
  }

  const DispFunc m_dispFunc;
  QMessageBox *m_dialog;
};

#endif // ECHMET_EDII_THREADEDDIALOG_H
