#ifndef CHEMSTATIONBATCHLOADER_H
#define CHEMSTATIONBATCHLOADER_H

#include <QDir>
#include "chemstationfileloader.h"

class UIPlugin;

class ChemStationBatchLoader
{
public:
  class Filter {
  public:
    explicit Filter();
    explicit Filter(const ChemStationFileLoader::Type type, const int wlMeasured, const int wlReference);
    Filter(const Filter &other);
    Filter & operator=(const Filter &other);

    const ChemStationFileLoader::Type type;
    const int wlMeasured;
    const int wlReference;
    const bool isValid;
  };

  typedef QVector<ChemStationFileLoader::Data> CHSDataVec;
  typedef QVector<CHSDataVec> CHSDataVecVec;

  ChemStationBatchLoader() = delete;

  static bool filterMatches(const ChemStationFileLoader::Data &chData, const Filter &filter);
  static CHSDataVec getCommonTypes(UIPlugin *plugin, const QString &path);
  static CHSDataVec getCommonTypes(UIPlugin *plugin, const QStringList &dirPaths);
  static QStringList getFilesList(UIPlugin *plugin, const QString &path, const Filter &filter);
  static QStringList getFilesList(UIPlugin *plugin, const QStringList &dirPaths, const Filter &filter);

private:
  static CHSDataVec getChemStationFiles(UIPlugin *plugin, const QDir &dir);
  static CHSDataVec intersection(const CHSDataVecVec &chvVec);
  static QStringList walkDirectory(UIPlugin *plugin, const QString &path, const Filter &filter);

};

#endif // CHEMSTATIONBATCHLOADER_H
