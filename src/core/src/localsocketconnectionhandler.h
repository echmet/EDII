#ifndef LOCALSOCKETCONNECTIONHANDLER_H
#define LOCALSOCKETCONNECTIONHANDLER_H

#include <QLocalSocket>
#include <QRunnable>

class DataLoader;

class LocalSocketConnectionHandler : public QRunnable
{
public:
  LocalSocketConnectionHandler(const quintptr sockDesc, const DataLoader &loader);

private:
  void handleConnection(QLocalSocket *socket);
  bool respondABIVersion(QLocalSocket *socket);
  bool respondLoadData(QLocalSocket *socket);
  bool respondSupportedFormats(QLocalSocket *socket);
  virtual void run() override;

  const DataLoader &h_loader;
  const quintptr m_sockDesc;
};

#endif // LOCALSOCKETCONNECTIONHANDLER_H
