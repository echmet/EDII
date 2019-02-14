#ifndef CSVSUPPORT_H
#define CSVSUPPORT_H

#include <plugins/plugininterface.h>
#include "csvsupport_global.h"

class QStringList;

namespace plugin {

class LoadCsvFileThreadedDialog;

class CSVSUPPORTSHARED_EXPORT CSVSupport : public EDIIPlugin {
public:
  virtual Identifier identifier() const override;
  virtual void destroy() override;
  virtual std::vector<Data> load(const int option) override;
  virtual std::vector<Data> loadHint(const std::string &hintPath, const int option) override;
  virtual std::vector<Data> loadPath(const std::string &path, const int option) override;

  static CSVSupport *instance(UIPlugin *plugin);

private:
  CSVSupport(UIPlugin *plugin);
  virtual ~CSVSupport() override;
  std::vector<Data> loadCsvFromClipboard();
  std::vector<Data> loadCsvFromFile(const std::string &sourcePath);
  std::vector<Data> loadCsvFromFileInternal(const QStringList &files);

  UIPlugin *m_uiPlugin;
  LoadCsvFileThreadedDialog *m_paramsDlg;

  static CSVSupport *s_me;
  static const Identifier s_identifier;
};

extern "C" {
  CSVSUPPORTSHARED_EXPORT EDIIPlugin * initialize(UIPlugin *plugin);
}

} // namespace plugin

#endif // CSVSUPPORT_H
