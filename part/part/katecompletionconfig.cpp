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

#include <QTreeWidgetItem>

#include <ktexteditor/codecompletion2.h>

#include "katecompletionmodel.h"

#include "ui_completionconfigwidget.h"

KateCompletionConfig::KateCompletionConfig(KateCompletionModel* model, QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::CompletionConfigWidget())
  , m_model(model)
{
  ui->setupUi(this);

  ui->sorting->setChecked(m_model->isSortingEnabled());
  ui->filtering->setChecked(m_model->isFilteringEnabled());
  ui->grouping->setChecked(m_model->isGroupingEnabled());
  ui->columnMerging->setChecked(m_model->isColumnMergingEnabled());
  connect(ui->columnUp, SIGNAL(pressed()), SLOT(moveColumnUp()));
  connect(ui->columnDown, SIGNAL(pressed()), SLOT(moveColumnDown()));

  QList<int> mergedColumns;
  if (!m_model->columnMerges().isEmpty()) {
    foreach (const QList<int>& list, m_model->columnMerges()) {
      bool first = true;
      foreach (int column, list) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->columnMergeTree, column);
        item->setText(0, KateCompletionModel::columnName(column));
        item->setCheckState(1, first ? Qt::Unchecked : Qt::Checked);
        item->setCheckState(2, Qt::Checked);
        first = false;
        mergedColumns << column;
      }
    }

    for (int i = 0; i < KTextEditor::CodeCompletionModel::ColumnCount; ++i)
      if (!mergedColumns.contains(i)) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->columnMergeTree, i);
        item->setText(0, KateCompletionModel::columnName(i));
        item->setCheckState(1, Qt::Unchecked);
        item->setCheckState(2, Qt::Unchecked);
      }

  } else {
    for (int i = 0; i < KTextEditor::CodeCompletionModel::ColumnCount; ++i) {
      QTreeWidgetItem* item = new QTreeWidgetItem(ui->columnMergeTree, i);
      item->setText(0, KateCompletionModel::columnName(i));
      item->setCheckState(1, Qt::Unchecked);
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
  m_model->setSortingEnabled(ui->sorting->isChecked());
  m_model->setFilteringEnabled(ui->filtering->isChecked());
  m_model->setGroupingEnabled(ui->grouping->isChecked());
  m_model->setColumnMergingEnabled(ui->columnMerging->isChecked());

  QList< QList<int> > mergedColumns;
  QList<int> oneMerge;
  for (int i = 0; i < ui->columnMergeTree->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = ui->columnMergeTree->topLevelItem(i);
    if (item->checkState(2) == Qt::Unchecked)
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

#include "katecompletionconfig.moc"
