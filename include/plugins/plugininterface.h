#ifndef ECHMET_EDII_PLUGININTERFACE_H
#define ECHMET_EDII_PLUGININTERFACE_H

#include <string>
#include <tuple>
#include <vector>

class UIPlugin;

namespace plugin {

/*!
 * \brief Class representing the loaded data.
 */
class Data {
public:
  Data()
  {
  }

  Data(std::string _name, std::string _dataId, std::string _path,
       std::string _xDesc, std::string _yDesc, std::string _xUnit, std::string _yUnit,
       std::vector<std::tuple<double, double>> _pts) noexcept :
    name{std::move(_name)}, dataId{std::move(_dataId)}, path{std::move(_path)},
    xDescription{std::move(_xDesc)}, yDescription{std::move(_yDesc)},
    xUnit{std::move(_xUnit)}, yUnit{std::move(_yUnit)},
    datapoints{std::move(_pts)}
  {
  }

  const std::string name;                                     /*!< Name of the source file */
  const std::string dataId;                                   /*!< Optional identifier of the data block */
  const std::string path;                                     /*!< Absolute path to the source file */
  const std::string xDescription;                             /*!< Description (label) of X axis */
  const std::string yDescription;                             /*!< Description (label) of Y axis */
  const std::string xUnit;                                    /*!< Units of data on X axis */
  const std::string yUnit;                                    /*!< Units of data on Y axis */
  const std::vector<std::tuple<double, double>> datapoints;   /*!< [X, Y] tuples of datapoints */
};

/*!
 * Identifier of a specific backed
 */
class Identifier {
public:
  const std::string longDescription;            /*!< Human-readable description of the loader backend. This should be as descriptive as possible. */
  const std::string shortDescription;           /*!< Breif description of the loader. The string should be suitable for display in menus and other UI elements. */
  const std::string tag;                        /*!< Unique ID tag */
  const std::vector<std::string> loadOptions;   /*!< Description of each modifier of loading behavior */
};

class EDIIPlugin {
public:
  /*!
   * \brief Destroys the loader object.
   */
  virtual void destroy() = 0;

  /*!
   * \brief Returns the identifier for this loader backend .
   * \return Identifier object.
   */
  virtual Identifier identifier() const = 0;

  /*!
   * \brief Loads data interactively.
   * \param option Loading behavior modifier.
   * \return Vector of <tt>Data</tt> objects, each corresponding to one loaded data file.
   */
  virtual std::vector<Data> load(const int option) = 0;

  /*!
   * \brief Loads data file interactively with a hint where to load data from.
   * \param hintPath Path where to look for the data. This may be adjusted form interactive prompt by the user.
   * \param option Loading behavior modifier.
   * \return Vector of <tt>Data</tt> objects, each corresponding to one loaded data file.
   */
  virtual std::vector<Data> loadHint(const std::string &hintPath, const int option) = 0;

  /*!
   * \brief Loads data from a given path.
   * \param path Path to a file or directory where to load data from.
   * \param option Loading behavior modifier.
   * \return Vector of <tt>Data</tt> objects, each corresponding to one loaded data file.
   */
  virtual std::vector<Data> loadPath(const std::string &path, const int option) = 0;
protected:
  virtual ~EDIIPlugin() = 0;
};

typedef EDIIPlugin *(*PluginInitializer)(UIPlugin *);

} // namespace plugin

#endif // ECHMET_EDII_PLUGININTERFACE_H
