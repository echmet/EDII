#include "csvfileloader.h"
#include "malformedcsvfiledialog.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QLocale>
#include <QMessageBox>
#include <QTextStream>
#include <plugins/threadeddialog.h>


class InvalidHeaderError : std::runtime_error {
public:
  InvalidHeaderError(const QString &line) : std::runtime_error("Cannot head table header"),
    line(line)
  {}

  const QString line;
};

class InvalidSeparatorError : std::runtime_error {
public:
  InvalidSeparatorError() : std::runtime_error("String does not contain any periods")
  {}
};

class NonnumericValueError : std::runtime_error {
public:
  NonnumericValueError() : std::runtime_error("Value cannot be converted to number")
  {}
};

namespace plugin {

const QMap<QString, CsvFileLoader::Encoding> CsvFileLoader::SUPPORTED_ENCODINGS = { {"ISO-8859-1", CsvFileLoader::Encoding("ISO-8859-1", QByteArray(), "ISO-8859-1 (Latin 1)") },
                                                                                    {"ISO-8859-2", CsvFileLoader::Encoding("ISO-8859-2", QByteArray(), "ISO-8859-2 (Latin 2)") },
                                                                                    { "windows-1250", CsvFileLoader::Encoding("windows-1250", QByteArray(), "Windows-1250 (cp1250)") },
                                                                                    { "windows-1251", CsvFileLoader::Encoding("windows-1251", QByteArray(), "Windows-1251 (cp1251)") },
                                                                                    { "windows-1252", CsvFileLoader::Encoding("windows-1252", QByteArray(), "Windows-1252 (cp1252)") },
                                                                                    { "UTF-8", CsvFileLoader::Encoding("UTF-8", QByteArray("\xEF\xBB\xBF", 3), "UTF-8") },
                                                                                    { "UTF-16LE", CsvFileLoader::Encoding("UTF-16LE", QByteArray("\xFF\xFE", 2), "UTF-16LE (Little Endian)") },
                                                                                    { "UTF-16BE", CsvFileLoader::Encoding("UTF-16BE", QByteArray("\xFF\xFF", 2), "UTF-16BE (Big Endian)") },
                                                                                    { "UTF-32LE", CsvFileLoader::Encoding("UTF-32LE", QByteArray("\xFF\xFE\x00\x00", 4), "UTF-32LE (Little Endian)") },
                                                                                    { "UTF-32BE", CsvFileLoader::Encoding("UTF-32BE", QByteArray("\x00\x00\xFF\xFF", 4), "UTF-32BE (Big Endian)") } };

CsvFileLoader::Encoding::Encoding() :
  name(""),
  bom(QByteArray()),
  canHaveBom(false),
  displayedName("")
{
}

CsvFileLoader::Encoding::Encoding(const QString &name, const QByteArray &bom, const QString &displayedName) :
  name(name),
  bom(bom),
  canHaveBom(bom.size() > 0 ? true : false),
  displayedName(displayedName)
{
}

CsvFileLoader::Encoding::Encoding(const Encoding &other) :
  name(other.name),
  bom(other.bom),
  canHaveBom(other.canHaveBom),
  displayedName(other.displayedName)
{
}

CsvFileLoader::Encoding & CsvFileLoader::Encoding::operator=(const CsvFileLoader::Encoding &other)
{
  const_cast<QString&>(name) = other.name;
  const_cast<QByteArray&>(bom) = other.bom;
  const_cast<bool&>(canHaveBom) = other.canHaveBom;
  const_cast<QString&>(displayedName) = other.displayedName;

  return *this;
}

CsvFileLoader::Parameters::Parameters() :
  delimiter('\0'),
  decimalSeparator('.'),
  xColumn(0), yColumn(0),
  multipleYcols(false),
  hasHeader(false),
  linesToSkip(0),
  encodingId(QString()),
  readBom(false),
  isValid(false)
{
}

CsvFileLoader::Parameters::Parameters(const QChar &delimiter, const QChar &decimalSeparator,
                                      const int xColumn, const int yColumn,
                                      const bool multipleYcols,
                                      const bool hasHeader, const int linesToSkip,
                                      const QString &encodingId, const bool &readBom) :
  delimiter(delimiter),
  decimalSeparator(decimalSeparator),
  xColumn(xColumn), yColumn(yColumn),
  multipleYcols(multipleYcols),
  hasHeader(hasHeader), linesToSkip(linesToSkip),
  encodingId(encodingId), readBom(readBom),
  isValid(true)
{
}

CsvFileLoader::Parameters & CsvFileLoader::Parameters::operator=(const Parameters &other)
{
  const_cast<QChar&>(delimiter) = other.delimiter;
  const_cast<QChar&>(decimalSeparator) = other.decimalSeparator;
  const_cast<int&>(xColumn) = other.xColumn;
  const_cast<int&>(yColumn) = other.yColumn;
  const_cast<bool&>(multipleYcols) = other.multipleYcols;
  const_cast<bool&>(hasHeader) = other.hasHeader;
  const_cast<int&>(linesToSkip) = other.linesToSkip;
  const_cast<QString&>(encodingId) = other.encodingId;
  const_cast<bool&>(readBom) = other.readBom;
  const_cast<bool&>(isValid) = other.isValid;

  return *this;
}

static
double readValue(const QString &s)
{
  static QLocale cLoc(QLocale::C);
  bool ok;

  const double d = cLoc.toDouble(s, &ok);
  if (!ok)
    throw NonnumericValueError();

  return d;
}

static
void sanitizeDecSep(QString &s, const QChar &sep)
{
  /* Check that the string does not contain period as the default separator */
  if (sep != '.' && s.contains('.'))
    throw InvalidSeparatorError();

  s.replace(sep, '.');
}

class MalformedCsvFileThreadedDialog : public ThreadedDialog<MalformedCsvFileDialog>
{
public:
  MalformedCsvFileThreadedDialog(UIPlugin *plugin, const MalformedCsvFileDialog::Error err, const int lineNo, const QString &fileName, const QString &badLine) :
    ThreadedDialog<MalformedCsvFileDialog>{
      plugin,
      [this]() {
        return new MalformedCsvFileDialog{this->err, this->lineNo, this->fileName, this->badLine};
      }
    },
    err{err},
    lineNo{lineNo},
    fileName{fileName},
    badLine{badLine}
  {}

private:
  const MalformedCsvFileDialog::Error err;
  const int lineNo;
  const QString fileName;
  const QString badLine;
};

void showMalformedFileError(UIPlugin *uiPlugin, const MalformedCsvFileDialog::Error err, const int lineNo, const QString &fileName, const QString &badLine)
{
  MalformedCsvFileThreadedDialog dlgWrap{uiPlugin, err, lineNo, fileName, badLine};
  dlgWrap.execute();
}

CsvFileLoader::DataPack CsvFileLoader::readClipboard(UIPlugin *uiPlugin, const Parameters &params)
{
  QString clipboardText = QApplication::clipboard()->text();
  QTextStream stream;

  stream.setCodec(params.encodingId.toUtf8());
  stream.setString(&clipboardText);

  return readStream(uiPlugin, stream, params.delimiter, params.decimalSeparator, params.xColumn, params.yColumn,
                    params.multipleYcols, params.hasHeader, params.linesToSkip, "<clipboard>");
}

CsvFileLoader::DataPack CsvFileLoader::readFile(UIPlugin *uiPlugin, const QString &path, const Parameters &params)
{
  QFile dataFile(path);
  QTextStream stream;

  if (!dataFile.exists()) {
    ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Invalid file"), QObject::tr("Specified file does not exist"));
    return {};
  }

  if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Cannot open file"), QString(QObject::tr("Cannot open the specified file for reading\n"
                                                                                                                "Error reported: %1")).arg(dataFile.errorString()));
    return {};
  }

  /* Check BOM */
  if (params.readBom) {
    const QByteArray &bom = SUPPORTED_ENCODINGS[params.encodingId].bom;
    const QByteArray actualBom = dataFile.read(bom.size());

    if (actualBom.size() != bom.size()) {
      ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Cannot read file"), QObject::tr("Byte order mark was expected but not found"));
      return {};
    }

    if (memcmp(actualBom.data(), bom.data(), bom.size())) {
      ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Cannot read file"), QObject::tr("Byte order mark does not match to that expected for the given encoding"));
      return {};
    }
  }

  stream.setDevice(&dataFile);
  stream.setCodec(params.encodingId.toUtf8());

  return readStream(uiPlugin, stream, params.delimiter, params.decimalSeparator, params.xColumn, params.yColumn,
                    params.multipleYcols, params.hasHeader, params.linesToSkip, dataFile.fileName());

}

