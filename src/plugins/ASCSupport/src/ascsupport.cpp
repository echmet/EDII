#include "ascsupport.h"
#include "availablechannels.h"
#include "ui/selectchannelsdialog.h"
#include "ui/commonpropertiesdialog.h"
#include "ui/pickdecimalpointdialog.h"

#include <plugins/pluginhelpers_p.h>
#include <plugins/threadeddialog.h>
#include <QFileDialog>
#include <QString>
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <locale>
#include <iostream>
#include <regex>
#include <QComboBox>
#include <QLayout>


#if defined Q_OS_UNIX
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <cstring>
#elif defined Q_OS_WIN
  #include <Windows.h>
#endif // Q_OS_

#if defined ENCODING_USE_ICU
#include <unicode/unistr.h>
#endif // ENCODING_USE_ICU

#define KV_DELIM ':'

static const std::string HZ_STR{"Hz"};
static const std::string PTS_STR{"Pts."};

namespace plugin {

typedef std::list<std::string>::const_iterator LIt;

class PickDecimalPointThreadedDialog : public ThreadedDialog<PickDecimalPointDialog>
{
public:
  PickDecimalPointThreadedDialog(UIPlugin *plugin, const QString &name) :
    ThreadedDialog<PickDecimalPointDialog>{
      plugin,
      [this]() {
        auto dlg = new PickDecimalPointDialog{this->m_name};

        return dlg;
      }
    },
    m_name{name}
  {}
private:
  const QString m_name;
};

class SelectChannelsThreadedDialog : public ThreadedDialog<SelectChannelsDialog>
{
public:
  SelectChannelsThreadedDialog(UIPlugin *plugin, const std::vector<std::string> &availChans) :
    ThreadedDialog<SelectChannelsDialog>{
      plugin,
      [this]() {
        auto dlg = new SelectChannelsDialog{this->m_availChans};

        return dlg;
      }
    },
    m_availChans{availChans}
  {}

private:
  const std::vector<std::string> m_availChans;
};

class OpenFileThreadedDialog : public ThreadedDialog<QFileDialog>
{
public:
  OpenFileThreadedDialog(UIPlugin *plugin, const QString &path) :
    ThreadedDialog<QFileDialog>{
      plugin,
      [this]() {
        auto dlg = new QFileDialog{nullptr, QObject::tr("Pick an ASC file"), this->m_path};

        dlg->setNameFilter("ASC file (*.asc *.ASC)");
        dlg->setAcceptMode(QFileDialog::AcceptOpen);
        dlg->setFileMode(QFileDialog::ExistingFiles);

        return dlg;
      }
    },
    m_path{path}
  {
  }

private:
  const QString m_path;
};

class CommonPropertiesThreadedDialog : public ThreadedDialog<CommonPropertiesDialog>
{
public:
  CommonPropertiesThreadedDialog(UIPlugin *plugin) :
    ThreadedDialog<CommonPropertiesDialog>{
      plugin,
      []() {
        return new CommonPropertiesDialog{SupportedEncodings::supportedEncodings()};
      }
    }
  {}
};

static
char initDecimalPoint()
{
  static const std::locale sLoc{""};
  const char sep = std::use_facet<std::numpunct<char>>(sLoc).decimal_point();

#ifdef Q_OS_WIN
  setlocale(LC_ALL, sLoc.c_str()); /* No, this is not a joke */
#endif // Q_OS_WIN

  return sep;
}

static
double strToDbl(const std::string &ns)
{
  static const char decPoint = initDecimalPoint();

  std::string s{ns};

  auto replaceAll = [&s](const char current, const char wanted) {
    const std::string _wanted{wanted};

    size_t idx = s.find(current);
    while (idx != s.npos) {
      s = s.replace(idx , 1, _wanted);
      idx = s.find(current);
    }
  };

  switch (decPoint) {
  case '.':
    replaceAll(',', '.');
    break;
  case ',':
    replaceAll('.', ',');
    break;
  }

  double d;
  try {
    size_t invIdx;
    d = std::stod(s, &invIdx);
    if (invIdx != s.length())
      throw ASCFormatException{"String \"" + ns + "\" cannot be converted to decimal number"};
  } catch (std::invalid_argument &) {
    throw ASCFormatException{"String \"" + ns + "\" cannot be converted to decimal number"};
  } catch (std::out_of_range &) {
    throw ASCFormatException{"String \"" + ns + "\" represents a number outside of the available numerical range"};
  }

  return d;
}

static
int strToInt(const std::string &s)
{
  try {
    size_t invIdx;
    int n = std::stoi(s, &invIdx);
    if (invIdx != s.length())
      throw ASCFormatException{"String \"" + s + "\" cannot be converted to integer"};
    return n;
  } catch (std::invalid_argument &) {
    throw ASCFormatException{"String \"" + s + "\" cannot be converted to integer"};
  } catch (std::out_of_range &) {
    throw ASCFormatException{"String \"" + s + "\" represents a number outside of the available numerical range"};
  }
}

static
void reportError(UIPlugin *plugin, const QString &error)
{
  ThreadedDialog<QMessageBox>::displayCritical(plugin, QObject::tr("Cannot read ASC file"), error);
}

static
void reportWarning(UIPlugin *plugin, const QString &warning)
{
  ThreadedDialog<QMessageBox>::displayWarning(plugin, QObject::tr("Cannot read ASC file"), warning);
}

static
std::vector<std::string> fileList(UIPlugin *plugin, const QString &hintPath)
{
  std::vector<std::string> files{};
  OpenFileThreadedDialog dlgWrap{plugin, hintPath};

  if (dlgWrap.execute() != QDialog::Accepted)
    return files;

  const auto _files = dlgWrap.dialog()->selectedFiles();

  for (const QString &file : _files)
    files.emplace_back(QDir::toNativeSeparators(file).toUtf8());

  return files;
}

std::tuple<std::string, std::string> splitKeyValue(const std::string &entry, const std::string &delim)
{
  const size_t delimIdx = entry.find(delim);
  const size_t STEP = delim.length();

  if (delimIdx == entry.npos)
    throw std::runtime_error{"KeyValue entry contains no delimiter"};

  std::string key = entry.substr(0, delimIdx);
  std::transform(key.begin(), key.end(), key.begin(), ::tolower);

  if (entry.length() <= delimIdx + STEP)
    return {key, ""};
  else
    return {key, entry.substr(delimIdx + delim.length())};
}

static
char pickDecimalPoint(UIPlugin *plugin, const std::string &name)
{
  PickDecimalPointThreadedDialog dlgWrap{plugin, QString::fromStdString(name)};
  dlgWrap.execute();

  return dlgWrap.dialog()->separator();
}

static
ASCContext makeContext(UIPlugin *plugin, const std::string &name, const std::string &path, const std::list<std::string> &header)
{
  int nChans = -1;
  std::string kvDelim;

  /* Autodetect the value delimiter first */
  const char valueDelim = [&header] {
    const std::string &firstLine = header.front();
    const size_t delimIdx = firstLine.find(KV_DELIM);

    if (delimIdx == firstLine.npos)
      throw ASCFormatException{"KeyValue entry contains no delimiter"};

    if (firstLine.length() == delimIdx + 1)
      throw ASCFormatException{"First line in the header contains a key with no value"};

    return firstLine.at(delimIdx + 1);
  }();

  kvDelim = std::string{KV_DELIM} + std::string{valueDelim};
  const char dataDecimalPoint = [plugin, &name](const char valueDelim) {
    if (valueDelim == '.' || valueDelim == ',')
      return pickDecimalPoint(plugin, name);

    return '.'; /* The actual value does not really matter */
  }(valueDelim);

  /* Look for the number of channels in the file */
  for (LIt it = header.cbegin(); it != header.cend(); it++) {
    const auto kv = splitKeyValue(*it, kvDelim);
    const std::string &key = std::get<0>(kv);
    const std::string &value = std::get<1>(kv);

    if (key == "maxchannels") {
      try {
        nChans = strToInt(value);
        break;
      } catch (ASCFormatException &) {
        throw ASCFormatException{"Cannot read number of channels"};
      }
    }
  }

  if (nChans < 1)
    throw ASCFormatException{"Number of channels in the file is either not specified or invalid"};

  return ASCContext{name, path, static_cast<size_t>(nChans), std::move(kvDelim), valueDelim, dataDecimalPoint};
}

static
void parseTraces(UIPlugin *plugin, std::vector<Data> &data, const ASCContext &ctx, const std::list<std::string> &traces, const ASCSupport::SelectedChannelsVec &selChans)
{
  auto isSelected = [&](const size_t idx) {
    const auto &yUnit = ctx.yAxisTitles.at(idx);
    const auto &yUnitSel = selChans.at(idx).first;

    return (yUnit == yUnitSel) && (selChans.at(idx).second);
  };
  LIt it = traces.cbegin();

  for (size_t channel = 0; channel < ctx.nChans; channel++) {
    const int numPoints = ctx.nDatapoints.at(channel);
    if (!isSelected(channel)) {
      std::advance(it, numPoints);
      continue;
    }

    const double timeStep = 1.0 / ctx.samplingRates.at(channel) * ctx.xAxisMultipliers.at(channel);
    const double yMultiplier = ctx.yAxisMultipliers.at(channel);
    std::vector<std::tuple<double, double>> dataPoints{};

    dataPoints.reserve(numPoints);

    for (int pt = 0; pt < numPoints; pt++) {
      const std::string &s = *it;
      const double xValue = timeStep * pt;
      const double yValue = strToDbl(s) * yMultiplier;
      dataPoints.emplace_back(xValue, yValue);

      if ((++it == traces.cend()) && (pt != numPoints - 1))
        throw ASCFormatException{"Unexpected end of data trace"};
    }

    data.emplace_back(Data{ctx.name, std::string{"Channel "} + std::to_string(channel),
                           ctx.path,
                           "Time",
                           "Signal",
                           ctx.xAxisTitles.at(channel),
                           ctx.yAxisTitles.at(channel),
                           std::move(dataPoints)
                      });
  }

  if (it != traces.cend())
    reportWarning(plugin, "Data trace is longer than expected");
}

#if defined Q_OS_UNIX

static
std::string convertToUTF8ICU(const char *buf, const int32_t bufLen, const std::string &encoding)
{
  icu::UnicodeString ustr{buf, bufLen, encoding.c_str()};

  std::string converted{};
  ustr.toUTF8String(converted);

  return converted;
}

static
std::istringstream readFile(const std::string &path, const std::string &encoding)
{
  int fd = open(path.c_str(), O_RDONLY);
  if (fd <= 0) {
    const std::string errStr{strerror(errno)};
    throw ASCFormatException{std::string{"Cannot open file: "} + errStr};
  }

  const off_t len = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  if (len + 1 > std::numeric_limits<int32_t>::max()) {
    close(fd);
    throw ASCFormatException{"File is too large process"};
  }

  char *buf = new (std::nothrow) char[len + 1];
  if (buf == nullptr) {
    close(fd);
    throw ASCFormatException{"Cannot open file: out of memory"};
  }

  ssize_t r = read(fd, buf, len);
  if (r < len) {
    delete [] buf;
    close(fd);
    throw ASCFormatException{"Cannot open file: unable to read the whole file"};
  }
  buf[len] = '\0'; /* Force zero-termination */

  close(fd);

  std::string converted = convertToUTF8ICU(buf, len, encoding);
  delete [] buf;

  std::istringstream iss{std::move(converted)};

  return iss;
}
#elif defined Q_OS_WIN
static
std::string convertToUTF8WinAPI(const char *buf, const int encoding)
{
  int size;
  wchar_t *wStr;

  if (encoding == CP_UTF8)
    return std::string{buf}; /* No need to convert anything */

  /* Convert from source encoding to wchar */
  size = MultiByteToWideChar(encoding, 0, buf, -1, nullptr, 0);
  if (size <= 0)
    throw ASCFormatException{"Cannot get wchar_t array size"};

  wStr = new (std::nothrow) wchar_t[size];
  if (wStr == nullptr)
    throw ASCFormatException{"Cannot convert ASC input to UTF-8: Out of memory"};

  size = MultiByteToWideChar(encoding, 0, buf, -1, wStr, size);
  if (size <= 0) {
    delete [] wStr;
    throw ASCFormatException{"Cannot convert ASC input to UTF-8: Conversion failure"};
  }

  /* Convert from wchar to UTF-8 */
  char *convStr;

  size = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, nullptr, 0, nullptr, nullptr);
  if (size <= 0) {
    delete [] wStr;
    throw ASCFormatException{"Cannot convert ASC input to UTF-8: Cannot get char array size"};
  }

