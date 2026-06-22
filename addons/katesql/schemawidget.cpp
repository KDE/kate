/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "schemawidget.h"
#include "helpers/foreignkeyhelper.h"
#include "katesqlconstants.h"
#include "sqlmanager.h"

#include <KConfigGroup>
#include <KFuzzyMatcher>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QApplication>
#include <QDrag>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QSqlDatabase>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlRecord>
#include <QStringList>

SchemaWidget::SchemaWidget(QWidget *parent, SQLManager *manager)
    : QTreeWidget(parent)
    , m_manager(manager)
    , m_columnToForeignKeysMap()
    , m_tableToDisplayColumnMap()
{
    m_tablesLoaded = false;
    m_viewsLoaded = false;

    setHeaderLabels(QStringList{i18nc("@title:column", "Database schema")});

    setContextMenuPolicy(Qt::CustomContextMenu);
    setDragDropMode(QAbstractItemView::DragOnly);
    setDragEnabled(true);
    setAcceptDrops(false);

    connect(this, &SchemaWidget::customContextMenuRequested, this, &SchemaWidget::slotCustomContextMenuRequested);
    connect(this, &SchemaWidget::itemExpanded, this, &SchemaWidget::slotItemExpanded);
}

SchemaWidget::~SchemaWidget() = default;

bool SchemaWidget::isConnectionValidAndOpen() const
{
    return m_manager->isValidAndOpen(m_connectionName);
}

void SchemaWidget::deleteChildren(QTreeWidgetItem *item)
{
    const QList<QTreeWidgetItem *> items = item->takeChildren();

    for (QTreeWidgetItem *i : items) {
        delete i;
    }
}

void SchemaWidget::buildTree(const QString &connection)
{
    m_connectionName = connection;
    m_searchFilter.clear();
    m_systemTablesCache.clear();
    m_tablesCache.clear();
    m_viewsCache.clear();

    clear();

    m_tablesLoaded = false;
    m_viewsLoaded = false;

    if (!m_connectionName.isEmpty()) {
        loadForeignKeys();
        buildDatabase(new QTreeWidgetItem(this));
    }
}

void SchemaWidget::rebuildTreeWithFilter(const QString &filter)
{
    m_searchFilter = filter;

    clear();

    m_tablesLoaded = false;
    m_viewsLoaded = false;

    if (!m_connectionName.isEmpty() && isConnectionValidAndOpen()) {
        auto *dbItem = new QTreeWidgetItem(this);
        buildDatabase(dbItem);
        dbItem->setExpanded(true);

        // Immediately build tables and views (with filter) instead of lazy loading
        for (int i = 0; i < dbItem->childCount(); ++i) {
            QTreeWidgetItem *child = dbItem->child(i);
            if (child->type() == CustomUIType::TablesFolderType) {
                buildTables(child);
                child->setExpanded(true);
            } else if (child->type() == CustomUIType::ViewsFolderType) {
                buildViews(child);
                child->setExpanded(true);
            }
        }
    }
}

void SchemaWidget::refresh()
{
    // Force a live re-query of foreign keys and enums, and update the cache
    m_columnToForeignKeysMap.clear();
    m_tableToDisplayColumnMap.clear();

    buildTree(m_connectionName);
}

void SchemaWidget::buildDatabase(QTreeWidgetItem *databaseItem)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QString dbname = (db.isValid() ? db.databaseName() : m_connectionName);

    databaseItem->setText(0, dbname);
    databaseItem->setIcon(0, QIcon::fromTheme(QStringLiteral("server-database"))); // TODO better Icon from QIcon::ThemeIcon::...

    auto *tablesItem = new QTreeWidgetItem(databaseItem, CustomUIType::TablesFolderType);
    tablesItem->setText(0, i18nc("@title Folder name", "Tables"));
    tablesItem->setIcon(0, QIcon::fromTheme(QStringLiteral("folder"))); // TODO better Icon from QIcon::ThemeIcon::...
    tablesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    auto *viewsItem = new QTreeWidgetItem(databaseItem, CustomUIType::ViewsFolderType);
    viewsItem->setText(0, i18nc("@title Folder name", "Views"));
    viewsItem->setIcon(0, QIcon::fromTheme(QStringLiteral("folder"))); // TODO better Icon from QIcon::ThemeIcon::...
    viewsItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    databaseItem->setExpanded(true);
}

