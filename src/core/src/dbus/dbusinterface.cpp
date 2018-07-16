#include "dbusinterface.h"

DBusInterface::DBusInterface(QObject *parent) :
  QObject(parent)
{
}

DBusInterface::~DBusInterface()
{
}

EDII::IPCQtDBus::ABIVersion DBusInterface::abiVersion()
{
  return { EDII_ABI_VERSION_MAJOR, EDII_ABI_VERSION_MINOR };
}

EDII::IPCQtDBus::DataPack DBusInterface::loadData(const QString &formatTag, const int loadOption)
{
  EDII::IPCQtDBus::DataPack pack;

  emit loadDataForwarder(pack, formatTag, LoadMode::INTERACTIVE, "", loadOption);

  return pack;
}

EDII::IPCQtDBus::DataPack DBusInterface::loadDataHint(const QString &formatTag, const QString &hint, const int loadOption)
{
  EDII::IPCQtDBus::DataPack pack;

  emit loadDataForwarder(pack, formatTag, LoadMode::HINT, hint, loadOption);

  return pack;
}

EDII::IPCQtDBus::DataPack DBusInterface::loadDataFile(const QString &formatTag, const QString &filePath, const int loadOption)
{
  EDII::IPCQtDBus::DataPack pack;

  emit loadDataForwarder(pack, formatTag, LoadMode::FILE, filePath, loadOption);

  return pack;
}

EDII::IPCQtDBus::SupportedFileFormatVec DBusInterface::supportedFileFormats()
{
  EDII::IPCQtDBus::SupportedFileFormatVec vec;

  emit supportedFileFormatsForwarder(vec);

  return vec;
}
