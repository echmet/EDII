#include "csvsupport.h"
#include "csvfileloader.h"
#include "loadcsvfiledialog.h"

#include <QFileDialog>
#include <plugins/pluginhelpers_p.h>
#include <plugins/threadeddialog.h>

namespace plugin {

class OpenFileThreadedDialog : public ThreadedDialog<QFileDialog>
{
public:
  OpenFileThreadedDialog(UIPlugin *plugin, const QString &path) :
    ThreadedDialog<QFileDialog>{
      plugin,
      [this]() {
        auto dlg = new QFileDialog{nullptr, QObject::tr("Pick a text file"), this->m_path};

        dlg->setAcceptMode(QFileDialog::AcceptOpen);
        dlg->setFileMode(QFileDialog::ExistingFiles);
        PluginHelpers::showWindowOnTop(dlg);

        return dlg;
      }
    },
    m_path{path}
  {
  }

private:
  const QString m_path;
};

class LoadCsvFileThreadedDialog : public ThreadedDialog<LoadCsvFileDialog>
{
public:
  LoadCsvFileThreadedDialog(UIPlugin *plugin) :
    ThreadedDialog<LoadCsvFileDialog>{
      plugin,
      []() {
        auto dlg = new LoadCsvFileDialog{};

        for (const CsvFileLoader::Encoding &enc : CsvFileLoader::SUPPORTED_ENCODINGS)
          dlg->addEncoding(enc.name, enc.displayedName, enc.canHaveBom);

        PluginHelpers::showWindowOnTop(dlg);
        return dlg;
      }
    }
  {}
};

CSVSupport *CSVSupport::s_me{nullptr};
const Identifier CSVSupport::s_identifier{"Comma separated file format support", "CSV", "CSV", {"file", "clipboard"}};

Data loadCsvData(const CsvFileLoader::Data &csvData, const QString &file, const LoadCsvFileDialog::Parameters &p)
{
  QString xType;
  QString yType;
  QString xUnit;
  QString yUnit;
  QString fileName;
  QString dataID = QString::number(p.yColumn);

  switch (p.header) {
  case LoadCsvFileDialog::HeaderHandling::NO_HEADER:
    xType = p.xType;
    yType = p.yType;
    xUnit = p.xUnit;
    yUnit = p.yUnit;
    break;
  case LoadCsvFileDialog::HeaderHandling::HEADER_WITH_UNITS:
    xType = csvData.xType;
    yType = csvData.yType;
    break;
  case LoadCsvFileDialog::HeaderHandling::HEADER_WITHOUT_UNITS:
    xType = csvData.xType;
    yType = csvData.yType;
    xUnit = p.xUnit;
    yUnit = p.yUnit;
    break;
  }

  if (yUnit.length() > 0) {
    if (yUnit.at(yUnit.length() - 1) == '\n')
      yUnit.chop(1);
  }

  if (file.length() > 0)
    fileName = QFileInfo(file).fileName();

  return Data{fileName.toStdString(),
              dataID.toStdString(),
              file.toStdString(),
              xType.toStdString(),
              yType.toStdString(),
              xUnit.toStdString(),
              yUnit.toStdString(),
              std::move(csvData.data)};
}

static
LoadCsvFileThreadedDialog * makeCsvLoaderDialog(UIPlugin *plugin)
{
  return new LoadCsvFileThreadedDialog{plugin};
}

static
CsvFileLoader::Parameters makeCsvLoaderParameters(UIPlugin *plugin, LoadCsvFileThreadedDialog *dlg)
{
  while (true) {
    int dlgRet = dlg->execute();
    if (dlgRet != QDialog::Accepted)
      return CsvFileLoader::Parameters();

    auto _dlg = dlg->dialog();
    const LoadCsvFileDialog::Parameters p = _dlg->parameters();

    if (p.delimiter.length() != 1 && (p.delimiter.compare("\\t") != 0)) {
      ThreadedDialog<QMessageBox>::displayWarning(plugin, QObject::tr("Invalid input"), QObject::tr("Delimiter must be a single character or '\\t' to represent TAB."));
      continue;
    }
    if (p.decimalSeparator == p.delimiter) {
      ThreadedDialog<QMessageBox>::displayWarning(plugin, QObject::tr("Invalid input"), QObject::tr("Delimiter and decimal separator cannot be the same character."));
      continue;
    }

    if (p.xColumn == p.yColumn) {
      ThreadedDialog<QMessageBox>::displayWarning(plugin, QObject::tr("Invalid input"), QObject::tr("X and Y columns cannot be the same"));
      continue;
    }

    QChar delimiter;
    if (p.delimiter.compare("\\t") == 0)
      delimiter = '\t';
    else
      delimiter = p.delimiter.at(0);

    return CsvFileLoader::Parameters(delimiter, p.decimalSeparator,
                                     p.xColumn, p.yColumn,
                                     p.header != LoadCsvFileDialog::HeaderHandling::NO_HEADER,
                                     p.linesToSkip,
                                     p.encodingId, p.readBom);
  }
}

EDIIPlugin::~EDIIPlugin()
{
}

CSVSupport::CSVSupport(UIPlugin *plugin) :
  m_uiPlugin{plugin}
{
}

CSVSupport::~CSVSupport()
{
}

void CSVSupport::destroy()
{
  delete s_me;
}

Identifier CSVSupport::identifier() const
{
  return s_identifier;
}

CSVSupport *CSVSupport::instance(UIPlugin *plugin)
{
  if (s_me == nullptr) {
    try {
      s_me = new CSVSupport{plugin};
    } catch (...) {
      s_me = nullptr;
    }
  }

  return s_me;
}

std::vector<Data> CSVSupport::load(const int option)
{
  switch (option) {
  case 0:
    return loadCsvFromFile("");
  case 1:
    return loadCsvFromClipboard();
  default:
    return std::vector<Data>();
  }
}

std::vector<Data> CSVSupport::loadCsvFromClipboard()
{
  std::vector<Data> retData{};
  auto dlg = makeCsvLoaderDialog(m_uiPlugin);

  while (true) {
    CsvFileLoader::Parameters readerParams = makeCsvLoaderParameters(m_uiPlugin, dlg);
    if (!readerParams.isValid)
      break;

    CsvFileLoader::Data csvData = CsvFileLoader::readClipboard(m_uiPlugin, readerParams);
    if (!csvData.isValid()) {
      readerParams = CsvFileLoader::Parameters();
      continue;
    }

    retData.emplace_back(loadCsvData(csvData, QString(), dlg->dialog()->parameters()));
    break;
  }

  delete dlg;
  return retData;
}

std::vector<Data> CSVSupport::loadCsvFromFile(const std::string &sourcePath)
{
  QStringList files;
  OpenFileThreadedDialog dlgWrap{m_uiPlugin,  QString::fromStdString(sourcePath)};

  if (dlgWrap.execute() != QDialog::Accepted)
   return std::vector<Data>{};

  files = dlgWrap.dialog()->selectedFiles();
  if (files.length() < 1)
    return std::vector<Data>{};

  return loadCsvFromFileInternal(files);
}

std::vector<Data> CSVSupport::loadCsvFromFileInternal(const QStringList &files)
{
  std::vector<Data> retData;
  CsvFileLoader::Parameters readerParams;
  auto dlg = makeCsvLoaderDialog(m_uiPlugin);

  for (const QString &f : files) {
    while (true) {
      if (!readerParams.isValid) {
        readerParams = makeCsvLoaderParameters(m_uiPlugin, dlg);
        if (!readerParams.isValid)
          break;
      }

      CsvFileLoader::Data csvData = CsvFileLoader::readFile(m_uiPlugin, f, readerParams);
      if (!csvData.isValid()) {
        readerParams = CsvFileLoader::Parameters();
        continue;
      }

      retData.emplace_back(loadCsvData(csvData, f, dlg->dialog()->parameters()));
      break;
    }
  }

  delete dlg;
  return retData;
}

std::vector<Data> CSVSupport::loadHint(const std::string &hintPath, const int option)
{
  switch (option) {
  case 0:
    return loadCsvFromFile(hintPath);
  case 1:
    return loadCsvFromClipboard();
  default:
    return std::vector<Data>{};
  }
}

std::vector<Data> CSVSupport::loadPath(const std::string &path, const int option)
{
  switch (option) {
  case 0:
    return loadCsvFromFileInternal(QStringList{QString::fromStdString(path)});
  case 1:
    return loadCsvFromClipboard();
  default:
    return std::vector<Data>{};
  }
}

EDIIPlugin * initialize(UIPlugin *plugin)
{
  return CSVSupport::instance(plugin);
}

} // namespace backend