void SchemaWidget::buildTables(QTreeWidgetItem *tablesItem)
{
    if (!isConnectionValidAndOpen()) {
        return;
    }

    auto *systemTablesItem = new QTreeWidgetItem(tablesItem, CustomUIType::SystemTablesFolderType);
    systemTablesItem->setText(0, i18nc("@title Folder name", "System Tables"));
    systemTablesItem->setIcon(0, QIcon::fromTheme(QStringLiteral("folder"))); // TODO better Icon from QIcon::ThemeIcon::...
    systemTablesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    if (m_systemTablesCache.isEmpty()) {
        m_systemTablesCache = db.tables(QSql::SystemTables);
    }
    if (m_tablesCache.isEmpty()) {
        m_tablesCache = db.tables(QSql::Tables);
    }

    for (const QString &table : std::as_const(m_systemTablesCache)) {
        if (!m_searchFilter.isEmpty() && !KFuzzyMatcher::matchSimple(m_searchFilter, table)) {
            continue;
        }
        auto *item = new QTreeWidgetItem(systemTablesItem, CustomUIType::SystemTableType);
        item->setText(0, table);
        item->setIcon(0, QIcon(SqlTableIcon));
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    for (const QString &table : std::as_const(m_tablesCache)) {
        if (!m_searchFilter.isEmpty() && !KFuzzyMatcher::matchSimple(m_searchFilter, table)) {
            continue;
        }
        auto *item = new QTreeWidgetItem(tablesItem, CustomUIType::TableType);
        item->setText(0, table);
        item->setIcon(0, QIcon(SqlTableIcon));
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    m_tablesLoaded = true;
}

void SchemaWidget::buildViews(QTreeWidgetItem *viewsItem)
{
    if (!isConnectionValidAndOpen()) {
        return;
    }

    if (m_viewsCache.isEmpty()) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        m_viewsCache = db.tables(QSql::Views);
    }

    for (const QString &view : std::as_const(m_viewsCache)) {
        if (!m_searchFilter.isEmpty() && !KFuzzyMatcher::matchSimple(m_searchFilter, view)) {
            continue;
        }
        auto *item = new QTreeWidgetItem(viewsItem, CustomUIType::ViewType);
        item->setText(0, view);
        item->setIcon(0, QIcon(QLatin1String(":/katesql/pics/16-actions-sql-view.png")));
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    m_viewsLoaded = true;
}

void SchemaWidget::buildFields(QTreeWidgetItem *tableItem)
{
    if (!isConnectionValidAndOpen()) {
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    QString tableName = tableItem->text(0);

    QSqlIndex pk = db.primaryIndex(tableName);
    QSqlRecord rec = db.record(tableName);

    for (int i = 0; i < rec.count(); ++i) {
        QSqlField f = rec.field(i);

        QString fieldName = f.name();

        auto *item = new QTreeWidgetItem(tableItem, CustomUIType::FieldType);
        item->setText(0, fieldName);

        if (pk.contains(fieldName)) {
            item->setIcon(0, QIcon(QLatin1String(":/katesql/pics/16-actions-sql-field-pk.png")));
        } else {
            item->setIcon(0, QIcon(QLatin1String(":/katesql/pics/16-actions-sql-field.png")));
        }
    }
}

void SchemaWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    QTreeWidget::mousePressEvent(event);
}

void SchemaWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    //   QTreeWidgetItem *item = currentItem();
    QTreeWidgetItem *item = itemAt(event->pos());

    if (!item) {
        return;
    }

    if (item->type() != CustomUIType::SystemTableType && item->type() != CustomUIType::TableType && item->type() != CustomUIType::ViewType
        && item->type() != CustomUIType::FieldType) {
        return;
    }

    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;

    if (item->type() == CustomUIType::FieldType) {
        mimeData->setText(QStringLiteral("%1.%2").arg(item->parent()->text(0), item->text(0)));
    } else {
        mimeData->setText(item->text(0));
    }

    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);

    QTreeWidget::mouseMoveEvent(event);
}

void SchemaWidget::slotItemExpanded(QTreeWidgetItem *item)
{
    if (!item) {
        return;
    }

    switch (item->type()) {
    case CustomUIType::TablesFolderType: {
        if (!m_tablesLoaded) {
            buildTables(item);
        }
    } break;

    case CustomUIType::ViewsFolderType: {
        if (!m_viewsLoaded) {
            buildViews(item);
        }
    } break;

    case CustomUIType::TableType:
    case CustomUIType::SystemTableType:
    case CustomUIType::ViewType: {
        if (item->childCount() == 0) {
            buildFields(item);
        }
    } break;

    default:
        break;
    }
}

void SchemaWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->pos());

    if (item && (item->type() == CustomUIType::TableType || item->type() == CustomUIType::SystemTableType || item->type() == CustomUIType::ViewType)) {
        browseData();
        // Do NOT call base class — prevents the default expand/collapse toggle
        return;
    }

    QTreeWidget::mouseDoubleClickEvent(event);
}

void SchemaWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTreeWidgetItem *item = currentItem();
        if (item && (item->type() == CustomUIType::TableType || item->type() == CustomUIType::SystemTableType || item->type() == CustomUIType::ViewType)) {
            browseData();
            this->setFocus();
            return;
        }
    }
    QTreeWidget::keyPressEvent(event);
}

