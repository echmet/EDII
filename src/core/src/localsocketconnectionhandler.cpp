#include "localsocketconnectionhandler.h"
#include "dataloader.h"

#include <edii_ipc_network.h>

#define HANDLING_TIMEOUT 5000

bool finalize(QLocalSocket *socket)
{
  do {
    if (!socket->waitForBytesWritten(HANDLING_TIMEOUT))
      return false;
  } while (socket->bytesToWrite());
  return true;
}

/*
#ifdef Q_OS_WIN
  #define FINALIZE(socket) \
    do { \
      socket->waitForBytesWritten(HANDLING_TIMEOUT) \
    } (socket->bytesToWrite())
    if (socket->bytesToWrite()) \
      return socket->waitForBytesWritten(); \
    return true;
#else
   #define FINALIZE(socket) \
     return socket->waitForBytesWritten()
#endif // Q_OS_WIN
*/

#define INIT_RESPONSE(packet, rtype, s) \
  packet.magic = EDII_IPCS_PACKET_MAGIC; \
  packet.responseType = rtype; \
  packet.status = s

#define WAIT_FOR_DATA(socket) \
  if (!socket->bytesAvailable()) { \
    if (!socket->waitForReadyRead(1000)) { \
      qWarning() << "Timed out while waiting for data block"; \
      return false; \
    } \
  }

#define WRITE_CHECKED(socket, payload) \
  if (!writeSegmented(socket, payload.data(), payload.size())) { \
    qWarning() << "Failed to send payload:" << socket->errorString(); \
    return false; \
  }

#define WRITE_CHECKED_RAW(socket, payload) \
  if (!writeSegmented(socket, reinterpret_cast<const char*>(&payload), static_cast<qint64>(sizeof(payload)))) { \
    qWarning() << "Failed to send raw payload:" << socket->errorString(); \
    return false; \
  }

#define WRITE_RAW(socket, payload) \
  writeSegmented(socket, reinterpret_cast<const char*>(&payload), static_cast<qint64>(sizeof(payload)))

template <typename P>
bool checkSig(const P &packet)
{
  return packet->magic == EDII_IPCS_PACKET_MAGIC;
}

template <typename P, typename R>
bool checkSig(const P &packet, const R requestType)
{
  return checkSig(packet) && packet->requestType == requestType;
}

static
bool writeSegmented(QLocalSocket *socket, const char *payload, const qint64 bytesToWrite)
{
#ifdef Q_OS_WIN
  const qint64 MAX_SEGMENT_SIZE = 4095;
  qint64 written = 0;

  while (written < bytesToWrite) {
    const qint64 writeMax = (bytesToWrite - written) > MAX_SEGMENT_SIZE ? MAX_SEGMENT_SIZE : (bytesToWrite - written);
    const qint64 w = socket->write(payload + written, writeMax);
    if (w < 1)
      return false;
    written += w;

    if (!socket->waitForBytesWritten())
      return false;
  }

  return true;
#else
  if (bytesToWrite == 0)
    return true;
  return socket->write(payload, bytesToWrite) == bytesToWrite;
#endif // Q_OS_WIN
}

static
bool reportError(QLocalSocket *socket, const EDII_IPCSockResponseType rtype, const QString &message)
{
  EDII_IPCSockResponseHeader resp;
  QByteArray messageRaw(message.toUtf8());

  INIT_RESPONSE(resp, rtype, EDII_IPCS_FAILURE);
  resp.errorLength = messageRaw.size();

  WRITE_CHECKED_RAW(socket, resp);
  WRITE_CHECKED(socket, messageRaw);
  return finalize(socket);
}

static
bool readBlock(QLocalSocket *socket, QByteArray &buffer, qint64 size)
{
  buffer.resize(size);
  qint64 read = 0;
  while (read < size) {
    if (socket->state() != QLocalSocket::ConnectedState)
      return false;

    qint64 r = socket->read(buffer.data() + read, size);
    if (r < 0) {
      qWarning() << "Unable to read block:"<< socket->errorString();
      return false;
    }
    read += r;
  }

  return true;
}

static
bool readHeader(QLocalSocket *socket, EDII_IPCSockRequestType &reqType)
{
  static const qint64 HEADER_SIZE = sizeof(EDII_IPCSockRequestHeader);
  QByteArray headerRaw;

  if (!readBlock(socket, headerRaw, HEADER_SIZE)) {
    qWarning() << "Cannot read request header";
    return false;
  }

  const EDII_IPCSockRequestHeader *header = reinterpret_cast<const EDII_IPCSockRequestHeader *>(headerRaw.data());

  if (!checkSig(header))
    return false;

  reqType = static_cast<EDII_IPCSockRequestType>(header->requestType);

  return true;
}

