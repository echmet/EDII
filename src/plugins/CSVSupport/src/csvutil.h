#ifndef CSVUTIL_H
#define CSVUTIL_H

#include "csvfileloader.h"
#include "loadcsvfiledialog.h"

namespace plugin {

inline
CsvFileLoader::Parameters dialogParamsToLoaderParams(const LoadCsvFileDialog::Parameters &dlgParams)
{
  if (dlgParams.delimiter.length() != 1 && (dlgParams.delimiter.compare("\\t") != 0)) {
    std::string msg = QObject::tr("Delimiter must be a single character or '\\t' to represent TAB.").toStdString();
    throw std::runtime_error{msg};
  }
  if (dlgParams.decimalSeparator == dlgParams.delimiter) {
    std::string msg = QObject::tr("Delimiter and decimal separator cannot be the same character.").toStdString();
    throw std::runtime_error{msg};
  }

  if (dlgParams.xColumn == dlgParams.yColumn) {
    std::string msg = QObject::tr("X and Y columns cannot be the same").toStdString();
    throw std::runtime_error{msg};
  }

  QChar delimiter;
  if (dlgParams.delimiter.compare("\\t") == 0)
    delimiter = '\t';
  else
    delimiter = dlgParams.delimiter.at(0);

  return CsvFileLoader::Parameters(delimiter, dlgParams.decimalSeparator,
                                   dlgParams.xColumn, dlgParams.yColumn,
                                   dlgParams.multipleYcols,
                                   dlgParams.header != LoadCsvFileDialog::HeaderHandling::NO_HEADER,
                                   dlgParams.linesToSkip,
                                   dlgParams.encodingId);
}

} // plugin

#endif // CSVUTIL_H
