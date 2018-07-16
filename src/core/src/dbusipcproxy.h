#ifndef DBUSIPCPROXY_H
#define DBUSIPCPROXY_H

#ifdef ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED

#include "ipcproxy.h"
#include "dbus/dbusinterface.h"

class LoaderAdaptor;

class DBusIPCProxy : public IPCProxy
{
public:
  explicit DBusIPCProxy(DataLoader *loader, QObject *parent = nullptr);
  virtual ~DBusIPCProxy();

private:
  void unprovision();

  DBusInterface *m_interface;
  LoaderAdaptor *m_interfaceAdaptor;

private slots:
  void onSupportedFileFormats(EDII::IPCQtDBus::SupportedFileFormatVec &supportedFileFormats);
  void onLoadData(EDII::IPCQtDBus::DataPack &pack, const QString &formatTag, const DBusInterface::LoadMode loadMode, const QString &modeParam, const int loadOption);
};

#endif // ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED

#endif // DBUSIPCPROXY_H