  convStr = new (std::nothrow) char[size];
  if (convStr == nullptr) {
    delete [] wStr;
    throw ASCFormatException{"Cannot convert ASC input to UTF-8: Out of memory"};
  }

  size = WideCharToMultiByte(CP_UTF8, 0, wStr, -1, convStr, size, nullptr, nullptr);
  if (size <= 0) {
    delete [] wStr;
    delete [] convStr;
    throw ASCFormatException{"Cannot convert ASC input to UTF-8: Conversion failure"};
  }

  delete [] wStr;

  std::string converted{convStr};
  delete [] convStr;

  return converted;
}

static
std::istringstream readFile(const std::string &path, const SupportedEncodings::EncodingType &encoding)
{
  HANDLE fh;
  int wSize;
  wchar_t *wPath;
  DWORD fileSizeHigh;
  char *buf;
  int64_t fileSize = 0;

  wSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, nullptr, 0);
  if (wSize <= 0)
    throw ASCFormatException{"Cannot get wchar_t array size"};

  wPath = new (std::nothrow) wchar_t[wSize];
  if (wPath == nullptr)
    throw ASCFormatException{"Cannot convert UTF-8 path to Windows native path: Out of memory"};

  wSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, wPath, wSize);
  if (wSize <= 0) {
    delete [] wPath;
    throw ASCFormatException{"Cannot convert UTF-8 path to Windows native path: Conversion failure"};
  }

  fh = CreateFileW(wPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (fh == INVALID_HANDLE_VALUE) {
    delete [] wPath;
    throw ASCFormatException{"Cannot open data file"};
  }

  const DWORD fileSizeLow = GetFileSize(fh, &fileSizeHigh);
  if (fileSizeLow == INVALID_FILE_SIZE) {
    CloseHandle(fh);
    delete [] wPath;
    throw ASCFormatException{"Failed to get file size"};
  }

  fileSize |= static_cast<int64_t>(fileSizeHigh) << 32 | fileSizeLow;

  buf = new (std::nothrow) char[fileSize + 1];
  if (buf == nullptr) {
    CloseHandle(fh);
    delete [] wPath;
    throw ASCFormatException{"Cannot allocate input buffer"};
  }

  DWORD bytesRead;
  if (ReadFile(fh, buf, fileSize, &bytesRead, NULL) != TRUE) {
    CloseHandle(fh);
    delete [] buf;
    delete [] wPath;
    throw ASCFormatException{"Cannot read file content"};
  }
  buf[fileSize] = '\0';

  /* We fail if the file size exceeds 4 GiB */
  if (bytesRead != fileSize) {
    CloseHandle(fh);
    delete [] buf;
    delete [] wPath;
    throw ASCFormatException{"Number of read bytes does not match the size of the file"};
  }

  try {
    std::string converted = convertToUTF8WinAPI(buf, encoding);
    CloseHandle(fh);
    delete [] buf;
    delete [] wPath;

    std::istringstream iss{std::move(converted)};
    return iss;
  } catch (const ASCFormatException &up) {
    CloseHandle(fh);
    delete [] buf;
    delete [] wPath;

    throw up;
  }
}

