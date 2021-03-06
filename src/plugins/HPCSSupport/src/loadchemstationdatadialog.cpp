#include "loadchemstationdatadialog.h"
#include "ui_loadchemstationdatadialog.h"
#include "chemstationbatchloader.h"
#include "chemstationbatchloadmodel.h"

#include <QDir>
#include <QDirIterator>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QHeaderView>

static const QString SINGLE_FILE_HINT{"Loads selected signal traces from the selected .D directory"};
static const QString WHOLE_DIRECTORY_HINT{"Loads selected type of signal trace from all .D directories contained in the selected directory"};
static const QString MULTIPLE_DIRECTORIES_HINT{"Loads selected type of signal trace from all selected .D directories"};

LoadChemStationDataDialog::LoadInfo::LoadInfo(const LoadingMode loadingMode, const QString &path,
                                              const QStringList &dirPaths,
                                              const ChemStationBatchLoader::Filter &filter) :
  loadingMode(loadingMode),
  path(path),
  dirPaths(dirPaths),
  filter(filter)
{
}

LoadChemStationDataDialog::LoadInfo::LoadInfo(const LoadInfo &other) :
  loadingMode(other.loadingMode),
  path(other.path),
  dirPaths(other.dirPaths),
  filter(other.filter)
{
}

LoadChemStationDataDialog::LoadInfo & LoadChemStationDataDialog::LoadInfo::operator=(const LoadInfo &other)
{
  const_cast<LoadingMode&>(loadingMode) = other.loadingMode;
  const_cast<QString&>(path) = other.path;
  const_cast<QStringList&>(dirPaths) = other.dirPaths;
  const_cast<ChemStationBatchLoader::Filter&>(filter) = other.filter;

  return *this;
}

LoadChemStationDataDialog::LoadChemStationDataDialog(UIPlugin *plugin, QFileSystemModel *fsModel, QWidget *parent) :
  QDialog(parent),
  m_uiPlugin(plugin),
  ui(new Ui::LoadChemStationDataDialog),
  m_fsModel(fsModel),
  m_loadInfo(LoadInfo(LoadingMode::SINGLE_FILE, ""))
{
  ui->setupUi(this);

  try {
    m_finfoModel = new ChemStationFileInfoModel(this);
    m_batchLoadModel = new ChemStationBatchLoadModel(this);
    qs_splitter = new QSplitter(Qt::Horizontal, this);
    qtrv_fileSystem = new QTreeView(qs_splitter);
    qtbv_files = new QTableView(qs_splitter);
  } catch (std::bad_alloc&) {
    return;
  }

  m_fsModel->setReadOnly(true);
  m_fsModel->setResolveSymlinks(false);
  m_fsModel->setFilter(QDir::Dirs | QDir::Drives | QDir::NoDotAndDotDot);
#ifdef Q_OS_UNIX
  m_fsModel->setRootPath("/");
#else
  m_fsModel->setRootPath("");
#endif // Q_OS_

  qtrv_fileSystem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  qtbv_files->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  qtrv_fileSystem->setModel(m_fsModel);
  qtbv_files->setModel(m_finfoModel);

  ui->qcbox_loadingMode->addItem(tr("Single .D directory"), QVariant::fromValue(LoadingMode::SINGLE_FILE));
  ui->qcbox_loadingMode->addItem(tr("Multiple .D directories"), QVariant::fromValue(LoadingMode::MULTIPLE_DIRECTORIES));
  ui->qcbox_loadingMode->addItem(tr("All .D directories in parent directory"), QVariant::fromValue(LoadingMode::WHOLE_DIRECTORY));
  m_loadingMode = LoadingMode::SINGLE_FILE;
  ui->ql_loadingModeHint->setText(SINGLE_FILE_HINT);

  /* Hide all additonal information and show only the name
   * of the file in the browsing tree */
  qtrv_fileSystem->hideColumn(1);
  qtrv_fileSystem->hideColumn(2);
  qtrv_fileSystem->hideColumn(3);

  qtbv_files->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

  ui->panesLayout->addWidget(qs_splitter);
  qs_splitter->addWidget(qtrv_fileSystem);
  qs_splitter->addWidget(qtbv_files);

  connect(qtrv_fileSystem, &QTreeView::clicked, this, &LoadChemStationDataDialog::onClicked);
  connect(ui->qpb_cancel, &QPushButton::clicked, this, &LoadChemStationDataDialog::onCancelClicked);
  connect(ui->qpb_load, &QPushButton::clicked, this, &LoadChemStationDataDialog::onLoadClicked);
  connect(qtbv_files, &QTableView::doubleClicked, this, &LoadChemStationDataDialog::onFilesDoubleClicked);
  connect(ui->qcbox_loadingMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &LoadChemStationDataDialog::onLoadingModeActivated);
}

LoadChemStationDataDialog::~LoadChemStationDataDialog()
{
  delete ui;
}

