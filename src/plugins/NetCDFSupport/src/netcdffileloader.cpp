﻿#include "netcdffileloader.h"

#include <netcdf.h>
#include <exception>
#include <QString>

#ifdef WIN32
#include <memory>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif // WIN32

#define NC_CHECK(ret, errMsg) if (ret) { nc_close(ncid); throw std::runtime_error{errMsg}; }

namespace plugin {

static const char *actual_run_time_length_var = "actual_run_time_length";
static const char *detector_unit_name = "detector_unit";
static const char *retention_unit_name = "retention_unit";
static const char *ordinate_values_var = "ordinate_values";
static const char *point_number_dim = "point_number";

#ifdef WIN32
static
std::unique_ptr<char[]> toNativeCodepage(const char *utf8_str)
{
  auto wcLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_str, -1, nullptr, 0);
  if (wcLen == 0)
    throw std::runtime_error{"Cannot convert file name to UTF-16 string"};

  auto wstr = std::unique_ptr<wchar_t[]>(new wchar_t[wcLen]);
  if (wstr == nullptr)
    throw std::runtime_error{"Out of memory"};

  wcLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8_str, -1, wstr.get(), wcLen);
  if (wcLen == 0)
    throw std::runtime_error{"Cannot convert file name to UTF-16 string"};

  BOOL dummy;
  auto mbLen = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.get(), -1, nullptr, 0, NULL, &dummy);
  if (mbLen == 0)
    throw std::runtime_error{"Cannot convert file name to native codepage"};

  auto natstr = std::unique_ptr<char[]>(new char[mbLen]);
  mbLen = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wstr.get(), -1, natstr.get(), mbLen, NULL, &dummy);
  if (mbLen == 0)
    throw std::runtime_error{"Cannot convert file name to native codepage"};

  return natstr;
}
#endif // WIN32

static
std::string attributeToString(const int ncid, const int varid, const char *attrName, const size_t len)
{
  std::string sStr{};
  char *str = new char[len + 1];

  int ret = nc_get_att(ncid, varid, attrName, str);
  if (ret) {
     delete [] str;
     throw std::runtime_error{QString{"Cannot read attribute %1"}.arg(attrName).toUtf8().data()};
  }

  str[len] = '\0';
  sStr = std::string{str};
  delete [] str;

  return sStr;
}

NetCDFFileLoader::Data NetCDFFileLoader::load(const QString &path)
{
  int ret;

  /* libNetCFD variables */
  int ncid;
  int rootGrpId;
  int scansVarId;

  /* output */
  std::string detectorUnit{};
  std::string retentionUnit{};
  Scans scans{};

#ifdef WIN32
  auto natPath = toNativeCodepage(path.toUtf8().data());
  ret = nc_open(natPath.get(), NC_NOWRITE, &ncid);
#else
  ret = nc_open(path.toUtf8().data(), NC_NOWRITE, &ncid);
#endif // WIN32
  if (ret)
    throw std::runtime_error{"Cannot open datafile"};

  ret = nc_inq_ncid(ncid, "/", &rootGrpId);
  NC_CHECK(ret, "Cannot get root group ID");

  /* Read those global attributes that we need */
  nc_type attrType;
  size_t attrLen;
  ret = nc_inq_att(ncid, NC_GLOBAL, detector_unit_name, &attrType, &attrLen);
  NC_CHECK(ret, "Attribute \"detector_unit\" not found");
  if (attrType != NC_CHAR) {
    nc_close(ncid);
    throw std::runtime_error{"Unexpected type of attribute \"detector_unit\""};
  }
  detectorUnit = attributeToString(ncid, NC_GLOBAL, detector_unit_name, attrLen);

  ret = nc_inq_att(ncid, NC_GLOBAL, retention_unit_name, &attrType, &attrLen);
  NC_CHECK(ret, "Attribute \"detector_unit\" not found");
  if (attrType != NC_CHAR) {
    nc_close(ncid);
    throw std::runtime_error{"Unexpected type of attribute \"retention_unit\""};
  }
  retentionUnit = attributeToString(ncid, NC_GLOBAL, retention_unit_name, attrLen);

  ret = nc_inq_varid(ncid, ordinate_values_var, &scansVarId);
  NC_CHECK(ret, "Cannot get variable \"ordinate_values\"");

  int scansDims;
  ret = nc_inq_varndims(ncid, scansVarId, &scansDims);
  NC_CHECK(ret, "Cannot get dimensions of \"ordinate_values\"");
  if (scansDims < 1) {
    nc_close(ncid);
    throw std::runtime_error{"\"ordinate_values\" has zero dimension"};
  }

  int scansDim = -1;
  int *scansDimIds = new int[scansDims];
  ret = nc_inq_vardimid(ncid, scansVarId, scansDimIds);
  if (ret) {
    delete [] scansDimIds;
    nc_close(ncid);
    throw std::runtime_error{"Cannot get dimension IDs for \"ordinate_values\""};
  }
  for (int idx = 0; idx < scansDims; idx++) {
    const int dimId = scansDimIds[idx];

    char *dimName = new char[NC_MAX_NAME];
    ret = nc_inq_dimname(ncid, dimId, dimName);
    if (ret) {
      delete [] scansDimIds;
      delete [] dimName;
      throw std::runtime_error{"Cannot get dimension name in \"ordinate_values\""};
    }

    if (strcmp(dimName, point_number_dim) == 0) {
      scansDim = dimId;
      delete [] dimName;
      break;
    }
    delete [] dimName;
  }
  delete [] scansDimIds;
  if (scansDim == -1) {
    nc_close(ncid);
    throw std::runtime_error{"Cannot get dimension ID"};
  }

  size_t dimLen;
  ret = nc_inq_dimlen(ncid, scansDim, &dimLen);
  NC_CHECK(ret, "Cannot get scans dimension length");

  nc_type scansType;
  ret = nc_inq_vartype(ncid, scansVarId, &scansType);
  NC_CHECK(ret, "Cannot check \"ordinate_values\" type");
  if (scansType != NC_FLOAT) {
    nc_close(ncid);
    throw std::runtime_error{"\"ordinate_values\" type is not NC_FLOAT"};
  }

  int runtimeVarId;
  ret = nc_inq_varid(ncid, actual_run_time_length_var, &runtimeVarId);
  NC_CHECK(ret, "Cannot get \"actual_run_time_length\"");

  float runtime;
  const size_t rtFrom[] = {0};
  ret = nc_get_var1_float(ncid, runtimeVarId, rtFrom, &runtime);
  NC_CHECK(ret, "Cannot reat \"actual_run_time_length\"");

  float *theData = new float[dimLen];
  const size_t from[] = {0};
  const size_t to[] = {dimLen};
  ret = nc_get_vara(ncid, scansVarId, from, to, theData);
  if (ret) {
    delete [] theData;
    nc_close(ncid);
    throw std::runtime_error{"Cannot read scans data"};
  }

  const float samplingRate = runtime / static_cast<float>(dimLen);

  for (size_t idx = 0; idx < dimLen; idx++) {
    const float time = samplingRate * idx;
    scans.emplace_back(time, theData[idx]);
  }

  delete [] theData;
  nc_close(ncid);

  return Data{std::move(scans), std::move(retentionUnit), std::move(detectorUnit)};
}

} // namespace plugin
