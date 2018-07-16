#include "netcdfsupport.h"
#include "netcdffileloader.h"

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
        auto dlg = new QFileDialog{nullptr, QObject::tr("Pick a NetCDF file"), this->m_path};

        dlg->setNameFilter("NetCDF file (*.cdf *.CDF)");
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

const Identifier NetCDFSupport::s_identifier{"NetCDF (Common Data Format) file format support", "NetCDF", "NetCDF", {}};
NetCDFSupport *NetCDFSupport::s_me{nullptr};

EDIIPlugin::~EDIIPlugin()
{
}

NetCDFSupport::NetCDFSupport(UIPlugin *plugin) :
  m_uiPlugin{plugin}
{
}

NetCDFSupport::~NetCDFSupport()
{
}

void NetCDFSupport::destroy()
{
  delete s_me;
}

Identifier NetCDFSupport::identifier() const
{
  return s_identifier;
}

NetCDFSupport * NetCDFSupport::initialize(UIPlugin *plugin)
{
  if (s_me == nullptr)
    s_me = new (std::nothrow) NetCDFSupport{plugin};

  return s_me;
}

NetCDFSupport *NetCDFSupport::instance()
{
  return s_me;
}

std::vector<Data> NetCDFSupport::load(const int option)
{
  (void)option;

  return loadInternal("");
}

std::vector<Data> NetCDFSupport::loadHint(const std::string &hintPath, const int option)
{
  (void)option;

  return loadInternal(QString::fromStdString(hintPath));
}

std::vector<Data> NetCDFSupport::loadPath(const std::string &path, const int option)
{
  (void)option;

  try {
    return std::vector<Data>{loadOneFile(QString::fromStdString(path))};
  } catch (std::runtime_error &ex) {
    ThreadedDialog<QMessageBox>::displayWarning(m_uiPlugin, QObject::tr("Failed to load NetCDF file"), QString{ex.what()});
    return std::vector<Data>{};
  }
}

std::vector<Data> NetCDFSupport::loadInternal(const QString &path)
{
  OpenFileThreadedDialog dlgWrap{m_uiPlugin, path};

  int ret = dlgWrap.execute();
  if (ret != QDialog::Accepted)
    return std::vector<Data>{};

  QFileDialog *dlg = dlgWrap.dialog();

  const auto &files = dlg->selectedFiles();
  if (files.length() < 1)
    return std::vector<Data>{};

  std::vector<Data> retData{};
  for (const QString &filePath : files) {
    try {
      retData.emplace_back(loadOneFile(filePath));
    } catch (std::runtime_error &ex) {
      ThreadedDialog<QMessageBox>::displayWarning(m_uiPlugin, QObject::tr("Failed to load NetCDF file"), QString{ex.what()});
    }
  }

  return retData;
}

Data NetCDFSupport::loadOneFile(const QString &filePath)
{
  NetCDFFileLoader::Data data = NetCDFFileLoader::load(filePath);
  QString fileName = QFileInfo{filePath}.fileName();

  return Data{fileName.toStdString(), "", filePath.toStdString(),
              "Time", "Signal",
              std::move(data.xUnits),
              std::move(data.yUnits),
              std::move(data.scans)};
}

EDIIPlugin * initialize(UIPlugin *plugin)
{
  return NetCDFSupport::initialize(plugin);
}

} // namespace plugin
