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

#ifndef SCHEMAWIDGET_H
#define SCHEMAWIDGET_H

class SQLManager;
class QMouseEvent;

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <qstring.h>
#include <qsqldriver.h>

class SchemaWidget : public QTreeWidget
{
  Q_OBJECT

  public:
    static const int TableType        = QTreeWidgetItem::UserType + 1;
    static const int SystemTableType  = QTreeWidgetItem::UserType + 2;
    static const int ViewType         = QTreeWidgetItem::UserType + 3;
    static const int FieldType        = QTreeWidgetItem::UserType + 4;
    static const int TablesFolderType = QTreeWidgetItem::UserType + 101;
    static const int SystemTablesFolderType = QTreeWidgetItem::UserType + 102;
    static const int ViewsFolderType  = QTreeWidgetItem::UserType + 103;

    SchemaWidget(QWidget *parent, SQLManager *manager);
    ~SchemaWidget() override;

    void buildDatabase(QTreeWidgetItem * databaseItem);
    void buildTables(QTreeWidgetItem * tablesItem);
    void buildViews(QTreeWidgetItem * viewsItem);
    void buildFields(QTreeWidgetItem * tableItem);

  public Q_SLOTS:
    void buildTree(const QString &connection);
    void refresh();

    void generateSelect();
    void generateUpdate();
    void generateInsert();
    void generateDelete();
    void generateStatement(QSqlDriver::StatementType type);

  private Q_SLOTS:
    void slotCustomContextMenuRequested(const QPoint &pos);
    void slotItemExpanded(QTreeWidgetItem *item);

  private:
    void deleteChildren(QTreeWidgetItem *item);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool isConnectionValidAndOpen();

    QString m_connectionName;
    QPoint m_dragStartPosition;

    bool m_tablesLoaded;
    bool m_viewsLoaded;

    SQLManager *m_manager;
};

#endif // SCHEMAWIDGET_H
