// vim: set sw=2 ts=2 sts=2 expandtab :

#include "csvfileloader.h"
#include "malformedcsvfiledialog.h"

#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QFileInfo>
#include <QLocale>
#include <QMessageBox>
#include <QtGlobal>
#include <QStringConverter>
#include <plugins/threadeddialog.h>

#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

#ifdef WIN32
#include <memory>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // WIN32

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
  #define IS_BIG_ENDIAN
#endif

#define MAX_LINE_BYTES 4096

class InvalidCodePointError : public std::runtime_error {
public:
  InvalidCodePointError() : std::runtime_error("Input stream contains invalid code point")
  {}
};

class InvalidHeaderError : public std::runtime_error {
public:
  InvalidHeaderError(const QString &line) : std::runtime_error("Cannot head table header"),
    line(line)
  {}

  const QString line;
};

class InvalidSeparatorError : public std::runtime_error {
public:
  InvalidSeparatorError() : std::runtime_error("String does not contain any periods")
  {}
};

class NonnumericValueError : public std::runtime_error {
public:
  NonnumericValueError() : std::runtime_error("Value cannot be converted to number")
  {}
};

#ifdef WIN32
static
std::unique_ptr<char[]> toNativeCodepage(const char *utf8_str)
{
  auto wcLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_str, -1, nullptr, 0);
  if (wcLen == 0)
    throw std::runtime_error{"Cannot convert file name to UTF-16 string"};

  auto wstr = std::unique_ptr<wchar_t[]>(new wchar_t[wcLen]);
  if (wstr == nullptr)
    throw std::runtime_error{"Out of memory"};

  wcLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_str, -1, wstr.get(), wcLen);
  if (wcLen == 0)
    throw std::runtime_error{"Cannot convert file name to UTF-16 string"};

  BOOL dummy;
  auto mbLen = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.get(), -1, nullptr, 0, NULL, &dummy);
  if (mbLen == 0)
    throw std::runtime_error{"Cannot convert file name to native codepage"};

  auto natstr = std::unique_ptr<char[]>(new char[mbLen]);
  mbLen = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.get(), -1, natstr.get(), mbLen, NULL, &dummy);
  if (mbLen == 0)
    throw std::runtime_error{"Cannot convert file name to native codepage"};

  return natstr;
}
#endif // WIN32