CsvFileLoader::DataPack CsvFileLoader::readStream(UIPlugin *uiPlugin,
                                                  QTextStream &stream, const QChar &delimiter, const QChar &decimalSeparator,
                                                  const int xColumn, const int yColumn,
                                                  const bool multipleYcols,
                                                  const bool hasHeader, const int linesToSkip,
						  const QString &fileName)
{
  PointVecVec ptsVecVec;
  QString xType;
  std::vector<QString> yTypes;
  QStringList lines;
  int highColumn = (yColumn > xColumn) ? yColumn : xColumn;
  int linesRead = 0;
  int emptyLines = 0;

  /* Skip leading blank lines */
  while (!stream.atEnd()) {
    QString line = stream.readLine();

    if (line.trimmed() != QString("")) {
      lines.append(line);
      break;
    }

    emptyLines++;
  }

  /* Read the rest of the file */
  while (!stream.atEnd())
    lines.append(stream.readLine());

  if (lines.size() < 1) {
    ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("No data"), QObject::tr("Input stream contains no data"));
    return {};
  }

  if (lines.size() < linesToSkip + 1) {
    ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Invalid data"), QObject::tr("File contains less lines than the number of lines that were to be skipped"));
    return {};
  }

  linesRead = linesToSkip;

  if (multipleYcols) {
    try {
      auto header = readHeaderMulti(lines, delimiter, hasHeader, linesRead);
      int columns = std::get<0>(header);
      xType = std::get<1>(header);
      yTypes = std::get<2>(header);

      ptsVecVec = readStreamMulti(uiPlugin, std::move(lines), delimiter, decimalSeparator, columns, emptyLines, linesRead, fileName);
    } catch (const InvalidHeaderError &ex) {
      showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::POSSIBLY_INCORRECT_SETTINGS, linesRead, fileName, ex.line);

      return {};
    }
  } else {
    try {
      auto header = readHeaderSingle(lines, delimiter, xColumn, yColumn, hasHeader, highColumn, linesRead);
      xType = std::get<0>(header);
      yTypes = { std::get<1>(header) };
    } catch (const InvalidHeaderError &ex) {
      showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::POSSIBLY_INCORRECT_SETTINGS, linesRead, fileName, ex.line);

      return {};
    }

    ptsVecVec = readStreamSingle(uiPlugin, std::move(lines), delimiter, decimalSeparator,
                                 xColumn, yColumn, highColumn, emptyLines, linesRead, fileName);
  }

  return DataPack(std::move(ptsVecVec), std::move(xType), std::move(yTypes));
}