LocalSocketConnectionHandler::LocalSocketConnectionHandler(const quintptr sockDesc, const DataLoader &loader) :
  h_loader{loader},
  m_sockDesc{sockDesc}
{
}

void LocalSocketConnectionHandler::handleConnection(QLocalSocket *socket)
{
  if (!socket->bytesAvailable()) {
    if (!socket->waitForReadyRead(HANDLING_TIMEOUT))
      return;
  }

  EDII_IPCSockRequestType reqType;
  if (!readHeader(socket, reqType))
    return;

  switch (reqType) {
  case EDII_REQUEST_SUPPORTED_FORMATS:
    respondSupportedFormats(socket);
    break;
  case EDII_REQUEST_LOAD_DATA:
    respondLoadData(socket);
    break;
  case EDII_REQUEST_ABI_VERSION:
    respondABIVersion(socket);
    break;
  default:
    return;
  }
}

bool LocalSocketConnectionHandler::respondABIVersion(QLocalSocket *socket)
{
  EDII_IPCSockResponseABIVersion resp;

  INIT_RESPONSE(resp, EDII_RESPONSE_ABI_VERSION, EDII_IPCS_SUCCESS);

  resp.major = EDII_ABI_VERSION_MAJOR;
  resp.minor = EDII_ABI_VERSION_MINOR;

  WRITE_CHECKED_RAW(socket, resp);

  return finalize(socket);
}

bool LocalSocketConnectionHandler::respondLoadData(QLocalSocket *socket)
{
  static const qint64 REQ_DESC_SIZE = sizeof(EDII_IPCSockLoadDataRequestDescriptor);
  QString formatTag;
  QString path;

  /* Read request descriptor */
  WAIT_FOR_DATA(socket);
  EDII_IPCSockLoadDataRequestDescriptor *reqDesc;
  QByteArray reqDescRaw;
  if (!readBlock(socket, reqDescRaw, REQ_DESC_SIZE)) {
    qWarning() << "Cannot read load data descriptor";
    return false;
  }
  reqDesc = reinterpret_cast<EDII_IPCSockLoadDataRequestDescriptor *>(reqDescRaw.data());
  if (!checkSig(reqDesc, EDII_REQUEST_LOAD_DATA_DESCRIPTOR)) {
    qWarning() << "Invalid load data descriptor signature";
    return false;
  }
  if (reqDesc->tagLength < 1) {
    qWarning() << "Invalid length of formatTag";
    reportError(socket, EDII_RESPONSE_LOAD_DATA_HEADER, "Invalid length of formatTag");
    return false;
  }

  /* Read tag */
  WAIT_FOR_DATA(socket)
  QByteArray tagRaw;
  if (!readBlock(socket, tagRaw, reqDesc->tagLength)) {
    qWarning() << "Cannot read format tag";
    return false;
  }
  formatTag = QString::fromUtf8(tagRaw);

  switch (reqDesc->mode) {
  case EDII_IPCS_LOAD_FILE:
  case EDII_IPCS_LOAD_HINT:
  {
    if (reqDesc->filePathLength > 0) {
      WAIT_FOR_DATA(socket);
      QByteArray pathRaw;
      if (!readBlock(socket, pathRaw, reqDesc->filePathLength)) {
        qWarning() << "Cannot read file path";
        return false;
      }

      path = QString::fromUtf8(pathRaw);
    } else {
      reportError(socket, EDII_RESPONSE_LOAD_DATA_HEADER, "Invalid file path length");
      return false;
    }
  }
    break;
  case EDII_IPCS_LOAD_INTERACTIVE:
    break;
  default:
    reportError(socket, EDII_RESPONSE_LOAD_DATA_HEADER, "Invalid load mode");
    return false;
  }

  DataLoader::LoadedPack result;
  switch (reqDesc->mode) {
  case EDII_IPCS_LOAD_INTERACTIVE:
    result = h_loader.loadData(formatTag, reqDesc->loadOption);
    break;
  case EDII_IPCS_LOAD_HINT:
    result = h_loader.loadDataHint(formatTag, path, reqDesc->loadOption);
    break;
  case EDII_IPCS_LOAD_FILE:
    result = h_loader.loadDataPath(formatTag, path, reqDesc->loadOption);
    break;
  }

  /* We have the data (or a failure), report it back */
  EDII_IPCSockResponseHeader respHeader;
  if (!std::get<1>(result)) {
    INIT_RESPONSE(respHeader, EDII_RESPONSE_LOAD_DATA_HEADER, EDII_IPCS_FAILURE);
    const QByteArray error = std::get<2>(result).toUtf8();

    respHeader.errorLength = error.size();

    WRITE_RAW(socket, respHeader);

    socket->write(error);
    return finalize(socket);
  }

  const std::vector<Data> &data = std::get<0>(result);
  INIT_RESPONSE(respHeader, EDII_RESPONSE_LOAD_DATA_HEADER, EDII_IPCS_SUCCESS);
  respHeader.items = data.size();
  respHeader.errorLength = 0;
  WRITE_CHECKED_RAW(socket, respHeader);

  for (const auto &item : data) {
    EDII_IPCSockLoadDataResponseDescriptor respDesc;
    INIT_RESPONSE(respDesc, EDII_RESPONSE_LOAD_DATA_DESCRIPTOR, EDII_IPCS_SUCCESS);

    QByteArray nameBytes = item.name.toUtf8();
    QByteArray dataIdBytes = item.dataId.toUtf8();
    QByteArray pathBytes = item.path.toUtf8();;
    QByteArray xDescBytes = item.xDescription.toUtf8();
    QByteArray yDescBytes = item.yDescription.toUtf8();
    QByteArray xUnitBytes = item.xUnit.toUtf8();
    QByteArray yUnitBytes = item.yUnit.toUtf8();

    respDesc.nameLength = nameBytes.size();
    respDesc.dataIdLength = dataIdBytes.size();
    respDesc.pathLength = pathBytes.size();
    respDesc.xDescriptionLength = xDescBytes.size();
    respDesc.yDescriptionLength = yDescBytes.size();
    respDesc.xUnitLength = xUnitBytes.size();
    respDesc.yUnitLength = yUnitBytes.size();
    respDesc.datapointsLength = item.datapoints.size();

    WRITE_CHECKED_RAW(socket, respDesc);
    WRITE_CHECKED(socket, nameBytes);
    WRITE_CHECKED(socket, dataIdBytes);
    WRITE_CHECKED(socket, pathBytes);
    WRITE_CHECKED(socket, xDescBytes);
    WRITE_CHECKED(socket, yDescBytes);
    WRITE_CHECKED(socket, xUnitBytes);
    WRITE_CHECKED(socket, yUnitBytes);

   for (const auto &dp : item.datapoints) {
      EDII_IPCSockDatapoint respDp;

      respDp.x = std::get<0>(dp);
      respDp.y = std::get<1>(dp);

      WRITE_CHECKED_RAW(socket, respDp);
    }
  }
  return finalize(socket);
}

