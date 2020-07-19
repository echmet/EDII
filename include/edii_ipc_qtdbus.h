#ifndef ECHMET_EDII_IPC_QTDBUS_H
#define ECHMET_EDII_IPC_QTDBUS_H

#include "edii_ipc_common.h"

#include <QObject>
#include <QVector>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMetaType>

#define EDII_DBUS_SERVICE_NAME "cz.cuni.natur.echmet.edii"
#define EDII_DBUS_OBJECT_PATH "/EDII"

namespace EDII {
namespace IPCQtDBus {

class Datapoint {
public:
  explicit Datapoint() :
    x{0.0},
    y{0.0}
  {}

  double x;
  double y;

  friend QDBusArgument & operator<<(QDBusArgument &argument, const Datapoint &result)
  {
    argument.beginStructure();
    argument << result.x;
    argument << result.y;
    argument.endStructure();

    return argument;
  }

  friend const QDBusArgument & operator>>(const QDBusArgument &argument, Datapoint &result)
  {
    argument.beginStructure();
    argument >> result.x;
    argument >> result.y;
    argument.endStructure();

    return argument;
  }
};

}
}
Q_DECLARE_METATYPE(EDII::IPCQtDBus::Datapoint)

namespace EDII {
namespace IPCQtDBus {

class Data {
public:
  QString path;
  QString dataId;
  QString name;

  QString xDescription;
  QString yDescription;
  QString xUnit;
  QString yUnit;

  QVector<Datapoint> datapoints;

  friend QDBusArgument & operator<<(QDBusArgument &argument, const Data &result)
  {
    argument.beginStructure();
    argument << result.path;
    argument << result.dataId;
    argument << result.name;
    argument << result.xDescription;
    argument << result.yDescription;
    argument << result.xUnit;
    argument << result.yUnit;
    argument << result.datapoints;
    argument.endStructure();

    return argument;
  }

  friend const QDBusArgument & operator>>(const QDBusArgument &argument, Data &result)
  {
    argument.beginStructure();
    argument >> result.path;
    argument >> result.dataId;
    argument >> result.name;
    argument >> result.xDescription;
    argument >> result.yDescription;
    argument >> result.xUnit;
    argument >> result.yUnit;
    argument >> result.datapoints;
    argument.endStructure();

    return argument;
  }
};

}
}
Q_DECLARE_METATYPE(EDII::IPCQtDBus::Data)

namespace EDII {
namespace IPCQtDBus {

class DataVec : public QVector<EDII::IPCQtDBus::Data> {
public:
  friend QDBusArgument & operator<<(QDBusArgument &argument, const DataVec &vec)
  {
    argument.beginArray(qMetaTypeId<EDII::IPCQtDBus::Data>());
    for (const auto &item : vec)
      argument << item;
    argument.endArray();

    return argument;
  }

  friend const  QDBusArgument & operator>>(const QDBusArgument &argument, DataVec &vec)
  {
    argument.beginArray();
    while (!argument.atEnd()) {
      EDII::IPCQtDBus::Data d;
      argument >> d;
      vec.append(d);
    }
    argument.endArray();

    return argument;
  }
};

}
}
Q_DECLARE_METATYPE(EDII::IPCQtDBus::DataVec)

namespace EDII {
namespace IPCQtDBus {

class DataPack {
public:
  explicit DataPack() :
    success{false},
    error{"Empty response"}
  {}

  bool success;
  QString error;
  DataVec data;

  friend QDBusArgument & operator<<(QDBusArgument &argument, const DataPack &pack)
  {
    argument.beginStructure();
    argument << pack.success;
    argument << pack.error;
    argument << pack.data;
    argument.endStructure();

    return argument;
  }

  friend const QDBusArgument & operator>>(const QDBusArgument &argument, DataPack &pack)
  {
    argument.beginStructure();
    argument >> pack.success;
    argument >> pack.error;
    argument >> pack.data;
    argument.endStructure();

    return argument;
  }
};

}
}
Q_DECLARE_METATYPE(EDII::IPCQtDBus::DataPack)

namespace EDII {
namespace IPCQtDBus {

class LoadOptionsVec : public QVector<QString>
{
public:
  friend QDBusArgument & operator<<(QDBusArgument &argument, const LoadOptionsVec &vec)
  {
    argument.beginArray(qMetaTypeId<QString>());
    for (const auto &item : vec)
      argument << item;
    argument.endArray();

    return argument;
  }