QString LoadChemStationDataDialog::createFileType(const ChemStationFileLoader::Type type)
{
  switch (type) {
  case ChemStationFileLoader::Type::CE_ANALOG:
    return "Analog input";
    break;
  case ChemStationFileLoader::Type::CE_CCD:
    return "Conductivity";
    break;
  case ChemStationFileLoader::Type::CE_CURRENT:
    return "Current";
    break;
  case ChemStationFileLoader::Type::CE_DAD:
    return "Absorbance";
    break;
  case ChemStationFileLoader::Type::CE_POWER:
    return "Power";
    break;
  case ChemStationFileLoader::Type::CE_PRESSURE:
    return "Pressure";
    break;
  case ChemStationFileLoader::Type::CE_TEMPERATURE:
    return "Temperature";
    break;
  case ChemStationFileLoader::Type::CE_VOLTAGE:
    return "Voltage";
    break;
  case ChemStationFileLoader::Type::CE_UNKNOWN:
  default:
    return "Unknown";
    break;
  }
}

QString LoadChemStationDataDialog::createAdditionalInfo(const ChemStationFileLoader::Data &data)
{
  if (data.type == ChemStationFileLoader::Type::CE_DAD)
    return QString("Measured: %1 nm / Reference %2 nm").arg(data.wavelengthMeasured.wavelength).arg(data.wavelengthReference.wavelength);

  return QString();
}

void LoadChemStationDataDialog::expandToPath(const QString &path)
{
  const QModelIndex index = m_fsModel->index(path);

  if (!index.isValid())
    return;

  connect(m_fsModel, &QFileSystemModel::layoutChanged, this, &LoadChemStationDataDialog::onLayoutChanged);

  qtrv_fileSystem->setCurrentIndex(index);
  scrollToIndex(index);
  if (qtrv_fileSystem->visualRect(index).isValid())
    disconnect(m_fsModel, &QFileSystemModel::layoutChanged, this, &LoadChemStationDataDialog::onLayoutChanged);
  onClicked(index);
}

LoadChemStationDataDialog::LoadInfo LoadChemStationDataDialog::loadInfo()
{
  return m_loadInfo;
}

void LoadChemStationDataDialog::loadMultipleDirectories(const QModelIndex &index)
{
  const QModelIndexList &indexes = qtrv_fileSystem->selectionModel()->selectedIndexes();
  const ChemStationBatchLoader::Filter filter = m_batchLoadModel->filter(index);
  QStringList dirPaths;

  if (!filter.isValid)
    return;

  for (const QModelIndex &idx : indexes) {
    const QString path = m_fsModel->filePath(idx);

    if (dirPaths.contains(path))
      continue;

    dirPaths.push_back(m_fsModel->filePath(idx));
  }

  m_loadInfo = LoadInfo(m_loadingMode, "", dirPaths, filter);
  accept();
}

void LoadChemStationDataDialog::loadSingleFile(const QModelIndex &index)
{
  QVariant var;
  bool ok;

  if (!index.isValid())
    return;

  var = m_finfoModel->data(m_finfoModel->index(index.row(), 0), Qt::DisplayRole);

  QString path = processFileName(var, ok);
  if (!ok)
    return;

  m_loadInfo = LoadInfo(LoadingMode::SINGLE_FILE, path);
  accept();
}

void LoadChemStationDataDialog::loadWholeDirectory(const QModelIndex &index)
{
  const ChemStationBatchLoader::Filter filter = m_batchLoadModel->filter(index);
  const QModelIndex idx = qtrv_fileSystem->selectionModel()->currentIndex();

  if (!filter.isValid)
    return;

  if (!idx.isValid())
    return;

  const QString path = m_fsModel->filePath(idx);

  m_loadInfo = LoadInfo(m_loadingMode, path, QStringList(), filter);
  accept();
}

void LoadChemStationDataDialog::multipleDirectoriesSelected()
{
  const QModelIndexList &indexes = qtrv_fileSystem->selectionModel()->selectedIndexes();
  QStringList dirPaths;

  for (const QModelIndex &index : indexes) {
    const QString path = m_fsModel->filePath(index);

    if (dirPaths.contains(path))
      continue;

    dirPaths.push_back(m_fsModel->filePath(index));
  }

  ChemStationBatchLoader::CHSDataVec common = ChemStationBatchLoader::getCommonTypes(m_uiPlugin, dirPaths);

  setBatchLoadModel(common);
}

void LoadChemStationDataDialog::onCancelClicked(const bool clicked)
{
  Q_UNUSED(clicked);

  reject();
}

void LoadChemStationDataDialog::onClicked(const QModelIndex &index)
{
  if (!index.isValid()) {
    m_finfoModel->clear();
    return;
  }

  switch (m_loadingMode) {
  case LoadingMode::SINGLE_FILE:
    singleSelected(index);
    break;
  case LoadingMode::WHOLE_DIRECTORY:
    wholeDirectorySelected(index);
    break;
  case LoadingMode::MULTIPLE_DIRECTORIES:
    multipleDirectoriesSelected();
    break;
  default:
    break;
  }
}

