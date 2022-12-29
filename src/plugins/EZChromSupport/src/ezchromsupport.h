// vim: sw=4 ts=4 sts=4 expandtab :

#ifndef EZCHROMSUPPORT_H
#define EZCHROMSUPPORT_H

#include "ezchromsupport_global.h"
#include <plugins/plugininterface.h>

#include <QString>

#include <set>
#include <string>

namespace plugin {

class AvailableChannels;

class EZCHROMSUPPORTSHARED_EXPORT EZChromSupport : public EDIIPlugin {
public:
  virtual Identifier identifier() const override;
  virtual void destroy() override;
  virtual std::vector<Data> load(const int option) override;
  virtual std::vector<Data> loadHint(const std::string &hintPath, const int option) override;
  virtual std::vector<Data> loadPath(const std::string &path, const int option) override;

  static EZChromSupport *instance(UIPlugin *plugin);

private:
  EZChromSupport(UIPlugin *plugin);
  virtual ~EZChromSupport() override;
  std::vector<Data> loadInteractive(const std::string &hintPath);
  std::vector<Data> loadSingleFile(const QString &path, std::set<std::string> &selectedChannels);

  UIPlugin *m_uiPlugin;

  static Identifier s_identifier;
  static EZChromSupport *s_me;
};

extern "C" {
  EZCHROMSUPPORTSHARED_EXPORT EDIIPlugin * initialize(UIPlugin *plugin);
}

} // namespace plugin

#endif // EZCHROMSUPPORT_H
