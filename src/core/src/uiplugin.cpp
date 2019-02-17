#include <plugins/uiplugin.h>
#include <plugins/threadeddialog.h>

UIPlugin *UIPlugin::s_me{nullptr};

void UIPlugin::initialize()
{
  if (s_me == nullptr) {
    s_me = new UIPlugin{};
  }
}

UIPlugin * UIPlugin::instance()
{
  return s_me;
}

UIPlugin::UIPlugin(QObject *parent) :
  QObject{parent}
{
}

void UIPlugin::createInstance(ThreadedDialogBase *disp)
{
  disp->initialize();
  disp->m_barrier.wakeAll();
}

void UIPlugin::display(ThreadedDialogBase *disp)
{
  disp->process();
  disp->m_barrier.wakeAll();
}
