/* This file is part of the KDE libraries
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katecompletionconfig.h"

#include <QtGui/QTreeWidget>

#include <kicon.h>
#include <kglobal.h>

#include "katecompletionmodel.h"

#include "ui_completionconfigwidget.h"

using namespace KTextEditor;

KateCompletionConfig::KateCompletionConfig(KateCompletionModel* model, QWidget* parent)
  : KDialog(parent)
  , ui(new Ui::CompletionConfigWidget())
  , m_model(model)
{
  //setAttribute(Qt::WA_DestructiveClose);
  setCaption(i18n("Code Completion Configuration"));
  setButtons(KDialog::Ok | KDialog::Cancel);
  setDefaultButton(KDialog::Ok);
  connect(this, SIGNAL(okClicked()), SLOT(apply()));

  QWidget* mw = new QWidget(this);
  ui->setupUi(mw);
  setMainWidget(mw);

  // Sorting
  ui->sorting->setChecked(m_model->isSortingEnabled());
  ui->sortingAlphabetical->setChecked(m_model->isSortingAlphabetical());
  ui->sortingCaseSensitive->setChecked(m_model->sortingCaseSensitivity() == Qt::CaseSensitive);
  ui->groupingOrderUp->setIcon(KIcon("go-up"));
  ui->groupingOrderDown->setIcon(KIcon("go-down"));
  connect(ui->groupingOrderUp, SIGNAL(pressed()), SLOT(moveGroupingOrderUp()));
  connect(ui->groupingOrderDown, SIGNAL(pressed()), SLOT(moveGroupingOrderDown()));

  // Filtering
  ui->filtering->setChecked(m_model->isFilteringEnabled());
  ui->filteringContextMatchOnly->setChecked(m_model->filterContextMatchesOnly());
  ui->filteringHideAttributes->setChecked(m_model->filterByAttribute());

  for (CodeCompletionModel::CompletionProperty i = CodeCompletionModel::FirstProperty; i <= CodeCompletionModel::LastProperty; i = static_cast<CodeCompletionModel::CompletionProperty>(i << 1)) {
    QListWidgetItem* item = new QListWidgetItem(m_model->propertyName(i), ui->filteringAttributesList, i);
    item->setCheckState((m_model->filterAttributes() & i) ? Qt::Checked : Qt::Unchecked);
  }

  ui->filteringMaximumInheritanceDepth->setValue(m_model->maximumInheritanceDepth());

  // Grouping
  ui->grouping->setChecked(m_model->isGroupingEnabled());
  ui->groupingUp->setIcon(KIcon("go-up"));
  ui->groupingDown->setIcon(KIcon("go-down"));

  m_groupingScopeType = ui->groupingMethods->topLevelItem(0);
  m_groupingScopeType->setCheckState(0, (m_model->groupingMethod() & KateCompletionModel::ScopeType) ? Qt::Checked : Qt::Unchecked);

  m_groupingScope = ui->groupingMethods->topLevelItem(1);
  m_groupingScope->setCheckState(0, (m_model->groupingMethod() & KateCompletionModel::Scope) ? Qt::Checked : Qt::Unchecked);

  m_groupingAccessType = ui->groupingMethods->topLevelItem(2);
  m_groupingAccessType->setCheckState(0, (m_model->groupingMethod() & KateCompletionModel::AccessType) ? Qt::Checked : Qt::Unchecked);

  m_groupingItemType = ui->groupingMethods->topLevelItem(3);
  m_groupingItemType->setCheckState(0, (m_model->groupingMethod() & KateCompletionModel::ItemType) ? Qt::Checked : Qt::Unchecked);

  ui->accessConst->setChecked(m_model->accessIncludeConst());
  ui->accessStatic->setChecked(m_model->accessIncludeStatic());
  ui->accessSignalSlot->setChecked(m_model->accessIncludeSignalSlot());

  for (int i = 0; i < 4; ++i)
    ui->groupingMethods->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
  connect(ui->groupingUp, SIGNAL(pressed()), SLOT(moveGroupingUp()));
  connect(ui->groupingDown, SIGNAL(pressed()), SLOT(moveGroupingDown()));

  // Column merging
  ui->columnMerging->setChecked(m_model->isColumnMergingEnabled());
  ui->columnUp->setIcon(KIcon("go-up"));
  ui->columnDown->setIcon(KIcon("go-down"));
  connect(ui->columnUp, SIGNAL(pressed()), SLOT(moveColumnUp()));
  connect(ui->columnDown, SIGNAL(pressed()), SLOT(moveColumnDown()));


  QList<int> mergedColumns;
  if (!m_model->columnMerges().isEmpty()) {
    foreach (const QList<int>& list, m_model->columnMerges()) {
      bool first = true;
      foreach (int column, list) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->columnMergeTree, column);
        item->setText(0, KateCompletionModel::columnName(column) + QString(" %1").arg(column));
        item->setCheckState(1, first ? Qt::Unchecked : Qt::Checked);

        if (column == KTextEditor::CodeCompletionModel::Name)
          item->setText(2, i18n("Always"));
        else
          item->setCheckState(2, Qt::Checked);

        first = false;
        mergedColumns << column;
      }
    }

    for (int column = 0; column < KTextEditor::CodeCompletionModel::ColumnCount; ++column) {
      if (!mergedColumns.contains(column)) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->columnMergeTree, column);
        item->setText(0, KateCompletionModel::columnName(column) + QString(" %1").arg(column));
        item->setCheckState(1, Qt::Unchecked);

        Q_ASSERT(column != KTextEditor::CodeCompletionModel::Name);

        item->setCheckState(2, Qt::Unchecked);
      }
    }

  } else {
    for (int column = 0; column < KTextEditor::CodeCompletionModel::ColumnCount; ++column) {
      QTreeWidgetItem* item = new QTreeWidgetItem(ui->columnMergeTree, column);
      item->setText(0, KateCompletionModel::columnName(column) + QString(" %1").arg(column));
      item->setCheckState(1, Qt::Unchecked);

      if (column == KTextEditor::CodeCompletionModel::Name)
        item->setText(2, i18n("Always"));
      else
        item->setCheckState(2, Qt::Checked);
    }
  }

  // init with defaults from config or really hardcoded ones
  KConfigGroup config( KGlobal::config(), "Kate Code Completion Defaults");
  readConfig (config);
}

KateCompletionConfig::~ KateCompletionConfig( )
{
  delete ui;
}

void KateCompletionConfig::readConfig(const KConfigGroup &config)
{
  configStart ();

  // Sorting
  ui->sorting->setChecked(config.readEntry("Sorting Enabled", true));
  ui->sortingAlphabetical->setChecked(config.readEntry("Sort Alphabetically", true));
  ui->sortingCaseSensitive->setChecked(config.readEntry("Case Sensitive Sort", false));
  ui->sortingInheritanceDepth->setChecked(config.readEntry("Sort by Inheritance Depth", true));

  // Filtering
  ui->filtering->setChecked(config.readEntry("Filtering Enabled", false));
  ui->filteringContextMatchOnly->setChecked(config.readEntry("Filter by Context Match Only", false));
  ui->filteringHideAttributes->setChecked(config.readEntry("Hide Completions by Attribute", false));

  int attributes = config.readEntry("Filter Attribute Mask", 0);
  for (int i = 0; i < ui->filteringAttributesList->count(); ++i) {
    QListWidgetItem* item = ui->filteringAttributesList->item(i);
    item->setCheckState(((1 << (i - 1)) & attributes) ? Qt::Checked : Qt::Unchecked);
  }

  ui->filteringMaximumInheritanceDepth->setValue(config.readEntry("Filter by Maximum Inheritance Depth", 0));

  // Grouping
  ui->grouping->setChecked(config.readEntry("Grouping Enabled", true));

  m_groupingScopeType->setCheckState(0, config.readEntry("Group by Scope Type", true) ? Qt::Checked : Qt::Unchecked);
  m_groupingScope->setCheckState(0, config.readEntry("Group by Scope", false) ? Qt::Checked : Qt::Unchecked);
  m_groupingAccessType->setCheckState(0, config.readEntry("Group by Access Type", true) ? Qt::Checked : Qt::Unchecked);
  m_groupingItemType->setCheckState(0, config.readEntry("Group by Item Type", false) ? Qt::Checked : Qt::Unchecked);

  ui->accessConst->setChecked(config.readEntry("Group by Const", false));
  ui->accessStatic->setChecked(config.readEntry("Group by Static", false));
  ui->accessSignalSlot->setChecked(config.readEntry("Group by Signals and Slots", false));

  // Column merging
  ui->columnMerging->setChecked(config.readEntry("Column Merging Enabled", true));

  for (int i = 0; i < ui->columnMergeTree->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = ui->columnMergeTree->topLevelItem(i);
    ///Initialize a standard column-merging: Merge Scope, Name, Arguments and Postfix
    item->setCheckState(1, config.readEntry(QString("Column %1 Merge").arg(i), (i == CodeCompletionModel::Scope || i == CodeCompletionModel::Name || i == CodeCompletionModel::Arguments)) ? Qt::Checked : Qt::Unchecked);
    item->setCheckState(2, config.readEntry(QString("Column %1 Show").arg(i), true) ? Qt::Checked : Qt::Unchecked);
  }

  applyInternal();

  configEnd();
}

void KateCompletionConfig::writeConfig(KConfigGroup &config)
{
  // Sorting
  config.writeEntry("Sorting Enabled", ui->sorting->isChecked());
  config.writeEntry("Sort Alphabetically", ui->sortingAlphabetical->isChecked());
  config.writeEntry("Case Sensitive Sort", ui->sortingCaseSensitive->isChecked());
  config.writeEntry("Sort by Inheritance Depth", ui->sortingInheritanceDepth->isChecked());

  // Filtering
  config.writeEntry("Filtering Enabled", ui->filtering->isChecked());
  config.writeEntry("Filter by Context Match Only", ui->filteringContextMatchOnly->isChecked());
  config.writeEntry("Hide Completions by Attribute", ui->filteringHideAttributes->isChecked());

  int attributes = 0;
  for (int i = 0; i < ui->filteringAttributesList->count(); ++i) {
    QListWidgetItem* item = ui->filteringAttributesList->item(i);
    if (item->checkState() == Qt::Checked)
      attributes |= 1 << (i - 1);
  }
  config.writeEntry("Filter Attribute Mask", attributes);

  config.writeEntry("Filter by Maximum Inheritance Depth", ui->filteringMaximumInheritanceDepth->value());

  // Grouping
  config.writeEntry("Grouping Enabled", ui->grouping->isChecked());

  config.writeEntry("Group by Scope Type", m_groupingScopeType->checkState(0) == Qt::Checked ? true : false);
  config.writeEntry("Group by Scope", m_groupingScope->checkState(0) == Qt::Checked ? true : false);
  config.writeEntry("Group by Access Type", m_groupingAccessType->checkState(0) == Qt::Checked ? true : false);
  config.writeEntry("Group by Item Type", m_groupingItemType->checkState(0) == Qt::Checked ? true : false);

  config.writeEntry("Group by Const", ui->accessConst->isChecked());
  config.writeEntry("Group by Static", ui->accessStatic->isChecked());
  config.writeEntry("Group by Signals and Slots", ui->accessSignalSlot->isChecked());

  // Column merging
  config.writeEntry("Column Merging Enabled", ui->columnMerging->isChecked());

  for (int i = 0; i < ui->columnMergeTree->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = ui->columnMergeTree->topLevelItem(i);
    config.writeEntry(QString("Column %1 Merge").arg(i), item->checkState(1) == Qt::Checked ? true : false);
    config.writeEntry(QString("Column %1 Show").arg(i), item->checkState(2) == Qt::Checked ? true : false);
  }

  config.sync();
}

void KateCompletionConfig::updateConfig()
{
  // Ah, nothing to do, I think...?
}

void KateCompletionConfig::moveColumnUp( )
{
  QTreeWidgetItem* item = ui->columnMergeTree->currentItem();
  if (item) {
    int index = ui->columnMergeTree->indexOfTopLevelItem(item);
    if (index > 0) {
      ui->columnMergeTree->takeTopLevelItem(index);
      ui->columnMergeTree->insertTopLevelItem(index - 1, item);
      ui->columnMergeTree->setCurrentItem(item);
    }
  }
}

void KateCompletionConfig::moveColumnDown( )
{
  QTreeWidgetItem* item = ui->columnMergeTree->currentItem();
  if (item) {
    int index = ui->columnMergeTree->indexOfTopLevelItem(item);
    if (index < ui->columnMergeTree->topLevelItemCount() - 1) {
      ui->columnMergeTree->takeTopLevelItem(index);
      ui->columnMergeTree->insertTopLevelItem(index + 1, item);
      ui->columnMergeTree->setCurrentItem(item);
    }
  }
}

void KateCompletionConfig::apply( )
{
  applyInternal();

  KConfigGroup config( KGlobal::config(), "Kate Code Completion Defaults");
  writeConfig (config);
}

void KateCompletionConfig::applyInternal()
{
  // Sorting
  m_model->setSortingEnabled(ui->sorting->isChecked());
  m_model->setSortingAlphabetical(ui->sortingAlphabetical->isChecked());
  m_model->setSortingCaseSensitivity(ui->sortingCaseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
  m_model->setSortingByInheritanceDepth(ui->sortingInheritanceDepth->isChecked());

  // Filtering
  m_model->setFilteringEnabled(ui->filtering->isChecked());

  m_model->setFilterContextMatchesOnly(ui->filteringContextMatchOnly->isChecked());
  m_model->setFilterByAttribute(ui->filteringHideAttributes->isChecked());

  CodeCompletionModel::CompletionProperties attributes = 0;
  for (int i = 0; i < ui->filteringAttributesList->count(); ++i) {
    QListWidgetItem* item = ui->filteringAttributesList->item(i);
    if (item->checkState() == Qt::Checked)
      attributes |= static_cast<CodeCompletionModel::CompletionProperty>(item->type());
  }
  m_model->setFilterAttributes(attributes);

  m_model->setMaximumInheritanceDepth(ui->filteringMaximumInheritanceDepth->value());

  // Grouping
  m_model->setGroupingEnabled(ui->grouping->isChecked());

  KateCompletionModel::GroupingMethods groupingMethod = 0;
  if (m_groupingScopeType->checkState(0) == Qt::Checked)
    groupingMethod = KateCompletionModel::ScopeType;
  if (m_groupingScope->checkState(0) == Qt::Checked)
    groupingMethod |= KateCompletionModel::Scope;
  if (m_groupingAccessType->checkState(0) == Qt::Checked)
    groupingMethod |= KateCompletionModel::AccessType;
  if (m_groupingItemType->checkState(0) == Qt::Checked)
    groupingMethod |= KateCompletionModel::ItemType;
  m_model->setGroupingMethod(groupingMethod);

  m_model->setAccessIncludeConst(ui->accessConst->isChecked());
  m_model->setAccessIncludeStatic(ui->accessStatic->isChecked());
  m_model->setAccessIncludeSignalSlot(ui->accessSignalSlot->isChecked());

  // Column merging
  m_model->setColumnMergingEnabled(ui->columnMerging->isChecked());

  QList< QList<int> > mergedColumns;
  QList<int> oneMerge;
  for (int i = 0; i < ui->columnMergeTree->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = ui->columnMergeTree->topLevelItem(i);

    if (item->type() != KTextEditor::CodeCompletionModel::Name && item->checkState(2) == Qt::Unchecked)
      continue;

    if (item->checkState(1) == Qt::Unchecked) {
      if (oneMerge.count())
        mergedColumns.append(oneMerge);
      oneMerge.clear();
    }

    oneMerge.append(item->type());
  }

  if (oneMerge.count())
    mergedColumns.append(oneMerge);

  m_model->setColumnMerges(mergedColumns);
}

void KateCompletionConfig::moveGroupingUp( )
{
  QTreeWidgetItem* item = ui->groupingMethods->currentItem();
  if (item) {
    int index = ui->groupingMethods->indexOfTopLevelItem(item);
    if (index > 0) {
      ui->groupingMethods->takeTopLevelItem(index);
      ui->groupingMethods->insertTopLevelItem(index - 1, item);
      ui->groupingMethods->setCurrentItem(item);
    }
  }
}

void KateCompletionConfig::moveGroupingDown( )
{
  QTreeWidgetItem* item = ui->groupingMethods->currentItem();
  if (item) {
    int index = ui->groupingMethods->indexOfTopLevelItem(item);
    if (index < ui->groupingMethods->topLevelItemCount() - 1) {
      ui->groupingMethods->takeTopLevelItem(index);
      ui->groupingMethods->insertTopLevelItem(index + 1, item);
      ui->groupingMethods->setCurrentItem(item);
    }
  }
}

void KateCompletionConfig::moveGroupingOrderUp( )
{
  QListWidgetItem* item = ui->sortGroupingOrder->currentItem();
  int index = ui->sortGroupingOrder->currentRow();
  if (index > 0) {
    ui->sortGroupingOrder->takeItem(index);
    ui->sortGroupingOrder->insertItem(index - 1, item);
    ui->sortGroupingOrder->setCurrentItem(item);
  }
}

void KateCompletionConfig::moveGroupingOrderDown( )
{
  QListWidgetItem* item = ui->sortGroupingOrder->currentItem();
  int index = ui->sortGroupingOrder->currentRow();
  if (index < ui->sortGroupingOrder->count() - 1) {
    ui->sortGroupingOrder->takeItem(index);
    ui->sortGroupingOrder->insertItem(index + 1, item);
    ui->sortGroupingOrder->setCurrentItem(item);
  }
}

#include "katecompletionconfig.moc"