bool LocalSocketConnectionHandler::respondSupportedFormats(QLocalSocket *socket)
{
  const QVector<FileFormatInfo> ffiVec = h_loader.supportedFileFormats();

  EDII_IPCSockResponseHeader responseHeader;
  INIT_RESPONSE(responseHeader, EDII_RESPONSE_SUPPORTED_FORMAT_HEADER, EDII_IPCS_SUCCESS);
  responseHeader.items = ffiVec.size();
  responseHeader.errorLength = 0;

  WRITE_CHECKED_RAW(socket, responseHeader);

  for (const auto &ffi : ffiVec) {
    EDII_IPCSockSupportedFormatResponseDescriptor desc;
    INIT_RESPONSE(desc, EDII_RESPONSE_SUPPORTED_FORMAT_DESCRIPTOR, EDII_IPCS_SUCCESS);

    QByteArray longDescription = ffi.longDescription.toUtf8();
    QByteArray shortDescription = ffi.shortDescription.toUtf8();
    QByteArray tag = ffi.tag.toUtf8();

    desc.longDescriptionLength = longDescription.size();
    desc.shortDescriptionLength = shortDescription.size();
    desc.tagLength = tag.size();
    if (ffi.loadOptions.size() > 1)
      desc.loadOptionsLength = ffi.loadOptions.size();
    else
      desc.loadOptionsLength = 0;

    /* Send the response descriptor */
    WRITE_CHECKED_RAW(socket, desc);

    /* Send the actual payload */
    WRITE_CHECKED(socket, longDescription);
    WRITE_CHECKED(socket, shortDescription);
    WRITE_CHECKED(socket, tag);

    if (ffi.loadOptions.size() > 1) {
      for (const QString &option : ffi.loadOptions) {
        EDII_IPCSockLoadOptionDescriptor loDesc;
        INIT_RESPONSE(loDesc, EDII_RESPONSE_LOAD_OPTION_DESCRIPTOR, EDII_IPCS_SUCCESS);

        QByteArray optionBA = option.toUtf8();
        loDesc.optionLength = optionBA.size();
        WRITE_CHECKED_RAW(socket, loDesc);

        WRITE_CHECKED(socket, optionBA);
      }
    }
  }
  return finalize(socket);
}

void LocalSocketConnectionHandler::run()
{
  QLocalSocket socket{};
  socket.setSocketDescriptor(m_sockDesc);

  handleConnection(&socket);
}