void SchemaWidget::slotCustomContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    QTreeWidgetItem *item = itemAt(pos);

    if (item) {
        if (item->type() == CustomUIType::SystemTableType || item->type() == CustomUIType::TableType || item->type() == CustomUIType::ViewType
            || item->type() == CustomUIType::FieldType) {
            menu.addAction(QIcon::fromTheme(QStringLiteral("view-sort-descending")), // TODO better Icon from QIcon::ThemeIcon::...
                           i18nc("@action:inmenu Context menu", "Select Data"),
                           this,
                           &SchemaWidget::browseData);
            QMenu *submenu = menu.addMenu(QIcon::fromTheme(QStringLiteral("tools-wizard")), // TODO better Icon from QIcon::ThemeIcon::...
                                          i18nc("@action:inmenu Submenu title", "Generate"));
            submenu->addAction(i18n("SELECT"), this, &SchemaWidget::generateSelectIntoView);
            submenu->addAction(i18n("UPDATE"), this, &SchemaWidget::generateUpdateIntoView);
            submenu->addAction(i18n("INSERT"), this, &SchemaWidget::generateInsertIntoView);
            submenu->addAction(i18n("DELETE"), this, &SchemaWidget::generateDeleteIntoView);
            menu.addSeparator();
        }
    }
    menu.addAction(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh), i18nc("@action:inmenu Context menu", "Refresh"), this, &SchemaWidget::refresh);

    menu.exec(mapToGlobal(pos));
}

QString SchemaWidget::generateStatement(QSqlDriver::StatementType statementType)
{
    if (!isConnectionValidAndOpen()) {
        return {};
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    QSqlDriver *drv = db.driver();

    if (!drv) {
        return {};
    }

    QTreeWidgetItem *item = currentItem();

    if (!item) {
        return {};
    }

    QString statement;

    switch (item->type()) {
    case CustomUIType::TableType:
    case CustomUIType::SystemTableType:
    case CustomUIType::ViewType: {
        QString tableName = item->text(0);

        QSqlRecord rec = db.record(tableName);

        // set all fields to a value (NULL)
        // values are needed to generate update and insert statements
        if (statementType == QSqlDriver::UpdateStatement || statementType == QSqlDriver::InsertStatement) {
            for (int i = 0, n = rec.count(); i < n; ++i) {
                rec.setNull(i);
            }
        }

        statement = drv->sqlStatement(statementType, tableName, rec, false);
    } break;

    case CustomUIType::FieldType: {
        QString tableName = item->parent()->text(0);
        QSqlRecord rec = db.record(tableName);

        // get the selected column...
        QSqlField field = rec.field(item->text(0));

        // ...and set its value to NULL
        field.clear();

        // clear all columns and re-append the selected one
        rec.clear();
        rec.append(field);

        statement = drv->sqlStatement(statementType, tableName, rec, false);

        if (statementType == QSqlDriver::DeleteStatement) {
            statement += u' ' + drv->sqlStatement(QSqlDriver::WhereStatement, tableName, rec, false).replace(QLatin1String(" IS NULL"), QLatin1String("=?"));
        }
    } break;
    }

    // replace NULL with a more generic '?'
    statement.replace(QLatin1String("NULL"), QLatin1String("?"));
    return statement;
}

void SchemaWidget::pasteStatementIntoActiveView(const QString &statement)
{
    KTextEditor::MainWindow *mw = KTextEditor::Editor::instance()->application()->activeMainWindow();
    KTextEditor::View *kv = mw->activeView();
    qDebug("Generated statement: %ls", qUtf16Printable(statement));

    if (!kv) {
        return;
    }
    // paste statement in the active view
    kv->insertText(statement);
    kv->setFocus();
}
void SchemaWidget::executeStatement(QSqlDriver::StatementType statementType)
{
    const QString statement = generateStatement(statementType);
    if (statement.length()) {
        m_manager->runQuery(statement, m_connectionName);
    }
}
void SchemaWidget::browseData()
{
    if (!isConnectionValidAndOpen()) {
        return;
    }

    QTreeWidgetItem *item = currentItem();

    if (!item) {
        return;
    }

    QString tableName;

    switch (item->type()) {
    case CustomUIType::TableType:
    case CustomUIType::SystemTableType:
    case CustomUIType::ViewType:
        tableName = item->text(0);
        break;

    case CustomUIType::FieldType:
        tableName = item->parent()->text(0);
        break;

    default:
        return;
    }

    const KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    const bool enableEditableTable =
        config.readEntry(KateSQLConstants::Config::EnableEditableTable, KateSQLConstants::Config::DefaultValues::EnableEditableTable);

    if (enableEditableTable) {
        if (canUseRelationalModel(tableName)) {
            m_manager->runEditableRelationalQuery(tableName, m_connectionName, m_columnToForeignKeysMap, m_tableToDisplayColumnMap);
        } else {
            m_manager->runEditableQuery(tableName, m_connectionName, m_tableToDisplayColumnMap);
        }
    } else {
        executeStatement(QSqlDriver::StatementType::SelectStatement);
    }
}

void SchemaWidget::generateAndPasteStatement(QSqlDriver::StatementType statementType)
{
    QString statement = generateStatement(statementType);
    pasteStatementIntoActiveView(statement);
}
void SchemaWidget::generateSelectIntoView()
{
    generateAndPasteStatement(QSqlDriver::SelectStatement);
}

void SchemaWidget::generateUpdateIntoView()
{
    generateAndPasteStatement(QSqlDriver::UpdateStatement);
}

void SchemaWidget::generateInsertIntoView()
{
    generateAndPasteStatement(QSqlDriver::InsertStatement);
}

void SchemaWidget::generateDeleteIntoView()
{
    generateAndPasteStatement(QSqlDriver::DeleteStatement);
}

bool SchemaWidget::isRelationalTablesEnabled() const
{
    const KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    return config.readEntry(KateSQLConstants::Config::EnableEditableRelationalTable, false);
}

void SchemaWidget::reloadDisplayColumnMap(const QString &tableName, const QString &columnName)
{
    if (!isConnectionValidAndOpen()) {
        return;
    }

    m_tableToDisplayColumnMap[tableName] = columnName;
}

void SchemaWidget::loadForeignKeys()
{
    if (!isConnectionValidAndOpen()) {
        m_columnToForeignKeysMap.clear();
        m_tableToDisplayColumnMap.clear();
        return;
    }

    m_columnToForeignKeysMap.clear();
    if (isRelationalTablesEnabled()) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        m_columnToForeignKeysMap = ForeignKeyHelper::getForeignKeys(db);
    }

    // Fill display column map after updating foreign keys
    fillTableToColumnMap();
}

