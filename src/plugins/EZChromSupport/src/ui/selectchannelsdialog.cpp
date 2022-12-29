#include "selectchannelsdialog.h"
#include "ui_selectchannelsdialog.h"

#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGridLayout>

SelectChannelsDialog::SelectChannelsDialog(const std::vector<std::tuple<std::string, std::string>> &channels, QWidget *parent) :
  QDialog{parent},
  ui{new Ui::SelectChannelsDialog}
{
  ui->setupUi(this);

  QGridLayout *lay = static_cast<QGridLayout *>(ui->gridLayout);

  lay->addWidget(new QLabel{"Channel name", this}, 0, 0);
  lay->addWidget(new QLabel{"Unit", this}, 0, 1);
  lay->addWidget(new QLabel{"Selected",  this}, 0, 2);

  int ctr = 0;
  for (const auto &chan : channels) {
    const auto &name = std::get<0>(chan);
    const auto &unit = std::get<1>(chan);

    auto *l = new QLabel{QString::fromUtf8(name.data()), this};
    auto *u = new QLabel{QString::fromUtf8(unit.data()), this};

    auto *cb = new QCheckBox{this};
    m_selected.emplace_back(name, cb);

    cb->setChecked(true);
    cb->setText(tr("Selected"));

    lay->addWidget(l, ctr + 1, 0);
    lay->addWidget(u, ctr + 1, 1);
    lay->addWidget(cb, ctr + 1, 2);

    ctr++;
  }

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SelectChannelsDialog::finish);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SelectChannelsDialog::finish);
  connect(ui->qpb_deselectAll, &QPushButton::clicked, this, &SelectChannelsDialog::onDeselectAll);
  connect(ui->qpb_selectAll, &QPushButton::clicked, this, &SelectChannelsDialog::onSelectAll);
}

SelectChannelsDialog::~SelectChannelsDialog()
{
  delete ui;
}

std::vector<std::pair<std::string, bool>> SelectChannelsDialog::selection() const
{
  std::vector<std::pair<std::string, bool>> sel{};

  for (const auto &item : m_selected)
    sel.emplace_back(item.first, item.second->isChecked());

  return sel;
}

void SelectChannelsDialog::finish()
{
  accept();
}

void SelectChannelsDialog::onDeselectAll()
{
  for (auto &item : m_selected)
    item.second->setChecked(false);
}

void SelectChannelsDialog::onSelectAll()
{
  for (auto &item : m_selected)
    item.second->setChecked(true);
}