std::tuple<int, QString, std::vector<QString>> CsvFileLoader::readHeaderMulti(const QStringList &lines, const QChar &delimiter,
                                                                              const bool hasHeader, int &linesRead)
{
  QString line = lines.at(linesRead);
  QStringList header = line.split(delimiter);

  if (header.size() < 2)
    throw InvalidHeaderError(line);

  if (hasHeader) {
    QString xType = header.at(0);
    std::vector<QString> yTypes;

    yTypes.resize(header.size() - 1);
    std::copy(header.begin() + 1, header.end(), yTypes.begin());

    linesRead++;
    return { header.size(), xType, yTypes };
  } else {
    return { header.size(), "", {} };
  }
}

std::tuple<QString, QString> CsvFileLoader::readHeaderSingle(const QStringList &lines, const QChar &delimiter,
                                                             const int xColumn, const int yColumn,
                                                             const bool hasHeader, const int highColumn, int &linesRead)
{
  if (hasHeader) {
    QStringList header;
    const QString &line = lines.at(linesRead);

    header = line.split(delimiter);
    if (header.size() < highColumn)
      throw InvalidHeaderError(line);

    linesRead++;
    return { header.at(xColumn - 1), header.at(yColumn - 1) };
  } else {
    /* Check file format and warn the user early if the expected format does not match to that of the file */
    const QString &line = lines.at(linesRead);
    const QStringList splitted = line.split(delimiter);

    if (splitted.size() < highColumn)
      throw InvalidHeaderError(line);

    return {};
  }
}

