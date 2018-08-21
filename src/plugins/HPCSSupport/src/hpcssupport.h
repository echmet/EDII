#ifndef HPCSSUPPORT_H
#define HPCSSUPPORT_H

#include "hpcssupport_global.h"
#include "chemstationbatchloader.h"

#include <plugins/plugininterface.h>
#include <QMutex>
#include <QStringList>
#include <QSize>

class QFileSystemModel;

namespace plugin {

class LoadChemStationDataThreadedDialog;

class HPCSSUPPORTSHARED_EXPORT HPCSSupport : public EDIIPlugin
{
public:
  virtual Identifier identifier() const override;
  virtual void destroy() override;
  virtual std::vector<Data> load(const int option) override;
  virtual std::vector<Data> loadHint(const std::string &hintPath, const int option) override;
  virtual std::vector<Data> loadPath(const std::string &path, const int option) override;

  static HPCSSupport * instance(UIPlugin *plugin);

private:
  HPCSSupport(UIPlugin *plugin);
  virtual ~HPCSSupport() override;
  std::string chemStationTypeToString(const ChemStationFileLoader::Type type);
  QString defaultPath() const;
  bool isDirectoryUsable(const QString &path) const;
  Data loadChemStationFileSingle(const QString &path);
  void loadChemStationFileMultipleDirectories(std::vector<Data> &dataVec, const QStringList &dirPaths, const ChemStationBatchLoader::Filter &filter);
  void loadChemStationFileWholeDirectory(std::vector<Data> &dataVec, const QString &path, const ChemStationBatchLoader::Filter &filter);
  std::vector<Data> loadInteractive(LoadChemStationDataThreadedDialog *dlg);
  LoadChemStationDataThreadedDialog * makeLoadDialog(const QString &path);

  UIPlugin *m_uiPlugin;

  QMutex m_dlgSizeLock;
  QMutex m_lastPathLock;

  QSize m_lastChemStationDlgSize;
  QString m_lastChemStationPath;

  QFileSystemModel *m_fsModel;

  const QStringList m_defaultPathsToTry;

  static HPCSSupport *s_me;
  static const Identifier s_identifier;
};

extern "C" {
  HPCSSUPPORTSHARED_EXPORT EDIIPlugin * initialize(UIPlugin *plugin);
}

} // namespace plugin

#endif // HPCSSUPPORT_H