#else
#error "Unknown platform"
#endif // Q_OS_

static
std::string readLine(std::istringstream &stream)
{
  auto endOfLine = [&stream]() {
    int c = stream.peek();
    char _c;

    if (c < 0) return true;
    if (c == '\r') {
      stream.get(_c);
      c = stream.peek();
      if (c == '\n') {
        stream.get(_c);
        return true;    /* CRLF - Windows */
      }
      return true;      /* CR - Old MacOS */
    } else if (c == '\n') {
      stream.get(_c);
      return true;      /* LF - Unix */
    }
    return false;
  };

  std::string line{};

  while (!endOfLine()) {
    char c;
    stream.get(c);
    line += c;
  }

  return line;
}

static
void selectChannels(UIPlugin *plugin, ASCSupport::SelectedChannelsVec &selChans, const std::vector<std::string> &availChans)
{
  SelectChannelsThreadedDialog dlgWrap{plugin, availChans};
  dlgWrap.execute();

  selChans = dlgWrap.dialog()->selection();
}

static
void spliceHeaderTraces(std::list<std::string> &lines, std::list<std::string> &header, std::list<std::string> &traces)
{
  LIt it = lines.cbegin();
  for (; it != lines.cend(); it++) {
    const auto &l = *it;

    if (l.find(KV_DELIM) == l.npos)
      break;
  }

  header.splice(header.cbegin(), lines, lines.cbegin(), it);

  traces = std::move(lines);
}