void SchemaWidget::fillTableToColumnMap()
{
    if (!isConnectionValidAndOpen() || m_columnToForeignKeysMap.isEmpty()) {
        m_tableToDisplayColumnMap.clear();
        return;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    // Collect all referenced tables from foreign key relationships
    QSet<QString> referencedTables;
    for (const auto &columnFks : std::as_const(m_columnToForeignKeysMap)) {
        for (const auto &ref : columnFks) {
            referencedTables.insert(ref.first);
        }
    }

    const QString keywords[5] = {
        QStringLiteral("name"),
        QStringLiteral("title"),
        QStringLiteral("label"),
        QStringLiteral("display"),
        QStringLiteral("description"),
    };

    for (const QString &refTable : std::as_const(referencedTables)) {
        // Already resolved in-memory (from a prior run or user selection)
        if (m_tableToDisplayColumnMap.contains(refTable)) {
            continue;
        }

        // Auto-detect a display column
        QString displayColumn;

        QSqlRecord tableRecord = db.record(refTable);

        for (int i = 0; i < tableRecord.count(); ++i) {
            const QString fieldName = tableRecord.fieldName(i).toLower();
            for (const QString &keyword : keywords) {
                if (fieldName.contains(keyword)) {
                    displayColumn = tableRecord.fieldName(i);
                    break;
                }
            }
            if (!displayColumn.isEmpty()) {
                break;
            }
        }

        if (displayColumn.isEmpty()) {
            QSqlIndex pk = db.primaryIndex(refTable);
            if (pk.count() > 0) {
                displayColumn = pk.fieldName(0);
            } else if (tableRecord.count() > 0) {
                displayColumn = tableRecord.fieldName(0);
            }
        }

        m_tableToDisplayColumnMap[refTable] = displayColumn;
    }
}

bool SchemaWidget::canUseRelationalModel(const QString &tableName) const
{
    if (!isConnectionValidAndOpen()) {
        return false;
    }

    const ColumnForeignKeys tableFks = m_columnToForeignKeysMap.value(tableName);
    // Check if table has foreign keys
    if (tableFks.isEmpty()) {
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    // Check if table has a primary key
    QSqlIndex pk = db.primaryIndex(tableName);
    if (pk.isEmpty()) {
        // KMessageBox::error(nullptr, i18n("Table has no primary key"), i18n("Error"));
        return false;
    }

    // Check if primary key contains a relation to another table
    for (int i = 0; i < pk.count(); ++i) {
        const QString pkField = pk.fieldName(i);
        if (tableFks.contains(pkField)) {
            // KMessageBox::error(nullptr, i18n("Table primary key is a foreign key"), i18n("Error"));
            // Primary key column has a foreign key - not supported by QSqlRelationalTableModel
            return false;
        }
    }

    return true;
}
