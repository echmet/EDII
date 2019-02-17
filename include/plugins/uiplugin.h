#ifndef ECHMET_EDII_UIBACKEND_H
#define ECHMET_EDII_UIBACKEND_H

#include <QObject>

class ThreadedDialogBase;

class UIPlugin : public QObject
{
  Q_OBJECT
public:
  static void initialize();
  static UIPlugin * instance();

public slots:
  virtual void createInstance(ThreadedDialogBase *disp);
  virtual void display(ThreadedDialogBase *disp);

private:
  explicit UIPlugin(QObject *parent = nullptr);

  static UIPlugin *s_me;
};

#endif // ECHMET_EDII_UIBACKEND_H