static
std::vector<std::string> split(std::string s, const char delim)
{
  std::vector<std::string> pieces;
  size_t delimIdx;

  do {
    delimIdx = s.find(delim);
    pieces.emplace_back(s.substr(0, delimIdx));
    s = s.substr(delimIdx + 1);
  } while (delimIdx != s.npos);

  return pieces;
}

static
std::vector<std::string> splitDecimal(std::string s, const char delim, const char dataDecimalPoint)
{
  static const std::regex sciNotation{"^([-+]?[0-9]+)[eE]{1}[-+]?[0-9]+$"};

  auto extractSegment = [&s](std::string &seg, const char delim) {
    size_t idx = -1;
    int cnt = 0;
    while (cnt < 2) {
      idx = s.find(delim, idx + 1);
      if (idx == s.npos)
        break;
      cnt++;
    }

    seg = s.substr(0, idx);
    s = s.substr(idx + 1);

    return cnt == 2;
  };

  if (delim != dataDecimalPoint)
    return split(s, delim);

  std::vector<std::string> segments{};
  while (true) {
    std::string seg{};
    const bool cont = extractSegment(seg, delim);

    const auto sep = split(seg, delim);
    if (sep.size() > 2)
      throw std::logic_error{"Incorrect string parsing in \"splitDecimal\" detected"};
    if (std::regex_match(sep.at(0), sciNotation)) {
      segments.emplace_back(sep.at(0));
      if (!cont) {
        if (sep.size() == 2)
          segments.emplace_back(sep.at(1));
      } else
        s = sep.at(1) + delim + s;
    } else
      segments.emplace_back(std::move(seg));
    if (!cont)
      break;
  }

  return segments;
}

