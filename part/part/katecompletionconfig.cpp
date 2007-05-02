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
  ui->sortingReverse->setChecked(m_model->isSortingReverse());
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
}

KateCompletionConfig::~ KateCompletionConfig( )
{
  delete ui;
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
  // Sorting
  m_model->setSortingEnabled(ui->sorting->isChecked());
  m_model->setSortingAlphabetical(ui->sortingAlphabetical->isChecked());
  m_model->setSortingReverse(ui->sortingReverse->isChecked());
  m_model->setSortingCaseSensitivity(ui->sortingCaseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

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
