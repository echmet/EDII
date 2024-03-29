// vim: set sw=2 ts=2 sts=2 expandtab :

#include "csvsupport.h"
#include "csvfileloader.h"
#include "loadcsvfiledialog.h"
#include "csvutil.h"

#include <QFileDialog>
#include <plugins/pluginhelpers_p.h>
#include <plugins/threadeddialog.h>
#include <stdexcept>

namespace plugin {

class OpenFileThreadedDialog : public ThreadedDialog<QFileDialog>
{
public:
  OpenFileThreadedDialog(UIPlugin *plugin, const QString &path) :
    ThreadedDialog<QFileDialog>{
      plugin,
      [this]() {
        auto dlg = new QFileDialog{nullptr, QObject::tr("Pick a text file"), this->m_path};

        dlg->setNameFilters({"CSV file (*.csv *.CSV *.txt *.TXT)", "Any file (*.*)"});
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

class LoadCsvFileThreadedDialog : public ThreadedDialog<LoadCsvFileDialog>
{
public:
  LoadCsvFileThreadedDialog(UIPlugin *plugin, const QString &source) :
    ThreadedDialog<LoadCsvFileDialog>{
      plugin,
      [this]() {
        auto dlg = new LoadCsvFileDialog{this->m_source};

        for (const CsvFileLoader::Encoding &enc : CsvFileLoader::SUPPORTED_ENCODINGS)
          dlg->addEncoding(enc.name, enc.displayedName, enc.canHaveBom);

        return dlg;
      }
    },
    m_source{source}
  {
  }

private:
  const QString m_source;
};

CSVSupport *CSVSupport::s_me{nullptr};
const Identifier CSVSupport::s_identifier{"Comma separated file format support", "CSV", "CSV", {"file", "clipboard"}};

static
void appendCsvData(std::vector<Data> &data, CsvFileLoader::DataPack &csvData, const QString &file, const LoadCsvFileDialog::Parameters &p)
{
  const size_t N = csvData.dataVec.size();
  const std::string path = file.toStdString();
  std::string xType;
  std::vector<std::string> yTypes;
  std::string xUnit;
  std::string yUnit;
  std::string fileName;

  switch (p.header) {
  case LoadCsvFileDialog::HeaderHandling::NO_HEADER:
    xType = p.xType.toStdString();
    yTypes.resize(N);
    std::fill(yTypes.begin(), yTypes.end(), p.yType.toStdString());
    xUnit = p.xUnit.toStdString();
    yUnit = p.yUnit.toStdString();
    break;
  case LoadCsvFileDialog::HeaderHandling::HEADER_WITH_UNITS:
    xType = csvData.xType.toStdString();
    yTypes = [&]() {
      std::vector<std::string> out;
      for (auto &v : csvData.yTypes)
        out.emplace_back(v.toStdString());
      return out;
    }();
    break;
  case LoadCsvFileDialog::HeaderHandling::HEADER_WITHOUT_UNITS:
    xType = csvData.xType.toStdString();
    yTypes = [&]() {
      std::vector<std::string> out;
      for (auto &v : csvData.yTypes)
        out.emplace_back(v.toStdString());
      return out;
    }();
    xUnit = p.xUnit.toStdString();
    yUnit = p.yUnit.toStdString();
    break;
  }

  if (yUnit.length() > 0) {
    if (yUnit.at(yUnit.length() - 1) == '\n')
      yUnit.pop_back();
  }

  if (file.length() > 0)
    fileName = QFileInfo(file).fileName().toStdString();

  for (size_t idx = 0; idx < N; idx++) {
    Data d{fileName,
           [&]() -> std::string {
             if (N == 1)
               return std::to_string(p.yColumn);
             else
               return std::to_string(idx + 2);
           }(),
           path,
           xType,
           yTypes.at(idx),
           xUnit,
           yUnit,
           std::move(csvData.dataVec[idx])
          };

    data.emplace_back(std::move(d));
  }
}

static
CsvFileLoader::Parameters makeCsvLoaderParameters(const QString &path, UIPlugin *plugin, LoadCsvFileThreadedDialog *dlg)
{
  dlg->create();
  dlg->dialog()->setSource(path);

  while (true) {
    int dlgRet = dlg->execute();
    if (dlgRet != QDialog::Accepted)
      return CsvFileLoader::Parameters();

    try {
      return dialogParamsToLoaderParams(dlg->dialog()->parameters());
    } catch (const std::runtime_error &ex) {
      ThreadedDialog<QMessageBox>::displayWarning(plugin, QObject::tr("Invalid input"), ex.what());
      continue;
    }
  }
}

EDIIPlugin::~EDIIPlugin()
{
}

CSVSupport::CSVSupport(UIPlugin *plugin) :
  m_uiPlugin{plugin}
{
  m_paramsDlg = new LoadCsvFileThreadedDialog{m_uiPlugin, ""};
}

CSVSupport::~CSVSupport()
{
  delete m_paramsDlg;
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

  while (true) {
    CsvFileLoader::Parameters readerParams = makeCsvLoaderParameters("", m_uiPlugin, m_paramsDlg);
    if (!readerParams.isValid)
      break;

    auto csvData = CsvFileLoader::readClipboard(m_uiPlugin, readerParams);
    if (!csvData.valid) {
      readerParams = CsvFileLoader::Parameters();
      continue;
    }

    appendCsvData(retData, csvData, QString(), m_paramsDlg->dialog()->parameters());
    break;
  }

  return retData;
}

std::vector<Data> CSVSupport::loadCsvFromFile(const std::string &sourcePath)
{
  QStringList files;
  OpenFileThreadedDialog dlgWrap{m_uiPlugin, QString::fromUtf8(sourcePath.c_str())};

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

  for (const QString &f : files) {
    while (true) {
      if (!readerParams.isValid) {
        readerParams = makeCsvLoaderParameters(f, m_uiPlugin, m_paramsDlg);
        if (!readerParams.isValid)
          break;
      }

      CsvFileLoader::DataPack csvData = CsvFileLoader::readFile(m_uiPlugin, f, readerParams);
      if (!csvData.valid) {
        readerParams = CsvFileLoader::Parameters();
        continue;
      }

      appendCsvData(retData, csvData, f, m_paramsDlg->dialog()->parameters());
      break;
    }
  }

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
    return loadCsvFromFileInternal(QStringList{QString::fromUtf8(path.c_str())});
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