static
void warnDifferentChannels(UIPlugin *plugin, const std::string &filePath)
{
  ThreadedDialog<QMessageBox>::displayInformation(plugin,
                                                  "Channels do not match",
                                                  QString{"Signal channels in the currently processed file\n%1\ndo not match the channels in previous files. "
                                                  "Please review the channels and update the selection of channels you want to load."}.arg(filePath.c_str())
                                                  );

}

Identifier ASCSupport::s_identifier{"ASC text file (EZChrom format)", "ASC text", "ASC", {}};
ASCSupport *ASCSupport::s_me{nullptr};

/* Format-specific handler implementations */
static
void EZChrom_SamplingRate(std::vector<double> &rates, const char delim, const char decPoint, const std::string &entry)
{
  const std::vector<std::string> parts = splitDecimal(entry, delim, decPoint);

  if (parts.back() != HZ_STR)
    throw ASCFormatException{"Unexpected unit of sampling rate"};

  for (size_t idx = 0; idx < parts.size() - 1; idx++)
    rates.emplace_back(strToDbl(parts.at(idx)));
}

static
void EZChrom_TotalDataPoints(std::vector<int> &rates, const char delim, const std::string &entry)
{
  std::vector<std::string> parts = split(entry, delim);

  if (delim == '.') {
    parts.pop_back();
    parts.back() += ".";
  }

  if (parts.back() != PTS_STR)
    throw ASCFormatException{"Unexpected unit of total data points"};

  for (size_t idx = 0; idx < parts.size() - 1; idx++)
    rates.emplace_back(strToInt(parts.at(idx)));
}

