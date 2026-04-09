/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

class SQLManager;
class QMouseEvent;

#include <QSqlDriver>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>

class SchemaWidget : public QTreeWidget
{
public:
    enum CustomUIType {
        TableType = QTreeWidgetItem::UserType + 1,
        SystemTableType = QTreeWidgetItem::UserType + 2,
        ViewType = QTreeWidgetItem::UserType + 3,
        FieldType = QTreeWidgetItem::UserType + 4,
        TablesFolderType = QTreeWidgetItem::UserType + 101,
        SystemTablesFolderType = QTreeWidgetItem::UserType + 102,
        ViewsFolderType = QTreeWidgetItem::UserType + 103,
    };

    SchemaWidget(QWidget *parent, SQLManager *manager);
    ~SchemaWidget() override;

    void buildDatabase(QTreeWidgetItem *databaseItem);
    void buildTables(QTreeWidgetItem *tablesItem);
    void buildViews(QTreeWidgetItem *viewsItem);
    void buildFields(QTreeWidgetItem *tableItem);

public:
    void buildTree(const QString &connection);
    void refresh();

    void generateSelectIntoView();
    void generateUpdateIntoView();
    void generateInsertIntoView();
    void generateDeleteIntoView();
    void browseData();
    QString generateStatement(QSqlDriver::StatementType statementType);
    static void pasteStatementIntoActiveView(const QString &statement);
    void generateAndPasteStatement(QSqlDriver::StatementType statementType);
    void executeStatement(QSqlDriver::StatementType statement);

private:
    static inline constexpr QLatin1String SqlTableIcon = QLatin1String(":/katesql/pics/16-actions-sql-table.png");

    void slotCustomContextMenuRequested(const QPoint &pos);
    void slotItemExpanded(QTreeWidgetItem *item);

private:
    static void deleteChildren(QTreeWidgetItem *item);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool isConnectionValidAndOpen();
    QString m_connectionName;
    QPoint m_dragStartPosition;

    bool m_tablesLoaded;
    bool m_viewsLoaded;

    SQLManager *m_manager;
};
