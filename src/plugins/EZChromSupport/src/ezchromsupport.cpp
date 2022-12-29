// vim: sw=4 ts=4 sts=4 expandtab :

#include "ezchromsupport.h"
#include "ui/selectchannelsdialog.h"
#include <ezfish.h>

#include <plugins/threadeddialog.h>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QVector>

#include <algorithm>
#include <set>
#include <utility>

namespace plugin {

static
void reportWarning(UIPlugin *plugin, const QString &warning)
{
  ThreadedDialog<QMessageBox>::displayWarning(plugin, QObject::tr("Cannot read ASC file"), warning);
}

class OpenFileThreadedDialog : public ThreadedDialog<QFileDialog>
{
public:
  OpenFileThreadedDialog(UIPlugin *plugin, const QString &path) :
    ThreadedDialog<QFileDialog>{
      plugin,
      [this]() {
        auto dlg = new QFileDialog{nullptr, QObject::tr("Pick an EZChrom/32Karat file"), this->m_path};

        dlg->setNameFilter("EZChrom/32Karat file (*.dat *.DAT)");
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

class SelectChannelsThreadedDialog : public ThreadedDialog<SelectChannelsDialog>
{
public:
  SelectChannelsThreadedDialog(UIPlugin *plugin, const std::vector<std::tuple<std::string, std::string>> &availChans) :
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
  const std::vector<std::tuple<std::string, std::string>> m_availChans;
};

static
auto fileList(UIPlugin *plugin, const QString &hintPath) -> QStringList
{
  OpenFileThreadedDialog dlgWrap{plugin, hintPath};

  if (dlgWrap.execute() != QDialog::Accepted)
    return {};

  return dlgWrap.dialog()->selectedFiles();
}

static
auto maybeUpdateSelectedChannels(UIPlugin *plugin, std::set<std::string> &selected, const std::vector<std::tuple<std::string, std::string>> &available)
{
    bool update = selected.empty();
    if (!update) {
        for (const auto &name : selected) {
            if (std::find_if(available.cbegin(), available.cend(), [&name](const auto &v) { return std::get<0>(v) == name; }) == available.cend()) {
                update = true;
                break;
            }
        }
    }

    if (update) {
        selected.clear();

        SelectChannelsThreadedDialog dlgWrap{plugin, available};
        int ret = dlgWrap.execute();
        if (ret != QDialog::Accepted) {
            for (const auto &chan : available)
                selected.emplace(std::get<0>(chan));
        } else {
            for (const auto &item : dlgWrap.dialog()->selection()) {
                if (item.second)
                    selected.emplace(item.first);
            }
        }
    }
}

static
auto readFile(const QString &path) -> QByteArray
{
    QFile fh{path};
    if (!fh.open(QFile::ReadOnly))
        throw std::runtime_error{"Cannot open file"};

    return fh.readAll();
}

EDIIPlugin::~EDIIPlugin()
{
}

Identifier EZChromSupport::s_identifier{"EZChrom/32Karat data", "EZChrom/32Karat", "EZChrom", {}};
EZChromSupport *EZChromSupport::s_me{nullptr};

EZChromSupport::EZChromSupport(UIPlugin *plugin) :
    m_uiPlugin{plugin}
{
}

EZChromSupport::~EZChromSupport()
{
}

void EZChromSupport::destroy()
{
    delete s_me;
    s_me = nullptr;
}

Identifier EZChromSupport::identifier() const
{
    return s_identifier;
}

EZChromSupport * EZChromSupport::instance(UIPlugin *plugin)
{
    if (s_me == nullptr)
        s_me = new (std::nothrow) EZChromSupport{plugin};

    return s_me;
}

std::vector<Data> EZChromSupport::load(const int options)
{
    (void)options;

    return loadInteractive("");
}

std::vector<Data> EZChromSupport::loadHint(const std::string &path, const int option)
{
    (void)option;

    return loadInteractive(path);
}

std::vector<Data> EZChromSupport::loadPath(const std::string &path, const int option)
{
    (void)option;

    std::set<std::string> channels{};
    return loadSingleFile(QString::fromUtf8(path.data()), channels);
}

std::vector<Data> EZChromSupport::loadInteractive(const std::string &hintPath)
{
    auto files = fileList(m_uiPlugin, QString::fromUtf8(hintPath.data()));

    std::vector<Data> allData;

    std::set<std::string> channels{};
    for (const auto &file : files) {
        try {
            auto data = loadSingleFile(file, channels);
            for (auto &&d : data)
                allData.push_back(std::move(d));
        } catch (const std::runtime_error &ex) {
            reportWarning(m_uiPlugin, QString::fromUtf8(ex.what()));
        }
    }

    return allData;
}

std::vector<Data> EZChromSupport::loadSingleFile(const QString &path, std::set<std::string> &selectedChannels)
{
    auto fileName = QFileInfo{path}.fileName();
    if (fileName.isEmpty())
        throw std::runtime_error{"Cannot determine file name"};

    const auto bytes = readFile(path);

    auto ezfTraces = ezf_empty_traces();
    auto tRet = ezf_read(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size(), &ezfTraces);
    if (tRet != EzfResult::Success)
        throw std::runtime_error{ezf_error_to_string(tRet)};

    std::vector<std::tuple<std::string, std::string>> channelsInTrace{};
    for (size_t idx = 0; idx < ezfTraces.n_traces; idx++) {
        const auto &trace = ezfTraces.traces[idx];

        std::string name{trace.name};
        if (std::find_if(channelsInTrace.begin(), channelsInTrace.end(), [&name](const auto &v) { return std::get<0>(v) == name; }) != channelsInTrace.end()) {
            ezf_release_traces(&ezfTraces);

            throw std::runtime_error{"Data file contains multiple traces with the same name. We do not allow this."};
        }
        channelsInTrace.emplace_back(std::move(name), std::string(trace.y_units));
    }

    maybeUpdateSelectedChannels(m_uiPlugin, selectedChannels, channelsInTrace);

    std::vector<Data> data{};
    data.reserve(ezfTraces.n_traces);
    for (size_t idx = 0; idx < ezfTraces.n_traces; idx++) {
        const auto &trace = ezfTraces.traces[idx];
        std::string name{trace.name};

        if (selectedChannels.find(name) == selectedChannels.end())
            continue;

        std::vector<std::tuple<double, double>> dataPoints{};
        dataPoints.reserve(trace.n_scans);
        const auto &scans = trace.scans;
        for (size_t sdx = 0; sdx < trace.n_scans; sdx++) {
            const auto &scan = scans[sdx];
            dataPoints.emplace_back(scan.x, scan.y);
        }

        data.push_back(
            Data{
                std::string(fileName.toUtf8().data()),
                std::move(name),
                std::string(path.toUtf8().data()),
                "Time",
                "Signal",
                std::string(trace.x_units),
                std::string(trace.y_units),
                std::move(dataPoints)
            }
        );
    }

    ezf_release_traces(&ezfTraces);

    return data;
}

EDIIPlugin * initialize(UIPlugin *plugin)
{
    return EZChromSupport::instance(plugin);
}

} // namespace plugin