static
void EZChrom_AxisMultiplier(std::vector<double> &multipliers, const char delim, const char decPoint, const std::string &entry)
{
  const std::vector<std::string> parts = splitDecimal(entry, delim, decPoint);

  for (const std::string &s : parts)
    multipliers.emplace_back(strToDbl(s));
}

static
void EZChrom_AxisTitle(std::vector<std::string> &titles, const char delim, const std::string &entry)
{
  titles = split(entry, delim);
}

/* Handlers */
EntryHandlersMap ASCSupport::s_handlers =
{
  {
    EntryHandlerSamplingRate::ID(),
    new EntryHandlerSamplingRate(EZChrom_SamplingRate)
  },
  {
    EntryHandlerTotalDataPoints::ID(),
    new EntryHandlerTotalDataPoints(EZChrom_TotalDataPoints)
  },
  {
    EntryHandlerXAxisMultiplier::ID(),
    new EntryHandlerXAxisMultiplier(EZChrom_AxisMultiplier)
  },
  {
    EntryHandlerYAxisMultiplier::ID(),
    new EntryHandlerYAxisMultiplier(EZChrom_AxisMultiplier)
  },
  {
    EntryHandlerXAxisTitle::ID(),
    new EntryHandlerXAxisTitle(EZChrom_AxisTitle)
  },
  {
    EntryHandlerYAxisTitle::ID(),
    new EntryHandlerYAxisTitle(EZChrom_AxisTitle)
  }
};

EDIIPlugin::~EDIIPlugin()
{
}

ASCSupport::ASCSupport(UIPlugin *plugin) :
  m_uiPlugin{plugin}
{
}

ASCSupport::~ASCSupport()
{
}

void ASCSupport::destroy()
{
  delete s_me;
  s_me = nullptr;
}

const EntryHandler * ASCSupport::getHandler(const std::string &key)
{
  class EntryHandlerNull : public EntryHandlerEssentalityTrait<false> { public: virtual void process(ASCContext &, const std::string&) const override {} };
  typedef EntryHandlersMap::const_iterator EHMIt;

  static EntryHandlerNull *nullHandler = new EntryHandlerNull{};

  EHMIt it = s_handlers.find(key);
  if (it == s_handlers.cend())
    return nullHandler;
  else
    return it->second;
}

Identifier ASCSupport::identifier() const
{
  return s_identifier;
}

ASCSupport * ASCSupport::instance(UIPlugin *plugin)
{
  if (s_me == nullptr)
    s_me = new (std::nothrow) ASCSupport{plugin};

  return s_me;
}

std::vector<Data> ASCSupport::load(const int option)
{
  (void)option;

  return loadInteractive("");
}

std::vector<Data> ASCSupport::loadHint(const std::string &hintPath, const int option)
{
  (void)option;

  return loadInteractive(hintPath);
}

std::vector<Data> ASCSupport::loadPath(const std::string &path, const int option)
{
  (void)option;

  AvailableChannels availChans{};
  SelectedChannelsVec selChans{};

  CommonPropertiesThreadedDialog dlgWrap{m_uiPlugin};
  dlgWrap.execute();
  const SupportedEncodings::EncodingType encoding = dlgWrap.dialog()->encoding();
  if (encoding == SupportedEncodings::INVALID_ENCTYPE)
    throw ASCFormatException{"Invalid encoding selected"};

  return loadInternal(path, availChans, selChans, encoding);
}