  friend const  QDBusArgument & operator>>(const QDBusArgument &argument, LoadOptionsVec &vec)
  {
    argument.beginArray();
    while (!argument.atEnd()) {
      QString d;
      argument >> d;
      vec.append(d);
    }
    argument.endArray();

    return argument;
  }
};

}
}
Q_DECLARE_METATYPE(EDII::IPCQtDBus::LoadOptionsVec)

namespace EDII {
namespace IPCQtDBus {

class SupportedFileFormat {
public:
  QString longDescription;
  QString shortDescription;
  QString tag;
  LoadOptionsVec loadOptions;

  friend QDBusArgument & operator<<(QDBusArgument &argument, const SupportedFileFormat &format)
  {
    argument.beginStructure();
    argument << format.longDescription;
    argument << format.shortDescription;
    argument << format.tag;
    argument << format.loadOptions;
    argument.endStructure();

    return argument;
  }

  friend const QDBusArgument & operator>>(const QDBusArgument &argument, SupportedFileFormat &format)
  {
    argument.beginStructure();
    argument >> format.longDescription;
    argument >> format.shortDescription;
    argument >> format.tag;
    argument >> format.loadOptions;
    argument.endStructure();

    return argument;
  }
};

}
}
Q_DECLARE_METATYPE(EDII::IPCQtDBus::SupportedFileFormat)

namespace EDII {
namespace IPCQtDBus {

class SupportedFileFormatVec : public QVector<SupportedFileFormat>
{
  friend QDBusArgument & operator<<(QDBusArgument &argument, const SupportedFileFormatVec &vec)
  {
    argument.beginArray(qMetaTypeId<SupportedFileFormat>());
    for (const auto &item : vec)
      argument << item;

    argument.endArray();

    return argument;
  }

  friend const QDBusArgument & operator>>(const QDBusArgument &argument, SupportedFileFormatVec &vec)
  {
    argument.beginArray();

    while (!argument.atEnd()) {
      SupportedFileFormat sff;
      argument >> sff;
      vec.append(sff);
    }
    argument.endArray();

    return argument;
  }
};

}
}
Q_DECLARE_METATYPE(EDII::IPCQtDBus::SupportedFileFormatVec)

namespace EDII {
namespace IPCQtDBus {

class ABIVersion {
public:
  int32_t ver_major;
  int32_t ver_minor;

  friend QDBusArgument & operator<<(QDBusArgument &argument, const ABIVersion &abiVer)
  {
    argument.beginStructure();

    argument << abiVer.ver_major;
    argument << abiVer.ver_minor;

    argument.endStructure();

    return argument;
  }

  friend const QDBusArgument & operator>>(const QDBusArgument &argument, ABIVersion &abiVer)
  {
    argument.beginStructure();

    argument >> abiVer.ver_major;
    argument >> abiVer.ver_minor;

    argument.endStructure();

    return argument;
  }
};

} // namespace IPCQtDBus
} // namespace EDII

Q_DECLARE_METATYPE(EDII::IPCQtDBus::ABIVersion)

namespace EDII {
namespace IPCQtDBus {

class DBusMetaTypesRegistrator {
public:
  static void registerAll()
  {
    qRegisterMetaType<Datapoint>("EDII::IPCQtDBus::Datapoint");
    qDBusRegisterMetaType<Datapoint>();
    qRegisterMetaType<Data>("EDII::IPCQtDBus::Data");
    qDBusRegisterMetaType<Data>();
    qRegisterMetaType<DataVec>("EDII::IPCQtDBus::DataVec");
    qDBusRegisterMetaType<DataVec>();
    qRegisterMetaType<DataPack>("EDII::IPCQtDBus::DataPack");
    qDBusRegisterMetaType<DataPack>();
    qRegisterMetaType<LoadOptionsVec>("EDII:IPCQtDBus::LoadOptionsVec");
    qDBusRegisterMetaType<LoadOptionsVec>();
    qRegisterMetaType<SupportedFileFormat>("EDII::IPCQtDBus::SupportedFileFormat");
    qDBusRegisterMetaType<SupportedFileFormat>();
    qRegisterMetaType<SupportedFileFormatVec>("EDII::IPCQtDBus::SupportedFileFormatVec");
    qDBusRegisterMetaType<SupportedFileFormatVec>();
    qRegisterMetaType<ABIVersion>("EDII::IPCQtDBus::ABIVersion");
    qDBusRegisterMetaType<ABIVersion>();
  }
};

} // IPCQtDBus
} // EDII

#endif // ECHMET_EDII_IPC_QTDBUS_H
