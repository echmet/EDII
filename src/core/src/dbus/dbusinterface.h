#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#ifdef ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED

#include <edii_ipc_qtdbus.h>
#include <QObject>

class DBusInterface : public QObject {
  Q_OBJECT

  Q_CLASSINFO("ECHMET Data Import Infrastructure D-Bus interface", "edii.loader")

public:
  enum class LoadMode {
    INTERACTIVE,
    HINT,
    FILE
  };
  Q_ENUM(LoadMode)

  explicit DBusInterface(QObject *parent = nullptr);
  virtual ~DBusInterface();

public slots:
  EDII::IPCQtDBus::ABIVersion abiVersion();
  EDII::IPCQtDBus::DataPack loadData(const QString &formatTag, const int loadOption);
  EDII::IPCQtDBus::DataPack loadDataHint(const QString &formatTag, const QString &hint, const int loadOption);
  EDII::IPCQtDBus::DataPack loadDataFile(const QString &formatTag, const QString &filePath, const int loadOption);
  EDII::IPCQtDBus::SupportedFileFormatVec supportedFileFormats();

signals:
  void loadDataForwarder(EDII::IPCQtDBus::DataPack &pack, const QString &formatTag, const LoadMode mode, const QString &modeParam, const int loadOption);
  void supportedFileFormatsForwarder(EDII::IPCQtDBus::SupportedFileFormatVec &supportedFileFormats);
};

#endif // ECHMET_EDII_IPCINTERFACE_QTDBUS_ENABLED

#endif // DBUSINTERFACE_H
