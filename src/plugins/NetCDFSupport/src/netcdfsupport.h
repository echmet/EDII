#ifndef NETCDFSUPPORT_H
#define NETCDFSUPPORT_H

#include "netcdfsupport_global.h"
#include <plugins/plugininterface.h>

namespace plugin {

class NETCDFSUPPORTSHARED_EXPORT NetCDFSupport : public EDIIPlugin
{
public:
  virtual Identifier identifier() const override;
  virtual void destroy() override;
  virtual std::vector<Data> load(const int option) override;
  virtual std::vector<Data> loadHint(const std::string &hintPath, const int option) override;
  virtual std::vector<Data> loadPath(const std::string &path, const int option) override;

  static NetCDFSupport *initialize(UIPlugin *backend);
  static NetCDFSupport *instance();

private:
  NetCDFSupport(UIPlugin *backend);
  virtual ~NetCDFSupport() override;
  std::vector<Data> loadInternal(const QString &path);
  Data loadOneFile(const QString &path);

  UIPlugin *m_uiPlugin;

  static const Identifier s_identifier;
  static NetCDFSupport *s_me;
};

extern "C" {
  NETCDFSUPPORTSHARED_EXPORT EDIIPlugin * initialize(UIPlugin *backend);
}

} // namespace plugin

#endif // NETCDFSUPPORT_H
