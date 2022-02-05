#ifndef CSVFILELOADER_H
#define CSVFILELOADER_H

#include <QPointF>
#include <QMap>
#include <QVector>
#include <cassert>

class QTextStream;
class UIPlugin;

namespace plugin {

class CsvFileLoader
{
public:
  enum class EncodingType {
    SingleByte,
    UTF8,
    UTF16LE,
    UTF16BE,
    UTF32LE,
    UTF32BE
  };

  class Encoding {
  public:
    explicit Encoding();
    Encoding(const QString &name, const QByteArray &bom, const QString &displayedName, const EncodingType type);
    Encoding(const Encoding &other);

    Encoding &operator=(const Encoding &other);

    const QString name;
    const QByteArray bom;
    const bool canHaveBom;
    const QString displayedName;
    const EncodingType type;
  };

  class DataPack {
  public:
    DataPack() :
      xType(""),
      yTypes(std::vector<QString>()),
      valid(false)
    {}

    DataPack(const DataPack &other) :
      dataVec(other.dataVec),
      xType(other.xType),
      yTypes(other.yTypes),
      valid(other.valid)
    {}

    DataPack(DataPack &&other) noexcept :
      dataVec(std::move(other.dataVec)),
      xType(other.xType),
      yTypes(std::move(other.yTypes)),
      valid(other.valid)
    {}

    DataPack(std::vector<std::vector<std::tuple<double, double>>> &&dataVec, QString &&xType, std::vector<QString> &&yTypes) noexcept :
      dataVec(std::move(dataVec)),
      xType(std::move(xType)),
      yTypes(std::move(yTypes)),
      valid(true)
    {
      assert(dataVec.size() == yTypes.size());
    }

    DataPack & operator=(const DataPack &other)
    {
      dataVec = other.dataVec;
      const_cast<QString&>(xType) = other.xType;
      const_cast<std::vector<QString>&>(yTypes) = other.yTypes;
      const_cast<bool&>(valid) = other.valid;

      return *this;
    }

    DataPack & operator=(DataPack &&other) noexcept
    {
      dataVec = std::move(other.dataVec);
      const_cast<QString&>(xType) = std::move(other.xType);
      const_cast<std::vector<QString>&>(yTypes) = std::move(other.yTypes);
      const_cast<bool&>(valid) = other.valid;

      return *this;
    }

    std::vector<std::vector<std::tuple<double, double>>> dataVec;
    const QString xType;
    const std::vector<QString> yTypes;
    const bool valid;
  };

  class Parameters {
  public:
    Parameters();
    Parameters(const QChar &delimiter, const QChar &decimalSeparator,
               const int xColumn, const int yColumn,
               const bool multipleYcols,
               const bool hasHeader, const int linesToSkip,
               const QString &encodingId);
    Parameters(const Parameters &other);
    Parameters & operator=(const Parameters &other);

    const QChar delimiter;
    const QChar decimalSeparator;
    const int xColumn;
    const int yColumn;
    const bool multipleYcols;
    const bool hasHeader;
    const int linesToSkip;
    const QString encodingId;
    const bool isValid;
  };

  CsvFileLoader() = delete;

  static std::pair<QString, QString> previewClipboard(const QString &encodingId, const int maxLines);
  static std::pair<QString, QString> previewFile(const QString &path, const QString &encodingId, const int maxLines);
  static DataPack readClipboard(UIPlugin *uiPlugin, const Parameters &params);
  static DataPack readFile(UIPlugin *uiPlugin, const QString &path, const Parameters &params);

  static const QMap<QString, Encoding> SUPPORTED_ENCODINGS;

private:
  typedef std::vector<std::tuple<double, double>> PointVec;
  typedef std::vector<PointVec> PointVecVec;

  static std::tuple<int, QString, std::vector<QString>> readHeaderMulti(const QStringList &lines, const QChar &delimiter,
                                                                        const bool hasHeader, int &linesRead);

  static std::tuple<QString, QString> readHeaderSingle(const QStringList &lines, const QChar &delimiter,
                                                       const int xColumn, const int yColumn,
                                                       const bool hasHeader, const int highColumn, int &linesRead);

  static DataPack readStream(UIPlugin *uiPlugin,
                             std::istream &stream, const Encoding &encoding,
                             const QChar &delimiter, const QChar &decimalSeparator,
                             const int xColumn, const int yColumn,
                             const bool multipleYcols,
                             const bool hasHeader, const int linesToSkip,
                             const QString &fileName);

  static PointVecVec readStreamMulti(UIPlugin *uiPlugin,
                                     QStringList &&lines, const QChar &delimiter, const QChar &decimalSeparator,
                                     const int columns, const int emptyLines, int linesRead,
                                     const QString &fileName);

  static PointVecVec readStreamSingle(UIPlugin *uiPlugin,
                                      QStringList &&lines, const QChar &delimiter, const QChar &decimalSeparator,
                                      const int xColumn, const int yColumn, const int highColumn,
                                      const int emptyLines, int linesRead,
                                      const QString &fileName);
};

} // namespace plugin

#endif // CSVFILELOADER_H
