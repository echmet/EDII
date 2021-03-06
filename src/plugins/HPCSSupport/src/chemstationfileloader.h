#ifndef CHEMSTATIONFILELOADER_H
#define CHEMSTATIONFILELOADER_H

#include <QDate>
#include <QPointF>
#include <QString>
#include <QVector>

class UIPlugin;

struct HPCS_Date;
struct HPCS_TVPair;
struct HPCS_Wavelength;

class ChemStationFileLoader
{
public:
  enum class Type {
    CE_ANALOG,
    CE_CCD,
    CE_CURRENT,
    CE_DAD,
    CE_POWER,
    CE_PRESSURE,
    CE_TEMPERATURE,
    CE_VOLTAGE,
    CE_UNKNOWN
  };

  struct Wavelength {
    Wavelength(const struct HPCS_Wavelength &w);
    Wavelength();

    int wavelength;
    int interval;
  };

  class Data {
  public:
    Data(const struct HPCS_MeasuredData *mdata);
    Data();
    Data(const Data &other);
    bool isValid() const;
    Data & operator=(const Data &other);

    const QString fileDescription;
    const QString sampleInfo;
    const QString operatorName;
    const QDate date;
    const QTime time;
    const QString methodName;
    const QString chemstationVersion;
    const QString chemstationRevision;
    const double samplingRate;
    const Wavelength wavelengthMeasured;
    const Wavelength wavelengthReference;
    const Type type;
    const QString yUnits;
    const QVector<QPointF> data;
  private:
    bool m_valid;
  };

  ChemStationFileLoader() = delete;

  static Data loadHeader(UIPlugin *plugin, const QString &path, const bool reportErrors = false);
  static Data loadFile(UIPlugin *plugin, const QString &path, const bool reportErrors = false);

private:
  static QDate HPCSDateToQDate(const struct HPCS_Date date);
  static QVector<QPointF> HPCSDataToQVector(const struct HPCS_TVPair *data, const size_t length);
  static QTime HPCSDateToQTime(const struct HPCS_Date date);
  static Data load(UIPlugin *plugin, const QString &path, const bool fullFile, const bool reportErrors);
};

#endif // CHEMSTATIONFILELOADER_H
