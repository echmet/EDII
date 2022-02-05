// vim: set sw=2 ts=2 sts=2 expandtab:

#include "loadcsvfiledialog.h"
#include "ui_loadcsvfiledialog.h"
#include "csvfileloader.h"

namespace plugin {

const QString LoadCsvFileDialog::s_qlDelimiterToolTip = QObject::tr("Character that separates columns in the data file");
const QString LoadCsvFileDialog::s_qlDecimalSeparatorToolTip = QObject::tr("Character that separates integer and decimal parf on a number in the data file");
const QString LoadCsvFileDialog::s_qlFirstLineIsHeaderToolTip = QObject::tr("Check when first line in the data file contains captions of the columns");

LoadCsvFileDialog::Parameters::Parameters() :
  delimiter(""),
  decimalSeparator('\0'),
  xColumn(0),
  yColumn(0),
  multipleYcols(false),
  xType(""),
  yType(""),
  xUnit(""),
  yUnit(""),
  header(HeaderHandling::NO_HEADER),
  linesToSkip(0),
  encodingId("")
{
}

LoadCsvFileDialog::Parameters::Parameters(const QString &delimiter, const QChar &decimalSeparator,
                                          const int xColumn, const int yColumn,
                                          const bool multipleYcols,
                                          const QString &xType, const QString &yType, const QString &xUnit, const QString &yUnit,
                                          const HeaderHandling header, const int linesToSkip,
                                          const QString &encodingId) :
  delimiter(delimiter),
  decimalSeparator(decimalSeparator),
  xColumn(xColumn),
  yColumn(yColumn),
  multipleYcols(multipleYcols),
  xType(xType),
  yType(yType),
  xUnit(xUnit),
  yUnit(yUnit),
  header(header),
  linesToSkip(linesToSkip),
  encodingId(encodingId)
{
}

LoadCsvFileDialog::Parameters::Parameters(const Parameters &other) :
  delimiter(other.delimiter),
  decimalSeparator(other.decimalSeparator),
  xColumn(other.xColumn),
  yColumn(other.yColumn),
  multipleYcols(other.multipleYcols),
  xType(other.xType),
  yType(other.yType),
  xUnit(other.xUnit),
  yUnit(other.yUnit),
  header(other.header),
  linesToSkip(other.linesToSkip),
  encodingId(other.encodingId)
{
}

LoadCsvFileDialog::Parameters &LoadCsvFileDialog::Parameters::operator=(const Parameters &other)
{
  const_cast<QString&>(delimiter) = other.delimiter;
  const_cast<QChar&>(decimalSeparator) = other.decimalSeparator;
  const_cast<int&>(xColumn) = other.xColumn;
  const_cast<int&>(yColumn) = other.yColumn;
  const_cast<bool&>(multipleYcols) = other.multipleYcols;
  const_cast<QString&>(xType) = other.xType;
  const_cast<QString&>(yType) = other.yType;
  const_cast<QString&>(xUnit) = other.xUnit;
  const_cast<QString&>(yUnit) = other.yUnit;
  const_cast<HeaderHandling&>(header) = other.header;
  const_cast<int&>(linesToSkip) = other.linesToSkip;
  const_cast<QString&>(encodingId) = other.encodingId;

  return *this;
}

LoadCsvFileDialog::LoadCsvFileDialog(const QString &source, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::LoadCsvFileDialog),
  m_source(source)
{
  ui->setupUi(this);

  connect(ui->qpb_cancel, &QPushButton::clicked, this, &LoadCsvFileDialog::onCancelClicked);
  connect(ui->qpb_load, &QPushButton::clicked, this, &LoadCsvFileDialog::onLoadClicked);

  ui->qcbox_encoding->setModel(&m_encodingsModel);

  ui->qcbox_decimalSeparator->addItem("Period (.)", QChar('.'));
  ui->qcbox_decimalSeparator->addItem("Comma (,)", QChar(','));

  ui->ql_delimiter->setToolTip(s_qlDelimiterToolTip);
  ui->ql_decimalSeparator->setToolTip(s_qlDecimalSeparatorToolTip);

  /* Add header handling options */
  ui->qcbox_headerHandling->addItem(tr("No header"), QVariant::fromValue<HeaderHandling>(HeaderHandling::NO_HEADER));
  ui->qcbox_headerHandling->addItem(tr("Header contains units"), QVariant::fromValue<HeaderHandling>(HeaderHandling::HEADER_WITH_UNITS));
  ui->qcbox_headerHandling->addItem(tr("Header without units"), QVariant::fromValue<HeaderHandling>(HeaderHandling::HEADER_WITHOUT_UNITS));

  connect(ui->qcbox_headerHandling, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &LoadCsvFileDialog::onHeaderHandlingChanged);
  connect(ui->qcb_multipleYcols, &QCheckBox::clicked, this, &LoadCsvFileDialog::onMultipleYColsClicked);

  connect(ui->qcb_showPreview, &QCheckBox::clicked, this, &LoadCsvFileDialog::onShowPreviewClicked);

  connect(ui->qcbox_encoding, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &LoadCsvFileDialog::refreshPreview);
  connect(ui->qspbox_numLinesPreview, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LoadCsvFileDialog::refreshPreview);

  ui->qpte_preview->setWordWrapMode(QTextOption::NoWrap);
  ui->qpte_preview->setReadOnly(true);

  onMultipleYColsClicked();
  updatePreview(false);
}

LoadCsvFileDialog::~LoadCsvFileDialog()
{
  delete ui;
}

void LoadCsvFileDialog::addEncoding(const QString &id, const QString &name, const bool canHaveBom)
{
  QStandardItem *item = new QStandardItem(name);

  if (item == nullptr)
    return;

  item->setData(id, Qt::UserRole + 1);
  item->setData(canHaveBom, Qt::UserRole + 2);
  m_encodingsModel.appendRow(item);
}

void LoadCsvFileDialog::onCancelClicked()
{
  reject();
}

void LoadCsvFileDialog::onHeaderHandlingChanged(const int idx)
{
  QVariant v = ui->qcbox_headerHandling->itemData(idx);

  if (!v.canConvert<HeaderHandling>())
    return;

  HeaderHandling h = v.value<HeaderHandling>();

  switch (h) {
  case HeaderHandling::NO_HEADER:
    ui->qle_xType->setEnabled(true);
    ui->qle_yType->setEnabled(true);
    ui->qle_xUnit->setEnabled(true);
    ui->qle_yUnit->setEnabled(true);
    break;
  case HeaderHandling::HEADER_WITH_UNITS:
    ui->qle_xType->setEnabled(false);
    ui->qle_yType->setEnabled(false);
    ui->qle_xUnit->setEnabled(false);
    ui->qle_yUnit->setEnabled(false);
    break;
  case HeaderHandling::HEADER_WITHOUT_UNITS:
    ui->qle_xType->setEnabled(false);
    ui->qle_yType->setEnabled(false);
    ui->qle_xUnit->setEnabled(true);
    ui->qle_yUnit->setEnabled(true);
    break;
  }
}

LoadCsvFileDialog::Parameters LoadCsvFileDialog::makeParameters()
{
  QStandardItem *item = m_encodingsModel.item(ui->qcbox_encoding->currentIndex());
  if (item == nullptr)
    throw std::runtime_error{"No item in encodingsModel"};

  QVariant v = ui->qcbox_headerHandling->currentData();
  if (!v.canConvert<HeaderHandling>())
    throw std::runtime_error{"Invalid HeaderHandling"};

  HeaderHandling h = v.value<HeaderHandling>();
  auto linesToSkip = ui->qspbox_linesToSkip->value();

  return Parameters(ui->qle_delimiter->text(),
                    ui->qcbox_decimalSeparator->currentData().toChar(),
                    ui->qspbox_xColumn->value(), ui->qspbox_yColumn->value(),
                    ui->qcb_multipleYcols->checkState() == Qt::Checked,
                    ui->qle_xType->text(), ui->qle_yType->text(),
                    ui->qle_xUnit->text(), ui->qle_yUnit->text(),
                    h, linesToSkip,
                    item->data(Qt::UserRole + 1).toString());
}

void LoadCsvFileDialog::onLoadClicked()
{
  try {
    m_parameters = makeParameters();
    accept();
  } catch (const std::runtime_error &) {
    reject();
  }
}

void LoadCsvFileDialog::onMultipleYColsClicked()
{
	const bool colsAdjustable = ui->qcb_multipleYcols->checkState() != Qt::Checked;

	ui->qspbox_xColumn->setEnabled(colsAdjustable);
	ui->qspbox_yColumn->setEnabled(colsAdjustable);
}

LoadCsvFileDialog::Parameters LoadCsvFileDialog::parameters() const
{
  return m_parameters;
}

void LoadCsvFileDialog::onShowPreviewClicked(const bool checked)
{
  updatePreview(checked);
}

void LoadCsvFileDialog::refreshPreview()
{
  if (!ui->qcb_showPreview->isChecked())
    return;

  try {
    QStandardItem *item = m_encodingsModel.item(ui->qcbox_encoding->currentIndex());
    if (item == nullptr)
      throw std::runtime_error{"No item in encodingsModel"};
    auto encId = item->data(Qt::UserRole + 1).toString();

    auto lines = ui->qspbox_numLinesPreview->value();
    auto ret = m_source.isEmpty()
      ?
      CsvFileLoader::previewClipboard(encId, lines)
      :
      CsvFileLoader::previewFile(m_source, encId, lines);

    const auto &prev = ret.first;
    const auto &err = ret.second;

    if (!err.isEmpty()) {
      ui->ql_previewError->setText(err);
      ui->ql_previewError->setVisible(true);
      ui->qpte_preview->setVisible(false);
    } else {
      ui->qpte_preview->setPlainText(prev);
      ui->qpte_preview->setVisible(true);
      ui->ql_previewError->setVisible(false);
    }
  } catch (const std::runtime_error &ex) {
    const QString msg = QString{"Cannot display preview because parameters are invalid:\n%1"}.arg(ex.what());
    ui->ql_previewError->setText(msg);
    ui->ql_previewError->setVisible(true);
  }
}

void LoadCsvFileDialog::setSource(const QString &path)
{
  m_source = path;
}

void LoadCsvFileDialog::updatePreview(const bool show)
{
  ui->ql_previewError->setVisible(false);
  ui->qpte_preview->setVisible(false);

  if (!show) {
    ui->ql_numLinesPreview->setVisible(false);
    ui->qspbox_numLinesPreview->setVisible(false);
    return;
  }

  ui->ql_numLinesPreview->setVisible(true);
  ui->qspbox_numLinesPreview->setVisible(true);
  refreshPreview();
}

} // namespace backend