CsvFileLoader::PointVecVec CsvFileLoader::readStreamMulti(UIPlugin *uiPlugin,
                                                          QStringList &&lines, const QChar &delimiter, const QChar &decimalSeparator,
                                                          const int columns, const int emptyLines, int linesRead, const QString &fileName)
{
  assert(columns > 1);

  PointVecVec ptsVecVec;

  std::vector<double> yVals;

  ptsVecVec.resize(columns - 1);
  yVals.resize(columns - 1);

  for (int idx = linesRead; idx < lines.size(); idx++) {
    QStringList values;
    double x;
    const QString &line = lines.at(idx);

    values = line.split(delimiter);
    if (values.size() != columns) {
      showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::BAD_DELIMITER, linesRead + emptyLines + 1, fileName, line);
      return ptsVecVec;
    }

    for (auto &v : values) {
      try {
	sanitizeDecSep(v, decimalSeparator);
      } catch (const InvalidSeparatorError &) {
        showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::BAD_DELIMITER, linesRead + emptyLines + 1, fileName, line);

	return ptsVecVec;
      }
    }

    try {
      x = readValue(values.at(0));
      for (int jdx = 1; jdx < columns; jdx++) {
        yVals.at(jdx - 1) = readValue(values.at(jdx));
      }
    } catch (const NonnumericValueError &) {
      showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::BAD_VALUE_DATA, linesRead + emptyLines + 1, fileName, line);

      return ptsVecVec;
    }

    for (int jdx = 0; jdx < columns - 1; jdx++)
      ptsVecVec.at(jdx).emplace_back(std::make_tuple(x, yVals.at(jdx)));

    linesRead++;
  }

  return ptsVecVec;
}

CsvFileLoader::PointVecVec CsvFileLoader::readStreamSingle(UIPlugin *uiPlugin,
                                                           QStringList &&lines, const QChar &delimiter, const QChar &decimalSeparator,
                                                           const int xColumn, const int yColumn, const int highColumn,
                                                           const int emptyLines, int linesRead, const QString &fileName)
{
  PointVec points;

  for (int idx = linesRead; idx < lines.size(); idx++) {
    QStringList values;
    double x, y;
    const QString &line = lines.at(idx);

    values = line.split(delimiter);
    if (values.size() < highColumn) {
      showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::BAD_DELIMITER, linesRead + emptyLines + 1, fileName, line);
      return { points };
    }

    QString &sx = values[xColumn - 1];
    QString &sy = values[yColumn - 1];

    try {
      sanitizeDecSep(sx, decimalSeparator);
      sanitizeDecSep(sy, decimalSeparator);
    } catch (const InvalidSeparatorError &) {
      showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::BAD_DELIMITER, linesRead + emptyLines + 1, fileName, line);
      return { points };
    }

    try {
      x = readValue(sx);
      y = readValue(sy);
    } catch (const NonnumericValueError &) {
      showMalformedFileError(uiPlugin, MalformedCsvFileDialog::Error::BAD_VALUE_DATA, linesRead + emptyLines + 1, fileName, line);
      return { points };
    }

    points.emplace_back(std::make_tuple(x, y));
    linesRead++;
  }

  return { points };
}

} // namespace backend