void LoadChemStationDataDialog::onFilesDoubleClicked(const QModelIndex &index)
{
  if (!index.isValid())
    return;

  switch (m_loadingMode) {
  case LoadingMode::SINGLE_FILE:
    loadSingleFile(index);
    break;
  case LoadingMode::WHOLE_DIRECTORY:
    loadWholeDirectory(index);
    break;
  case LoadingMode::MULTIPLE_DIRECTORIES:
    loadMultipleDirectories(index);
    break;
  }
}

void LoadChemStationDataDialog::onLayoutChanged()
{
  scrollToIndex(qtrv_fileSystem->currentIndex());
  disconnect(m_fsModel, &QFileSystemModel::layoutChanged, this, &LoadChemStationDataDialog::onLayoutChanged);
}

void LoadChemStationDataDialog::onLoadClicked(const bool clicked)
{
  Q_UNUSED(clicked);
  QModelIndex idx;

  idx = m_finfoModel->index(qtbv_files->currentIndex().row(), 0);

  onFilesDoubleClicked(idx);
}

void LoadChemStationDataDialog::onLoadingModeActivated()
{
  QVariant v = ui->qcbox_loadingMode->currentData(Qt::UserRole);

  if (!v.canConvert<LoadingMode>())
    return;

  m_loadingMode = v.value<LoadingMode>();

  qtrv_fileSystem->clearSelection();

  switch (m_loadingMode) {
  case LoadingMode::SINGLE_FILE:
    qtrv_fileSystem->setSelectionMode(QAbstractItemView::SingleSelection);
    m_finfoModel->clear();
    qtbv_files->setModel(m_finfoModel);
    ui->ql_loadingModeHint->setText(SINGLE_FILE_HINT);
    break;
  case LoadingMode::WHOLE_DIRECTORY:
    qtrv_fileSystem->setSelectionMode(QAbstractItemView::SingleSelection);
    m_batchLoadModel->clear();
    qtbv_files->setModel(m_batchLoadModel);
    ui->ql_loadingModeHint->setText(WHOLE_DIRECTORY_HINT);
    break;
  case LoadingMode::MULTIPLE_DIRECTORIES:
    qtrv_fileSystem->setSelectionMode(QAbstractItemView::MultiSelection);
    m_batchLoadModel->clear();
    qtbv_files->setModel(m_batchLoadModel);
    ui->ql_loadingModeHint->setText(MULTIPLE_DIRECTORIES_HINT);
    break;
  }
}

QString LoadChemStationDataDialog::processFileName(const QVariant &fileNameVariant, bool &ok)
{
  QDir dir(m_currentDirPath);

  if (!fileNameVariant.isValid()) {
    ok = false;
    return "";
  }

  ok = true;
  return dir.absoluteFilePath(fileNameVariant.toString());
}

void LoadChemStationDataDialog::scrollToIndex(const QModelIndex &index)
{
  if (!index.isValid())
    return;

  const bool anim = qtrv_fileSystem->isAnimated();

  qtrv_fileSystem->setAnimated(false);
  qtrv_fileSystem->scrollTo(qtrv_fileSystem->currentIndex());
  qtrv_fileSystem->setAnimated(anim);
}

void LoadChemStationDataDialog::setBatchLoadModel(const ChemStationBatchLoader::CHSDataVec &common)
{
  QVector<ChemStationBatchLoadModel::Entry> entries;

  for (const ChemStationFileLoader::Data &chData : common) {
    ChemStationBatchLoader::Filter filter(chData.type, chData.wavelengthMeasured.wavelength, chData.wavelengthReference.wavelength);

    entries.push_back(ChemStationBatchLoadModel::Entry(createFileType(chData.type), createAdditionalInfo(chData), filter));
  }

  m_batchLoadModel->setNewData(entries);
  m_batchLoadModel->sort(0, Qt::AscendingOrder);
}

void LoadChemStationDataDialog::singleSelected(const QModelIndex &index)
{
  QVector<ChemStationFileInfoModel::Entry> entries;

  m_currentDirPath = m_fsModel->filePath(index);
  QDir dir(m_currentDirPath);

  if (!dir.exists())
    return;

  QDirIterator dirIt(m_currentDirPath, QDir::Files | QDir::NoSymLinks, QDirIterator::NoIteratorFlags);

  while (dirIt.hasNext()) {
    QString fileName = dirIt.next();
    QString absoluteFilePath = dir.absoluteFilePath(fileName);
    QString name = QFileInfo(absoluteFilePath).fileName();

    ChemStationFileLoader::Data chData = ChemStationFileLoader::loadHeader(m_uiPlugin, absoluteFilePath);
    if (!chData.isValid())
      entries.push_back(ChemStationFileInfoModel::Entry(name, "", "", false));
    else
      entries.push_back(ChemStationFileInfoModel::Entry(name, createFileType(chData.type), createAdditionalInfo(chData), true));
  }

  m_finfoModel->setNewData(entries);
  m_finfoModel->sort(0, Qt::AscendingOrder);
}

void LoadChemStationDataDialog::wholeDirectorySelected(const QModelIndex &index)
{
  ChemStationBatchLoader::CHSDataVec common = ChemStationBatchLoader::getCommonTypes(m_uiPlugin, m_fsModel->filePath(index));

  setBatchLoadModel(common);
}
