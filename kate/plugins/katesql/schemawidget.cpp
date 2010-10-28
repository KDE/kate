/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

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

#include "schemawidget.h"

#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>
#include <kapplication.h>

#include <qvariant.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qsqldatabase.h>
#include <qsqlrecord.h>
#include <qsqlindex.h>
#include <qsqlfield.h>
#include <qmenu.h>

SchemaWidget::SchemaWidget(QWidget *parent)
: QTreeWidget(parent)
{
  m_tablesLoaded = false;
  m_viewsLoaded = false;

  setHeaderLabels(QStringList() << i18nc("@title:column", "Database schema"));

  setContextMenuPolicy(Qt::CustomContextMenu);
  setDragDropMode(QAbstractItemView::DragOnly);
  setDragEnabled(true);
  setAcceptDrops(false);
//   setDropIndicatorShown(true);

  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotCustomContextMenuRequested(const QPoint&)));
  connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(slotItemExpanded(QTreeWidgetItem*)));
}


SchemaWidget::~SchemaWidget()
{
}


void SchemaWidget::deleteChildren(QTreeWidgetItem *item)
{
  QList<QTreeWidgetItem *> items = item->takeChildren();

  foreach (QTreeWidgetItem *i, items)
    delete i;
}


void SchemaWidget::buildTree(const QString &connection)
{
  m_connectionName = connection;

  clear();

  m_tablesLoaded = false;
  m_viewsLoaded = false;

  QSqlDatabase db = QSqlDatabase::database(m_connectionName);

  if (!db.isValid())
    return;

  buildDatabase(new QTreeWidgetItem(this));
}


void SchemaWidget::refresh()
{
  if (!m_connectionName.isEmpty())
    buildTree(m_connectionName);
}


void SchemaWidget::buildDatabase(QTreeWidgetItem * databaseItem)
{
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);

  databaseItem->setText(0, db.connectionName());
  databaseItem->setIcon(0, KIcon("server-database"));

  QTreeWidgetItem *tablesItem = new QTreeWidgetItem(databaseItem, TablesFolderType);
  tablesItem->setText(0, i18nc("@title Folder name", "Tables"));
  tablesItem->setIcon(0, KIcon("folder"));
  tablesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

//   buildTables(tablesItem, db);

  QTreeWidgetItem *viewsItem = new QTreeWidgetItem(databaseItem, ViewsFolderType);
  viewsItem->setText(0, i18nc("@title Folder name", "Views"));
  viewsItem->setIcon(0, KIcon("folder"));
  viewsItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

//   buildViews(viewsItem, db);

  databaseItem->setExpanded(true);
//   tablesItem->setExpanded(true);
//   viewsItem->setExpanded(true);
}


void SchemaWidget::buildTables(QTreeWidgetItem * tablesItem)
{
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);

  QTreeWidgetItem *systemTablesItem = new QTreeWidgetItem(tablesItem, SystemTablesFolderType);
  systemTablesItem->setText(0, i18nc("@title Folder name", "System Tables"));
  systemTablesItem->setIcon(0, KIcon("folder"));
  systemTablesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

  if (!db.isValid() || !db.isOpen() )
    return;

  QStringList tables = db.tables(QSql::SystemTables);

  foreach(const QString& table, tables)
  {
    QTreeWidgetItem *item = new QTreeWidgetItem(systemTablesItem, SystemTableType);
    item->setText(0, table);
    item->setIcon(0, KIcon("sql-table"));
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
  }

  tables = db.tables(QSql::Tables);

  foreach(const QString& table, tables)
  {
    QTreeWidgetItem *item = new QTreeWidgetItem(tablesItem, TableType);
    item->setText(0, table);
    item->setIcon(0, KIcon("sql-table"));
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
  }

  m_tablesLoaded = true;
}


void SchemaWidget::buildViews(QTreeWidgetItem * viewsItem)
{
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);

  if (!db.isValid() || !db.isOpen() )
    return;

  const QStringList views = db.tables(QSql::Views);

  foreach(const QString& view, views)
  {
    QTreeWidgetItem *item = new QTreeWidgetItem(viewsItem, ViewType);
    item->setText(0, view);
    item->setIcon(0, KIcon("sql-view"));
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
  }

  m_viewsLoaded = true;
}


void SchemaWidget::buildFields(QTreeWidgetItem * tableItem)
{
  QSqlDatabase db = QSqlDatabase::database(m_connectionName);

  if (!db.isValid() || !db.isOpen() )
    return;

  QString tableName = tableItem->text(0);

  QSqlIndex pk = db.primaryIndex(tableName);
  QSqlRecord rec = db.record(tableName);

  for (int i = 0; i < rec.count(); ++i)
  {
    QSqlField f = rec.field(i);

    QString fieldName = f.name();

    QTreeWidgetItem *item = new QTreeWidgetItem(tableItem, FieldType);
    item->setText(0, fieldName);

    if (pk.contains(fieldName))
      item->setIcon(0, KIcon("sql-field-pk"));
    else
      item->setIcon(0, KIcon("sql-field"));
  }
}


void SchemaWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
    m_dragStartPosition = event->pos();
  QTreeWidget::mousePressEvent(event);
}

void SchemaWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (!(event->buttons() & Qt::LeftButton))
    return;
  if ((event->pos() - m_dragStartPosition).manhattanLength()
    < kapp->startDragDistance())
    return;

//   QTreeWidgetItem *item = currentItem();
  QTreeWidgetItem *item = itemAt(event->pos());

  if (!item)
    return;

  if (item->type() != SchemaWidget::SystemTableType &&
      item->type() != SchemaWidget::TableType &&
      item->type() != SchemaWidget::ViewType &&
      item->type() != SchemaWidget::FieldType)
    return;

  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData;

  if (item->type() == SchemaWidget::FieldType)
    mimeData->setText(QString("%1.%2").arg(item->parent()->text(0)).arg(item->text(0)));
  else
    mimeData->setText(item->text(0));

  drag->setMimeData(mimeData);
  drag->exec(Qt::CopyAction);

  QTreeWidget::mouseMoveEvent(event);
}


void SchemaWidget::slotItemExpanded(QTreeWidgetItem *item)
{
  if (!item)
    return;

  switch(item->type())
  {
    case SchemaWidget::TablesFolderType:
    {
      if (!m_tablesLoaded)
        buildTables(item);
    }
    break;

    case SchemaWidget::ViewsFolderType:
    {
      if (!m_viewsLoaded)
        buildViews(item);
    }
    break;

    case SchemaWidget::TableType:
    case SchemaWidget::SystemTableType:
    case SchemaWidget::ViewType:
    {
      if (item->childCount() == 0)
        buildFields(item);
    }
    break;

    default:
    break;
  }
}


void SchemaWidget::slotCustomContextMenuRequested(const QPoint &pos)
{
  Q_UNUSED(pos);

  QMenu menu;

  menu.addAction(KIcon("view-refresh"), i18nc("@action:inmenu Context menu", "Refresh"), this, SLOT(refresh()));

  menu.exec(QCursor::pos());

  /// TODO: context menu?
/*
  QTreeWidgetItem *item = itemAt(pos);

  if (!item)
    return;

  if (item->type() != SchemaWidget::SystemTableType &&
    item->type() != SchemaWidget::TableType &&
    item->type() != SchemaWidget::ViewType)
    return;

  QMenu menu;

  menu.addAction(item->text(0));
  menu.addAction("Pippo");
  menu.addAction("Pluto");

  menu.exec(QCursor::pos());
*/
}
