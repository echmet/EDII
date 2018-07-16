#ifndef NETCDFFILELOADER_H
#define NETCDFFILELOADER_H

#include <string>
#include <tuple>
#include <vector>

class QString;

namespace plugin {

class NetCDFFileLoader
{
public:
  typedef std::vector<std::tuple<double, double>> Scans;
  class Data {
  public:
    Data() :
      scans(Scans{}),
      xUnits(std::string{}),
      yUnits(std::string{})
    {}
    Data(Scans &&scans, std::string &&xUnits, std::string &&yUnits) :
      scans(scans),
      xUnits(xUnits),
      yUnits(yUnits)
    {}

    const Scans scans;
    const std::string xUnits;
    const std::string yUnits;
  };

  NetCDFFileLoader() = delete;

  static Data load(const QString &path);
};

} // namespace plugin

#endif // NETCDFFILELOADER_H
