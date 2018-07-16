#ifndef ASCSUPPORT_H
#define ASCSUPPORT_H

#include "ascsupport_global.h"
#include "ascsupport_handlers.h"
#include <plugins/plugininterface.h>
#include "supportedencodings.h"
#include <list>

namespace plugin {

class AvailableChannels;

class ASCSUPPORTSHARED_EXPORT ASCSupport : public EDIIPlugin {
public:
  typedef std::pair<std::string, bool> SelectedChannel;
  typedef std::vector<SelectedChannel> SelectedChannelsVec;

  virtual Identifier identifier() const override;
  virtual void destroy() override;
  virtual std::vector<Data> load(const int option) override;
  virtual std::vector<Data> loadHint(const std::string &hintPath, const int option) override;
  virtual std::vector<Data> loadPath(const std::string &path, const int option) override;

  static ASCSupport *instance(UIPlugin *plugin);

private:
  ASCSupport(UIPlugin *plugin);
  virtual ~ASCSupport() override;
  const EntryHandler * getHandler(const std::string &key);
  std::vector<Data> loadInteractive(const std::string &hintPath);
  std::vector<Data> loadInternal(const std::string &path, AvailableChannels &availChans, SelectedChannelsVec &selChans,
                                 const SupportedEncodings::EncodingType &encoding);
  void parseHeader(ASCContext &ctx, const std::list<std::string> &header);

  UIPlugin *m_uiPlugin;

  static Identifier s_identifier;
  static ASCSupport *s_me;
  static EntryHandlersMap s_handlers;
};

extern "C" {
  ASCSUPPORTSHARED_EXPORT EDIIPlugin * initialize(UIPlugin *plugin);
}

} // namespace plugin

#endif // ASCSUPPORT_H