std::vector<Data> ASCSupport::loadInteractive(const std::string &hintPath)
{
  std::vector<Data> data{};
  CommonPropertiesThreadedDialog dlgWrap{m_uiPlugin};
  std::vector<std::string> files = fileList(m_uiPlugin, QString::fromStdString(hintPath));

  if (files.size() == 0)
    return data;


  dlgWrap.execute();
  const SupportedEncodings::EncodingType encoding = dlgWrap.dialog()->encoding();
  if (encoding == SupportedEncodings::INVALID_ENCTYPE)
    throw ASCFormatException{"Invalid encoding selected"};

  AvailableChannels availChans{};
  SelectedChannelsVec selChans{};

  for (const std::string &file : files) {
    const auto _data = loadInternal(file, availChans, selChans, encoding);
    std::copy(_data.cbegin(), _data.cend(), std::back_inserter(data));
  }

  return data;
}

std::vector<Data> ASCSupport::loadInternal(const std::string &path, AvailableChannels &availChans, SelectedChannelsVec &selChans,
                                           const SupportedEncodings::EncodingType &encoding)
{
  std::vector<Data> data{};

  std::list<std::string> lines{};
  std::istringstream inStream{};

  try {
    inStream = readFile(path, encoding);
  } catch (const ASCFormatException &ex) {
    reportError(m_uiPlugin, QString{"Cannot read file %1\n%2"}.arg(path.c_str(), ex.what()));
    return data;
  }

  while (inStream.good()) {
    std::string line = readLine(inStream);
    if (line.length() > 0)
      lines.emplace_back(std::move(line));
  }

  if (!inStream.eof()) {
    reportError(m_uiPlugin, QString{"Cannot read file %1\n%2"}.arg("I/O error while reading ASC file"));
    return data;
  }

  std::list<std::string> header{};
  std::list<std::string> traces{};
  spliceHeaderTraces(lines, header, traces);

  if (header.size() < 1) {
    reportError(m_uiPlugin, QString{"Cannot read file %1\n%2"}.arg(path.c_str(), "File contains no header")); /* TODO: Switch to headerless mode */
    return data;
  }

  try {
    std::string name = [&path]() -> std::string {
        return {std::find_if(path.rbegin(), path.rend(),
                             [](char c) { return c == '/' || c == '\\'; }).base(),
                path.end()};
    }();

    ASCContext ctx = makeContext(m_uiPlugin, name, path, header);

    parseHeader(ctx, header);

    if (availChans.state == AvailableChannels::State::NOT_SET) {
      availChans = AvailableChannels{ctx.yAxisTitles};
      selectChannels(m_uiPlugin, selChans, availChans.channels());
    } else {
      if (!availChans.matches(ctx)) {
        warnDifferentChannels(m_uiPlugin, name);
        availChans = AvailableChannels{ctx.yAxisTitles};
        selectChannels(m_uiPlugin, selChans, availChans.channels());
      }
    }

    parseTraces(m_uiPlugin, data, ctx, traces, selChans);
  } catch (ASCFormatException &ex) {
    reportError(m_uiPlugin, QString{"Cannot read file %1\n%2"}.arg(path.c_str(), ex.what()));
    return std::vector<Data>{};
  } catch (std::bad_alloc &) {
    reportError(m_uiPlugin, QString{"Cannot read file %1\n%2"}.arg(path.c_str(), "Insufficient memory to read ASC file"));
    return std::vector<Data>{};
  }

  return data;
}

void ASCSupport::parseHeader(ASCContext &ctx, const std::list<std::string> &header)
{
  for (LIt it = header.cbegin(); it != header.cend(); it++) {
    const auto kv = splitKeyValue(*it, ctx.kvDelim);

    const std::string &key = std::get<0>(kv);
    const std::string &value = std::get<1>(kv);

    const EntryHandler *handler = getHandler(key);
    try {
      handler->process(ctx, value);
    } catch (ASCFormatException &ex) {
      if (handler->essential()) {
        throw;
      } else {
        reportWarning(m_uiPlugin, QString::fromStdString(ex.what()));
      }
    } catch (std::bad_alloc &) {
      throw;
    }
  }

  if (!ctx.validate())
    throw ASCFormatException{"ASC header does not contain all information necessary to interpret the data"};
}

EDIIPlugin * initialize(UIPlugin *plugin)
{
  return ASCSupport::instance(plugin);
}

} //namespace plugin
