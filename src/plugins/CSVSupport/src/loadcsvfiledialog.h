#ifndef LOADCSVFILEDIALOG_H
#define LOADCSVFILEDIALOG_H

#include <QDialog>
#include <QStandardItemModel>

namespace plugin {

namespace Ui {
class LoadCsvFileDialog;
}

class LoadCsvFileDialog : public QDialog
{
  Q_OBJECT
public:
  enum class HeaderHandling {
    NO_HEADER,             /*!< CSV file has no header */
    HEADER_WITH_UNITS,     /*!< CSV file header contains the unit of the scale */
    HEADER_WITHOUT_UNITS   /*!< CSV file header does not contain the unit of the scale */
  };
  Q_ENUM(HeaderHandling)

  class Parameters {
  public:
    Parameters();
    Parameters(const QString &delimiter, const QChar &decimalSeparator,
               const int xColumn, const int yColumn,
	       const bool multipleYcols,
               const QString &xType, const QString &yType, const QString &xUnit, const QString &yUnit,
               const HeaderHandling header, const int linesToSkip,
               const QString &encodingId);
    Parameters(const Parameters &other);

    const QString delimiter;
    const QChar decimalSeparator;
    const int xColumn;
    const int yColumn;
    const bool multipleYcols;
    const QString xType;
    const QString yType;
    const QString xUnit;
    const QString yUnit;
    const HeaderHandling header;
    const int linesToSkip;
    const QString encodingId;

    Parameters &operator=(const Parameters &other);
  };

  explicit LoadCsvFileDialog(const QString &source = "", QWidget *parent = nullptr);
  ~LoadCsvFileDialog();
  void addEncoding(const QString &id, const QString &name, const bool canHaveBom);
  Parameters parameters() const;
  void refreshPreview();
  void setSource(const QString &path);
  void updatePreview(const bool show);

private:
  Parameters makeParameters();

  Ui::LoadCsvFileDialog *ui;
  Parameters m_parameters;
  QStandardItemModel m_encodingsModel;
  QString m_source;

  static const QString s_qlDelimiterToolTip;
  static const QString s_qlDecimalSeparatorToolTip;
  static const QString s_qlFirstLineIsHeaderToolTip;

private slots:
  void onCancelClicked();
  void onHeaderHandlingChanged(const int idx);
  void onLoadClicked();
  void onMultipleYColsClicked();
  void onShowPreviewClicked(const bool checked);
};

} // namespace plugin

#endif // LOADCSVFILEDIALOG_H
