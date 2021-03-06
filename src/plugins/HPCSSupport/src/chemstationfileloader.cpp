#include "chemstationfileloader.h"

#include <libHPCS.h>
#include <plugins/threadeddialog.h>

static
ChemStationFileLoader::Type HPCSTypeToType(const enum HPCS_FileType type)
{
  switch (type) {
  case HPCS_TYPE_CE_ANALOG:
    return ChemStationFileLoader::Type::CE_ANALOG;
    break;
  case HPCS_TYPE_CE_CCD:
    return ChemStationFileLoader::Type::CE_CCD;
    break;
  case HPCS_TYPE_CE_CURRENT:
    return ChemStationFileLoader::Type::CE_CURRENT;
    break;
  case HPCS_TYPE_CE_DAD:
    return ChemStationFileLoader::Type::CE_DAD;
    break;
  case HPCS_TYPE_CE_POWER:
    return ChemStationFileLoader::Type::CE_POWER;
    break;
  case HPCS_TYPE_CE_PRESSURE:
    return ChemStationFileLoader::Type::CE_PRESSURE;
    break;
  case HPCS_TYPE_CE_TEMPERATURE:
    return ChemStationFileLoader::Type::CE_TEMPERATURE;
    break;
  case HPCS_TYPE_CE_VOLTAGE:
    return ChemStationFileLoader::Type::CE_VOLTAGE;
    break;
  default:
    return ChemStationFileLoader::Type::CE_UNKNOWN;
    break;
  }
}

static
ChemStationFileLoader::Wavelength makeWavelength(const enum HPCS_FileType type, const struct HPCS_Wavelength &wl)
{
  if (type == HPCS_TYPE_CE_DAD)
    return wl;
  else
    return {};
}

ChemStationFileLoader::Data::Data(const struct HPCS_MeasuredData *mdata) :
  fileDescription(mdata->file_description),
  sampleInfo(mdata->sample_info),
  operatorName(mdata->operator_name),
  date(HPCSDateToQDate(mdata->date)),
  time(HPCSDateToQTime(mdata->date)),
  methodName(mdata->method_name),
  chemstationRevision(mdata->cs_rev),
  samplingRate(mdata->sampling_rate),
  wavelengthMeasured(makeWavelength(mdata->file_type, mdata->dad_wavelength_msr)),
  wavelengthReference(makeWavelength(mdata->file_type, mdata->dad_wavelength_ref)),
  type(HPCSTypeToType(mdata->file_type)),
  yUnits(mdata->y_units),
  data(HPCSDataToQVector(mdata->data, mdata->data_count)),
  m_valid(true)
{
}

ChemStationFileLoader::Data::Data() :
  samplingRate(0.0),
  type(ChemStationFileLoader::Type::CE_UNKNOWN),
  m_valid(false)
{
}

ChemStationFileLoader::Data::Data(const Data &other) :
  fileDescription(other.fileDescription),
  sampleInfo(other.sampleInfo),
  operatorName(other.operatorName),
  date(other.date),
  time(other.time),
  methodName(other.methodName),
  chemstationVersion(other.chemstationVersion),
  chemstationRevision(other.chemstationRevision),
  samplingRate(other.samplingRate),
  wavelengthMeasured(other.wavelengthMeasured),
  wavelengthReference(other.wavelengthReference),
  type(other.type),
  yUnits(other.yUnits),
  data(other.data),
  m_valid(other.m_valid)
{
}

bool ChemStationFileLoader::Data::isValid() const
{
  return m_valid;
}

ChemStationFileLoader::Data & ChemStationFileLoader::Data::operator=(const Data &other)
{
  const_cast<QString&>(fileDescription) = other.fileDescription;
  const_cast<QString&>(sampleInfo) = other.sampleInfo;
  const_cast<QString&>(operatorName) = other.operatorName;
  const_cast<QDate&>(date) = other.date;
  const_cast<QTime&>(time) = other.time;
  const_cast<QString&>(methodName) = other.methodName;
  const_cast<QString&>(chemstationVersion) = other.chemstationVersion;
  const_cast<QString&>(chemstationRevision) = other.chemstationRevision;
  const_cast<double&>(samplingRate) = other.samplingRate;
  const_cast<Wavelength&>(wavelengthMeasured) = other.wavelengthMeasured;
  const_cast<Wavelength&>(wavelengthReference) = other.wavelengthReference;
  const_cast<Type&>(type) = other.type;
  const_cast<QString&>(yUnits) = other.yUnits;
  const_cast<QVector<QPointF>&>(data) = other.data;

  m_valid = other.m_valid;

  return *this;
}

ChemStationFileLoader::Wavelength::Wavelength(const struct HPCS_Wavelength &wl) :
  wavelength(wl.wavelength),
  interval(wl.interval)
{
}

ChemStationFileLoader::Wavelength::Wavelength() :
  wavelength(0),
  interval(0)
{
}

ChemStationFileLoader::Data ChemStationFileLoader::loadFile(UIPlugin *plugin, const QString &path, const bool reportErrors)
{
  return load(plugin, path, true, reportErrors);
}

ChemStationFileLoader::Data ChemStationFileLoader::loadHeader(UIPlugin *plugin, const QString &path, const bool reportErrors)
{
  return load(plugin, path, false, reportErrors);
}

QVector<QPointF> ChemStationFileLoader::HPCSDataToQVector(const HPCS_TVPair *data, const size_t length)
{
  QVector<QPointF> v;

  for (size_t idx = 0; idx < length; idx++) {
    v.push_back(QPointF(data[idx].time, data[idx].value));
  }

  return v;
}

QDate ChemStationFileLoader::HPCSDateToQDate(const struct HPCS_Date date)
{
  return QDate(date.year, date.month, date.day);
}

QTime ChemStationFileLoader::HPCSDateToQTime(const struct HPCS_Date date)
{
  return QTime(date.hour, date.minute, date.second);
}

ChemStationFileLoader::Data ChemStationFileLoader::load(UIPlugin *plugin, const QString &path, const bool fullFile, const bool reportErrors)
{
  enum HPCS_RetCode ret;
  struct HPCS_MeasuredData *mdata = hpcs_alloc_mdata();

  if (mdata == nullptr)
    return ChemStationFileLoader::Data();

  if (fullFile)
    ret = hpcs_read_mdata(path.toUtf8(), mdata);
  else
    ret = hpcs_read_mheader(path.toUtf8(), mdata);
  if (ret != HPCS_OK) {
    if (reportErrors)
      ThreadedDialog<QMessageBox>::displayWarning(plugin, QMessageBox::tr("Cannot load ChemStation file"),
                                                  QString("%1: %2").arg(QMessageBox::tr("Error reported")).arg(hpcs_error_to_string(ret)));
    hpcs_free_mdata(mdata);
    return ChemStationFileLoader::Data();
  }

  ChemStationFileLoader::Data data(mdata);
  hpcs_free_mdata(mdata);
  return data;
}