namespace plugin {

template <bool Flipped, typename T> struct LF{};
template <bool Flipped, typename T> struct CR{};
template <bool Flipped> struct Surrogate{};

template <> struct LF<false, char> { static const uint32_t value = 0x0A; };
template <> struct CR<false, char> { static const uint32_t value = 0x0D; };
template <> struct LF<false, uint16_t> { static const uint16_t value = 0x000A; };
template <> struct CR<false, uint16_t> { static const uint16_t value = 0x000D; };
template <> struct LF<false, uint32_t> { static const uint32_t value = 0x0000000A; };
template <> struct CR<false, uint32_t> { static const uint32_t value = 0x0000000D; };
template <> struct Surrogate<false> { static const uint16_t value = 0xD800; };

template <> struct LF<true, uint16_t> { static const uint16_t value = 0x0A00; };
template <> struct CR<true, uint16_t> { static const uint16_t value = 0x0D00; };
template <> struct LF<true, uint32_t> { static const uint32_t value = 0x0A000000; };
template <> struct CR<true, uint32_t> { static const uint32_t value = 0x0D000000; };
template <> struct Surrogate<true> { static const uint16_t value = 0x00D8; };

static
QByteArray extractLineSingle(std::istream &stm)
{
  QByteArray ba;

  while (stm.peek() != EOF && ba.size() < MAX_LINE_BYTES) {
    const auto b = stm.get();
    if (b != LF<false, char>::value && b != CR<false, char>::value) {
      ba.append(char(b));
      continue;
    }

    const auto next = stm.peek();
    if (next == b)
      break;
    if (next == LF<false, char>::value || next == CR<false, char>::value)
      stm.get();
    break;
  }

  return ba;
}

template <size_t N>
static
void readMultibyte(QByteArray &ba, std::istream &stm) {
  char nbs[N];
  stm.read(nbs, N);
  if (stm.eof())
    throw InvalidCodePointError();
  ba.append(nbs, N);
}

static
QByteArray extractLineUtf8(std::istream &stm)
{
  QByteArray ba;

  while (stm.peek() != EOF && ba.size() < MAX_LINE_BYTES) {
    const auto ch = stm.get();
    if (ch == 0xC0) {
      ba.append(char(ch));
      readMultibyte<1>(ba, stm);
    } else if (ch == 0xE0) {
      ba.append(char(ch));
      readMultibyte<2>(ba, stm);
    } else if (ch == 0xF0) {
      ba.append(char(ch));
      readMultibyte<3>(ba, stm);
    } else {
      if (ch != CR<false, char>::value && ch != LF<false, char>::value) {
        ba.append(char(ch));
        continue;
      }

      const auto next = stm.peek();
      if (next == ch)
        break;
      if (next == LF<false, char>::value || next == CR<false, char>::value)
        stm.get();

      break;
    }
  }

  return ba;
}

union Utf16Char {
  char bytes[2];
  uint16_t code;
};

template <bool Flip>
static
QByteArray extractLineUtf16(std::istream &stm)
{
  QByteArray ba;

  Utf16Char ch;
  while (stm.peek() != EOF && ba.size() < MAX_LINE_BYTES) {
    stm.read(ch.bytes, 2);
    if (stm.eof())
      throw InvalidCodePointError();

    if (ch.code & Surrogate<Flip>::value) {
      ba.append(ch.bytes);

      stm.read(ch.bytes, 2);
      if (stm.eof())
        throw InvalidCodePointError();
      ba.append(ch.bytes, 2);
    } else {
      if (ch.code != LF<Flip, uint16_t>::value && ch.code != CR<Flip, uint16_t>::value) {
        ba.append(ch.bytes, 2);
        continue;
      }

      if (!stm || stm.peek() == EOF)
        break;

      Utf16Char chTwo;
      stm.read(chTwo.bytes, 2);
      if (stm.eof())
        throw InvalidCodePointError();

      if (chTwo.code == ch.code) {
        stm.seekg(-2, stm.cur);
        break;
      }

      if (chTwo.code != LF<Flip, uint16_t>::value && chTwo.code != CR<Flip, uint16_t>::value)
        stm.seekg(-2, stm.cur);

      break;
    }
  }

  return ba;
}

union Utf32Char {
  char bytes[4];
  uint32_t code;
};

template <bool Flip>
static
QByteArray extractLineUtf32(std::istream &stm)
{
  QByteArray ba;

  Utf32Char ch;
  while (stm.peek() != EOF && ba.size() < MAX_LINE_BYTES) {
    stm.read(ch.bytes, 4);
    if (stm.eof())
      throw InvalidCodePointError();
    if (ch.code != LF<Flip, uint32_t>::value && ch.code != CR<Flip, uint32_t>::value) {
      ba.append(ch.bytes, 4);
      continue;
    }

    if (!stm || stm.peek() == EOF)
      break;

    Utf32Char chTwo;
    stm.read(chTwo.bytes, 4);
    if (stm.eof())
      throw InvalidCodePointError();

    if (chTwo.code == ch.code) {
      stm.seekg(-4, stm.cur);
      break;
    }

    if (chTwo.code != LF<Flip, uint32_t>::value && chTwo.code != CR<Flip, uint32_t>::value)
      stm.seekg(-4, stm.cur);

    break;
  }

  return ba;
}

static
QStringList streamToLines(std::istream &stream, const CsvFileLoader::Encoding &encoding, const int maxLines = -1)
{
  QStringList lines;

  QByteArray (*extractLine)(std::istream &) = nullptr;

  switch (encoding.type) {
  case CsvFileLoader::EncodingType::SingleByte:
    extractLine = extractLineSingle;
    break;
  case CsvFileLoader::EncodingType::UTF8:
    extractLine = extractLineUtf8;
    break;
  case CsvFileLoader::EncodingType::UTF16BE:
#ifdef IS_BIG_ENDIAN
    extractLine = extractLineUtf16<false>;
#else
    extractLine = extractLineUtf16<true>;
#endif
    break;
  case CsvFileLoader::EncodingType::UTF16LE:
#ifdef IS_BIG_ENDIAN
    extractLine = extractLineUtf16<true>;
#else
    extractLine = extractLineUtf16<false>;
#endif
    break;
  case CsvFileLoader::EncodingType::UTF32BE:
#ifdef IS_BIG_ENDIAN
    extractLine = extractLineUtf32<false>;
#else
    extractLine = extractLineUtf32<true>;
#endif
    break;
  case CsvFileLoader::EncodingType::UTF32LE:
#ifdef IS_BIG_ENDIAN
    extractLine = extractLineUtf32<true>;
#else
    extractLine = extractLineUtf32<false>;
#endif
    break;
  }

  assert(extractLine);

  QStringDecoder decoder{encoding.name};
  if (!decoder.isValid())
    throw new std::runtime_error{"Text decoder is not in a valid state. Requested character encoding might not be supported by your Qt libraries"};

  while (true) {
    auto raw = extractLine(stream);
    if (raw.size() > MAX_LINE_BYTES)
      throw std::runtime_error{"Line is too long"};
    if (stream.peek() != EOF || raw.size() > 0)
      lines.append(decoder.decode(raw));
    else
      break;

    if (maxLines > 0 && lines.length() == maxLines)
      break;
  }

  return lines;
}

const QMap<QString, CsvFileLoader::Encoding> CsvFileLoader::SUPPORTED_ENCODINGS = { { "ISO-8859-1", CsvFileLoader::Encoding("ISO-8859-1", QByteArray(), "ISO-8859-1 (Latin 1)", CsvFileLoader::EncodingType::SingleByte) },
                                                                                    { "ISO-8859-2", CsvFileLoader::Encoding("ISO-8859-2", QByteArray(), "ISO-8859-2 (Latin 2)", CsvFileLoader::EncodingType::SingleByte) },
                                                                                    { "windows-1250", CsvFileLoader::Encoding("windows-1250", QByteArray(), "Windows-1250 (cp1250)", CsvFileLoader::EncodingType::SingleByte) },
                                                                                    { "windows-1251", CsvFileLoader::Encoding("windows-1251", QByteArray(), "Windows-1251 (cp1251)", CsvFileLoader::EncodingType::SingleByte) },
                                                                                    { "windows-1252", CsvFileLoader::Encoding("windows-1252", QByteArray(), "Windows-1252 (cp1252)", CsvFileLoader::EncodingType::SingleByte) },
                                                                                    { "UTF-8", CsvFileLoader::Encoding("UTF-8", QByteArray("\xEF\xBB\xBF", 3), "UTF-8", CsvFileLoader::EncodingType::UTF8) },
                                                                                    { "UTF-16LE", CsvFileLoader::Encoding("UTF-16LE", QByteArray("\xFF\xFE", 2), "UTF-16LE (Little Endian)", CsvFileLoader::EncodingType::UTF16LE) },
                                                                                    { "UTF-16BE", CsvFileLoader::Encoding("UTF-16BE", QByteArray("\xFF\xFF", 2), "UTF-16BE (Big Endian)", CsvFileLoader::EncodingType::UTF16BE) },
                                                                                    { "UTF-32LE", CsvFileLoader::Encoding("UTF-32LE", QByteArray("\xFF\xFE\x00\x00", 4), "UTF-32LE (Little Endian)", CsvFileLoader::EncodingType::UTF32LE) },
                                                                                    { "UTF-32BE", CsvFileLoader::Encoding("UTF-32BE", QByteArray("\x00\x00\xFF\xFF", 4), "UTF-32BE (Big Endian)", CsvFileLoader::EncodingType::UTF32BE) } };

CsvFileLoader::Encoding::Encoding() :
  name(""),
  bom(QByteArray()),
  canHaveBom(false),
  displayedName(""),
  type(EncodingType::SingleByte)
{
}

CsvFileLoader::Encoding::Encoding(const QString &name, const QByteArray &bom, const QString &displayedName, const EncodingType type) :
  name(name),
  bom(bom),
  canHaveBom(bom.size() > 0 ? true : false),
  displayedName(displayedName),
  type(type)
{
}

CsvFileLoader::Encoding::Encoding(const Encoding &other) :
  name(other.name),
  bom(other.bom),
  canHaveBom(other.canHaveBom),
  displayedName(other.displayedName),
  type(other.type)
{
}

CsvFileLoader::Encoding & CsvFileLoader::Encoding::operator=(const CsvFileLoader::Encoding &other)
{
  const_cast<QString&>(name) = other.name;
  const_cast<QByteArray&>(bom) = other.bom;
  const_cast<bool&>(canHaveBom) = other.canHaveBom;
  const_cast<QString&>(displayedName) = other.displayedName;
  const_cast<EncodingType&>(type) = other.type;

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
  isValid(false)
{
}

CsvFileLoader::Parameters::Parameters(const QChar &delimiter, const QChar &decimalSeparator,
                                      const int xColumn, const int yColumn,
                                      const bool multipleYcols,
                                      const bool hasHeader, const int linesToSkip,
                                      const QString &encodingId) :
  delimiter(delimiter),
  decimalSeparator(decimalSeparator),
  xColumn(xColumn), yColumn(yColumn),
  multipleYcols(multipleYcols),
  hasHeader(hasHeader), linesToSkip(linesToSkip),
  encodingId(encodingId),
  isValid(true)
{
}

CsvFileLoader::Parameters::Parameters(const Parameters &other) :
  delimiter(other.delimiter),
  decimalSeparator(other.decimalSeparator),
  xColumn(other.xColumn), yColumn(other.yColumn),
  multipleYcols(other.multipleYcols),
  hasHeader(other.hasHeader), linesToSkip(other.linesToSkip),
  encodingId(other.encodingId),
  isValid(other.isValid)
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

static
void skipBom(std::ifstream &stream, const CsvFileLoader::Encoding &encoding)
{
  /* Check and skip BOM */
  const auto &bom = encoding.bom;

  if (!bom.isEmpty()) {
    const size_t sz = bom.size();
    const auto buf = std::unique_ptr<char[]>(new char[sz]);
    std::memset(buf.get(), 0, sz);

    size_t ctr = 0;
    for (; ctr < sz; ctr++) {
      const auto b = stream.get();
      if (b == EOF)
        break;
      buf[ctr] = b;
    }

    if (std::memcmp(buf.get(), bom.data(), bom.size())) {
      stream.clear();
      stream.seekg(0, stream.beg);
    }
  }
}

static
std::ifstream tryOpenStream(const QString &path)
{
#if WIN32
  auto natPath = toNativeCodepage(path.toUtf8().data());
  return std::ifstream{natPath.get(), std::ios::binary};
#else
  return std::ifstream{path.toUtf8().data(), std::ios::binary};
#endif // WIN32
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
  std::istringstream stream(clipboardText.toStdString());

  assert(SUPPORTED_ENCODINGS.contains(params.encodingId));
  const auto &encoding = SUPPORTED_ENCODINGS[params.encodingId];

  return readStream(uiPlugin, stream, encoding, params.delimiter, params.decimalSeparator, params.xColumn, params.yColumn,
                    params.multipleYcols, params.hasHeader, params.linesToSkip, "<clipboard>");
}

std::pair<QString, QString> CsvFileLoader::previewClipboard(const QString &encodingId, int maxLines)
{
  QString clipboardText = QApplication::clipboard()->text();
  std::istringstream stream(clipboardText.toStdString());

  assert(SUPPORTED_ENCODINGS.contains(encodingId));
  const auto &encoding = SUPPORTED_ENCODINGS[encodingId];

  try {
    auto lines = streamToLines(stream, encoding, maxLines);
    QString preview{};
    for (const auto &l: lines)
      preview += l + "\n";

    return {preview, ""};
  } catch (const std::runtime_error &ex) {
    return {
      "",
      QString{
        "Clipboard content does not look like text. Make sure that you are trying to load the correct file "
        "and try to set a different encoding.\n%1"
      }.arg(ex.what())
    };
  }

}

std::pair<QString, QString> CsvFileLoader::previewFile(const QString &path, const QString &encodingId, int maxLines)
{
  if (maxLines < 1)
    return {"", "Number of maximum lines to read must be positive"};

  std::ifstream stream{};
  try  {
    stream = tryOpenStream(path);
  } catch (const std::runtime_error &ex) {
    return {"", "Cannot open file for reading"};
  }

  if (!stream.is_open())
    return {"", "Cannot open file for reading"};

  assert(SUPPORTED_ENCODINGS.contains(encodingId));
  const auto &encoding = SUPPORTED_ENCODINGS[encodingId];

  skipBom(stream, encoding);

  try {
    auto lines = streamToLines(stream, encoding, maxLines);

    QString preview{};
    for (const auto &l : lines)
      preview += l + "\n";

    return {preview, ""};
  } catch (const std::runtime_error &ex) {
    return {
      "",
      QString{
        "File does not look like text. Make sure that you are trying to load the correct file "
        "and try to set a different encoding.\n%1"
      }.arg(ex.what())
    };
  }
}

CsvFileLoader::DataPack CsvFileLoader::readFile(UIPlugin *uiPlugin, const QString &path, const Parameters &params)
{
  std::ifstream stream{};
  try  {
    stream = tryOpenStream(path);
  } catch (const std::runtime_error &ex) {
    ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Cannot open file"), ex.what());
    return{};
  }

  if (!stream.is_open()) {
    ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Cannot open file"), QString(QObject::tr("Cannot open the specified file for reading")));
    return {};
  }

  assert(SUPPORTED_ENCODINGS.contains(params.encodingId));
  const auto &encoding = SUPPORTED_ENCODINGS[params.encodingId];

  skipBom(stream, encoding);

  return readStream(uiPlugin, stream, encoding, params.delimiter, params.decimalSeparator, params.xColumn, params.yColumn,
                    params.multipleYcols, params.hasHeader, params.linesToSkip, QFileInfo(path).fileName());

}

CsvFileLoader::DataPack CsvFileLoader::readStream(UIPlugin *uiPlugin,
                                                  std::istream &stream, const Encoding &encoding,
                                                  const QChar &delimiter, const QChar &decimalSeparator,
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

  try {
    lines = streamToLines(stream, encoding);
  } catch (const std::runtime_error &ex) {
    ThreadedDialog<QMessageBox>::displayWarning(uiPlugin, QObject::tr("Cannot read input"), ex.what());
    return {};
  }

  /* Remove leading blank lines */
  while (!lines.empty()) {
    if (lines.constFirst().trimmed().isEmpty()) {
      lines.pop_front();
      emptyLines++;
    } else
      break;
  }

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
