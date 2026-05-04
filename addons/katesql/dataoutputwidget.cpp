/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputwidget.h"
#include "dataoutput/columndelegates/booleandelegate.h"
#include "dataoutput/columndelegates/datedelegate.h"
#include "dataoutput/columndelegates/datetimedelegate.h"
#include "dataoutput/columndelegates/enumdelegate.h"
#include "dataoutput/dataoutputeditablerelationalmodel.h"
#include "dataoutput/dataoutputmodel.h"
#include "dataoutput/dataoutputmodelinterface.h"
#include "dataoutputview.h"
#include "exportwizard.h"
#include "helpers/databaseconfigserializinghelper.h"
#include "helpers/dataoutputstylehelper.h"
#include "helpers/enumhelper.h"
#include "katesqlconstants.h"

#include <algorithm>

#include <KConfigGroup>
#include <KMessageBox>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KToggleAction>
#include <KToolBar>
#include <QAction>

#include <QApplication>
#include <QClipboard>
#include <QCompleter>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFocusEvent>
#include <QHeaderView>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMargins>
#include <QSize>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlRelationalDelegate>
#include <QSqlTableModel>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QtTypes>
#include <kmessagebox.h>
#include <qdir.h>
#include <qlatin1stringview.h>
#include <qlist.h>
#include <qmap.h>
#include <qsqldatabase.h>

DataOutputWidget::DataOutputWidget(QWidget *parent, KActionCollection *actionCollection)
    : QWidget(parent)
    , m_actionCollection(actionCollection)
    , m_model(nullptr)
    , m_view(new DataOutputView(this))
    , m_editableSection(nullptr)
    , m_isEditable(false)
    , m_editableOnlyRightClickActions(QList<QAction *>(qsizetype(
          7))) // change once we have more than insertRowAction + duplicateRowAction + removeRowAction + setNullAction + undoAction + pasteAction + saveAction
{
    m_view->setModel(nullptr);

    connect(m_view, &DataOutputView::contextMenuAboutToShow, this, &DataOutputWidget::slotUpdateMakeColumnPresentableAction);

    auto *layout = new QHBoxLayout(this);
    m_dataLayout = new QVBoxLayout();

    // Create vertical toolbar on the left
    m_verticalToolbar = new KToolBar(this);
    m_verticalToolbar->setOrientation(Qt::Vertical);
    m_verticalToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // ensure reasonable icons sizes, like e.g. the quick-open and co. icons
    // the normal toolbar sizes are TOO large, e.g. for scaled stuff even more!
    const int iconSize = style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, this);
    m_verticalToolbar->setIconSize(QSize(iconSize, iconSize));

    /// TODO: disable actions if no results are displayed or selected

    QAction *action;

    auto *refreshAction = m_actionCollection->addAction(QStringLiteral("data_refresh"));
    refreshAction->setText(i18nc("@action:intoolbar", "Refresh"));
    refreshAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh));
    refreshAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(refreshAction, Qt::CTRL | Qt::Key_R);
    connect(refreshAction, &QAction::triggered, this, &DataOutputWidget::slotRefresh);
    m_verticalToolbar->addAction(refreshAction);
    m_view->addAction(refreshAction);
    addAction(refreshAction);

    auto *resizeColumnsAction = m_actionCollection->addAction(QStringLiteral("data_resize_columns"));
    resizeColumnsAction->setText(i18nc("@action:intoolbar", "Resize columns to contents"));
    resizeColumnsAction->setIcon(QIcon::fromTheme(QLatin1String("distribute-horizontal-x")));
    connect(resizeColumnsAction, &QAction::triggered, this, &DataOutputWidget::resizeColumnsToContents);
    m_verticalToolbar->addAction(resizeColumnsAction);

    action = m_actionCollection->addAction(QStringLiteral("data_resize_rows"));
    action->setText(i18nc("@action:intoolbar", "Resize rows to contents"));
    action->setIcon(QIcon::fromTheme(QLatin1String("distribute-vertical-y")));
    connect(action, &QAction::triggered, this, &DataOutputWidget::resizeRowsToContents);
    m_verticalToolbar->addAction(action);

    action = m_actionCollection->addAction(QStringLiteral("data_export"));
    action->setText(i18nc("@action:intoolbar", "Export..."));
    action->setIcon(QIcon::fromTheme(QLatin1String("document-export-table")));
    connect(action, &QAction::triggered, this, &DataOutputWidget::slotExport);
    m_verticalToolbar->addAction(action);
    m_view->addAction(action);

    action = m_actionCollection->addAction(QStringLiteral("data_copy"));
    action->setText(i18nc("@action:intoolbar", "Copy"));
    action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy));
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(action, Qt::CTRL | Qt::Key_C);
    connect(action, &QAction::triggered, this, &DataOutputWidget::slotCopySelected);
    m_verticalToolbar->insertAction(resizeColumnsAction, action);
    m_view->addAction(action);
    addAction(action);

    action = m_actionCollection->addAction(QStringLiteral("data_clear"));
    action->setText(i18nc("@action:intoolbar", "Clear"));
    action->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
    connect(action, &QAction::triggered, this, &DataOutputWidget::clearResults);
    m_verticalToolbar->addAction(action);

    m_verticalToolbar->addSeparator();

    auto *toggleAction =
        new KToggleAction(QIcon::fromTheme(QIcon::ThemeIcon::FormatTextDirectionRtl), i18nc("@action:intoolbar", "Use system locale"), m_actionCollection);
    m_actionCollection->addAction(QStringLiteral("data_toggle_locale"), toggleAction);
    m_verticalToolbar->addAction(toggleAction);
    connect(toggleAction, &QAction::triggered, this, &DataOutputWidget::slotToggleLocale);

    // === Section for Editable Table (only visible for editable tables) ===
    m_editableSection = new QWidget(this);
    auto *editableLayout = new QHBoxLayout(m_editableSection);
    editableLayout->setContentsMargins(0, 0, 0, 0);

    m_editableToolbar = new KToolBar(m_editableSection);
    m_editableToolbar->setOrientation(Qt::Horizontal);
    m_editableToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_editableToolbar->setIconSize(QSize(iconSize, iconSize));

    auto *pasteAction = m_actionCollection->addAction(QStringLiteral("data_paste"));
    pasteAction->setText(i18nc("@action:intoolbar", "Paste"));
    pasteAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditPaste));
    pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(pasteAction, Qt::CTRL | Qt::Key_V);
    connect(pasteAction, &QAction::triggered, this, &DataOutputWidget::slotPaste);
    m_editableOnlyRightClickActions.push_back(pasteAction);
    m_editableToolbar->addAction(pasteAction);
    addAction(pasteAction);

    auto *setNullAction = m_actionCollection->addAction(QStringLiteral("data_set_null"));
    setNullAction->setText(i18nc("@action:intoolbar", "Set Null"));
    setNullAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentRevert));
    setNullAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(setNullAction, Qt::CTRL | Qt::SHIFT | Qt::Key_X);
    connect(setNullAction, &QAction::triggered, this, &DataOutputWidget::slotSetNull);
    m_editableOnlyRightClickActions.push_back(setNullAction);
    m_editableToolbar->addAction(setNullAction);
    addAction(setNullAction);

    auto *insertRowAction = m_actionCollection->addAction(QStringLiteral("data_insert_row"));
    insertRowAction->setText(i18nc("@action:intoolbar", "Insert Row"));
    insertRowAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
    insertRowAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(insertRowAction, Qt::ALT | Qt::Key_Insert);
    connect(insertRowAction, &QAction::triggered, this, &DataOutputWidget::slotInsertRow);
    m_editableOnlyRightClickActions.push_back(insertRowAction);
    m_editableToolbar->addAction(insertRowAction);
    addAction(insertRowAction);

    auto *removeRowAction = m_actionCollection->addAction(QStringLiteral("data_remove_rows"));
    removeRowAction->setText(i18nc("@action:intoolbar", "Remove Selected Rows"));
    removeRowAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove));
    removeRowAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(removeRowAction, Qt::Key_Delete);
    connect(removeRowAction, &QAction::triggered, this, &DataOutputWidget::slotRemoveSelectedRows);
    m_editableOnlyRightClickActions.push_back(removeRowAction);
    m_editableToolbar->addAction(removeRowAction);
    addAction(removeRowAction);

    auto *duplicateRowAction = m_actionCollection->addAction(QStringLiteral("data_duplicate_rows"));
    duplicateRowAction->setText(i18nc("@action:intoolbar", "Duplicate Selected Rows"));
    duplicateRowAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy));
    duplicateRowAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(duplicateRowAction, Qt::CTRL | Qt::Key_D);
    connect(duplicateRowAction, &QAction::triggered, this, &DataOutputWidget::slotDuplicateRows);
    m_editableOnlyRightClickActions.push_back(duplicateRowAction);
    m_editableToolbar->addAction(duplicateRowAction);
    addAction(duplicateRowAction);

    auto *undoAction = m_actionCollection->addAction(QStringLiteral("data_undo"));
    undoAction->setText(i18nc("@action:intoolbar", "Undo"));
    undoAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditUndo));
    undoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(undoAction, Qt::CTRL | Qt::Key_Z);
    connect(undoAction, &QAction::triggered, this, &DataOutputWidget::slotUndo);
    m_editableOnlyRightClickActions.push_back(undoAction);
    m_editableToolbar->addAction(undoAction);
    addAction(undoAction);

    auto *saveAction = m_actionCollection->addAction(QStringLiteral("data_save"));
    saveAction->setText(i18nc("@action:intoolbar", "Save"));
    saveAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave));
    saveAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    // KActionCollection::setDefaultShortcut(saveAction, Qt::CTRL | Qt::Key_S);
    connect(saveAction, &QAction::triggered, this, &DataOutputWidget::slotSave);
    m_editableOnlyRightClickActions.push_back(saveAction);
    m_editableToolbar->addAction(saveAction);
    addAction(saveAction);

    m_makeColumnPresentableAction = m_actionCollection->addAction(QStringLiteral("data_make_column_presentable"));
    m_makeColumnPresentableAction->setText(i18nc("@action:intoolbar", "Make this column presentable for related tables"));
    m_makeColumnPresentableAction->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::FormatIndentMore));
    connect(m_makeColumnPresentableAction, &QAction::triggered, this, &DataOutputWidget::slotMakeColumnPresentable);
    m_view->addAction(m_makeColumnPresentableAction);

    // Separator between sections
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_editableToolbar->addWidget(separator);

    auto *whereLabel = new QLabel(i18nc("@label", "Where:"), this);
    QPalette palette = whereLabel->palette();
    palette.setColor(QPalette::WindowText, palette.color(QPalette::Highlight));
    whereLabel->setPalette(palette);
    whereLabel->setContentsMargins(QMargins(16, 0, 8, 0));
    m_editableToolbar->addWidget(whereLabel);

    m_filterInput = new QLineEdit(this);
    m_filterInput->setPlaceholderText(i18nc("@info:placeholder", "id > 0"));
    m_filterInput->setToolTip(i18nc("@info:tooltip", "Enter SQL WHERE clause to filter data"));
    m_filterInput->setClearButtonEnabled(true);
    m_filterInput->setMaximumWidth(250);
    m_editableToolbar->addWidget(m_filterInput);
    connect(m_filterInput, &QLineEdit::returnPressed, this, &DataOutputWidget::slotSetFilter);

    editableLayout->addWidget(m_editableToolbar);
    m_editableSection->setLayout(editableLayout);

    // Add horizontal toolbar container above the view
    m_dataLayout->addWidget(m_editableSection);
    m_dataLayout->addWidget(m_view);

    layout->addWidget(m_verticalToolbar);
    layout->addLayout(m_dataLayout);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);

    readConfig();
    adjustToEditableStateChange();
}

DataOutputWidget::~DataOutputWidget() = default;

// TODO dig deeper of why system theme change may breaks the toolbar style - remove this once the issue is fixed
void DataOutputWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        m_verticalToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_editableToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        readConfig();
    }
}

void DataOutputWidget::adjustToEditableStateChange()
{
    m_editableSection->setHidden(!m_isEditable);
    m_view->setSortingEnabled(m_isEditable);

    for (auto *action : std::as_const(m_editableOnlyRightClickActions)) {
        if (action) {
            action->setEnabled(m_isEditable);
        }
    }

    m_makeColumnPresentableAction->setEnabled(m_relationalTablesEnabled);

    m_makeColumnPresentableAction->setVisible(m_relationalTablesEnabled);

    if (m_relationalTablesEnabled && m_model) {
        const auto table = qobject_cast<QSqlTableModel *>(m_model->asQObject());
        if (table) {
            const auto column = m_tableToDisplayColumnMap.value(table->tableName());
            if (column.isEmpty()) {
                m_currentPresentableColumn = 0;
            } else {
                m_currentPresentableColumn = table->fieldIndex(column);
            }
        }
    }
    setupColumnDelegates();
}

void DataOutputWidget::setupColumnDelegates()
{
    if (!m_model || m_connectionName.isEmpty()) {
        return;
    }

    auto *model = abstractModel();

    // Remove and delete old per-column delegates
    for (int col = 0; col < model->columnCount(); ++col) {
        if (auto *oldDelegate = m_view->itemDelegateForColumn(col)) {
            m_view->setItemDelegateForColumn(col, nullptr);
            oldDelegate->deleteLater();
        }
    }

    auto *sqlTableModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());

    if (!sqlTableModel) {
        return;
    }

    const auto rec = sqlTableModel->record();

    // Load enum column values from config cache for the current table
    TableEnums columnEnums;
    const QString tableName = sqlTableModel->tableName();
    if (!tableName.isEmpty()) {
        KConfigGroup enumConfig(KSharedConfig::openConfig(), KateSQLConstants::Config::DatabaseEnumsGroup);
        if (DatabaseConfigSerializerHelper::hasEnums(enumConfig, m_connectionName)) {
            const DatabaseEnums dbEnums = DatabaseConfigSerializerHelper::readEnums(enumConfig, m_connectionName);
            columnEnums = dbEnums.value(tableName);
        }
    }

    const auto relationalModel = qobject_cast<DataOutputEditableRelationalModel *>(m_model->asQObject());

    for (int col = 0; col < model->columnCount() && col < rec.count(); ++col) {
        const auto field = rec.field(col);

        // Only set QSqlRelationalDelegate for columns that actually have a
        // relation defined in the relational model. Do not override delegates
        // for other columns.
        if (relationalModel && relationalModel->relation(col).isValid()) {
            m_view->setItemDelegateForColumn(col, new QSqlRelationalDelegate(m_view));
            continue;
        }

        // Check if this column has enum values
        const QString fieldName = rec.fieldName(col);
        if (columnEnums.contains(fieldName)) {
            m_view->setItemDelegateForColumn(col, new EnumDelegate(columnEnums.value(fieldName), m_view));
            continue;
        }

        switch (field.metaType().id()) {
        case QMetaType::Bool:
            m_view->setItemDelegateForColumn(col, new BooleanDelegate(m_view));
            break;
        case QMetaType::QDate:
            m_view->setItemDelegateForColumn(col, new DateDelegate(m_view));
            break;
        case QMetaType::QDateTime:
            m_view->setItemDelegateForColumn(col, new DateTimeDelegate(m_view));
            break;
        default:
            break;
        }
    }
}

void DataOutputWidget::showQueryResultSets(QSqlQuery &query, const QString &connectionName)
{
    m_connectionName = connectionName;
    /// TODO: loop resultsets if > 1
    /// NOTE from Qt Documentation:
    /// When one of the statements is a non-select statement a count of affected rows
    /// may be available instead of a result set.

    if (!query.isSelect() || query.lastError().isValid()) {
        return;
    }

    for (auto *action : m_editableOnlyRightClickActions) {
        m_view->removeAction(action);
    }

    // If we didnt clean up table model, delete it
    if (m_model) {
        auto *tableModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
        if (tableModel) {
            delete m_model;
            m_model = nullptr;
        }
    }

    // if model is now or was null, create a new DataOutputModel
    if (m_model == nullptr) {
        m_model = new DataOutputModel(this);
        m_view->setModel(static_cast<QAbstractItemModel *>(m_model->asQObject()));
    }

    // Now m_model should be a DataOutputModel
    auto *queryModel = qobject_cast<DataOutputModel *>(m_model->asQObject());
    queryModel->setStyleHelper(&m_styleHelper);
    queryModel->setQuery(std::move(query));

    m_isEditable = false;

    adjustToEditableStateChange();

    QTimer::singleShot(0, this, &DataOutputWidget::resizeColumnsToContents);

    raise();
}

void DataOutputWidget::showEditableTable(DataOutputModelInterface *model, const QString &connectionName, const QMap<QString, QString> &tableToDisplayColumnMap)
{
    if (!model) {
        return;
    }
    m_connectionName = connectionName;
    m_tableToDisplayColumnMap = tableToDisplayColumnMap;

    m_view->addActions(m_editableOnlyRightClickActions);

    if (m_model != nullptr) {
        // Delete current model
        delete m_model;
    }

    m_model = model;
    abstractModel()->setParent(this);

    m_model->setStyleHelper(&m_styleHelper);

    // Set the shared style helper and autocompleter
    auto *editableModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (editableModel) {
        editableModel->setEditStrategy(QSqlTableModel::EditStrategy::OnManualSubmit);

        if (m_filterInput->completer()) {
            m_filterInput->completer()->deleteLater();
        }

        QStringList columnNamesFilterCompletionList;
        columnNamesFilterCompletionList.reserve(editableModel->columnCount());

        for (int i = 0; i < editableModel->columnCount(); ++i) {
            columnNamesFilterCompletionList.emplace_back(editableModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString());
        }

        QCompleter *completer = new QCompleter(columnNamesFilterCompletionList, m_filterInput);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains); // or MatchStartsWith

        completer->setCompletionMode(QCompleter::CompletionMode::PopupCompletion);
        m_filterInput->setCompleter(completer);
    }

    m_view->setModel(abstractModel());

    m_isEditable = true;

    adjustToEditableStateChange();

    QTimer::singleShot(0, this, &DataOutputWidget::resizeColumnsToContents);

    raise();
}

void DataOutputWidget::clearResults()
{
    if (m_model != nullptr) {
        m_model->clear();
        // Delete current model
        delete m_model;
        m_model = nullptr;
        m_view->setModel(static_cast<QAbstractItemModel *>(nullptr));
    }

    m_isEditable = false;
    adjustToEditableStateChange();

    /// HACK needed to refresh headers. please correct if there's a better way
    m_view->horizontalHeader()->hide();
    m_view->verticalHeader()->hide();

    m_view->horizontalHeader()->show();
    m_view->verticalHeader()->show();
}

void DataOutputWidget::readConfig()
{
    m_styleHelper.readConfig();

    const KConfigGroup config(KSharedConfig::openConfig(), KateSQLConstants::Config::PluginGroup);
    m_relationalTablesEnabled = config.readEntry(KateSQLConstants::Config::EnableEditableRelationalTable, false);

    if (m_makeColumnPresentableAction) {
        m_makeColumnPresentableAction->setVisible(m_relationalTablesEnabled);
    }
}

void DataOutputWidget::resizeColumnsToContents()
{
    if (m_model == nullptr || abstractModel()->rowCount() == 0) {
        return;
    }

    m_view->resizeColumnsToContents();
}

void DataOutputWidget::resizeRowsToContents()
{
    if (m_model == nullptr || abstractModel()->rowCount() == 0) {
        return;
    }

    m_view->resizeRowsToContents();

    int h = m_view->rowHeight(0);

    if (h > 0) {
        m_view->verticalHeader()->setDefaultSectionSize(h);
    }
}

void DataOutputWidget::slotToggleLocale()
{
    if (m_model != nullptr) {
        m_model->setUseSystemLocale(!m_model->useSystemLocale());
    }
}

void DataOutputWidget::slotCopySelected()
{
    if (m_model == nullptr || abstractModel()->rowCount() <= 0) {
        return;
    }

    if (!m_view->selectionModel()->hasSelection()) {
        while (abstractModel()->canFetchMore(QModelIndex())) {
            abstractModel()->fetchMore(QModelIndex());
        }

        m_view->selectAll();
    }

    QString text;
    QTextStream stream(&text);

    exportData(stream);

    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
    }
}

void DataOutputWidget::slotExport()
{
    if (m_model == nullptr || abstractModel()->rowCount() <= 0) {
        return;
    }

    while (abstractModel()->canFetchMore(QModelIndex())) {
        abstractModel()->fetchMore(QModelIndex());
    }

    if (!m_view->selectionModel()->hasSelection()) {
        m_view->selectAll();
    }

    ExportWizard wizard(this);

    if (wizard.exec() != QDialog::Accepted) {
        return;
    }

    bool outputInDocument = wizard.field(KateSQLConstants::Export::Fields::OutputDocument).toBool();
    bool outputInClipboard = wizard.field(KateSQLConstants::Export::Fields::OutputClipboard).toBool();
    bool outputInFile = wizard.field(KateSQLConstants::Export::Fields::OutputFile).toBool();

    bool exportColumnNames = wizard.field(KateSQLConstants::Export::Fields::ExportColumnNames).toBool();
    bool exportLineNumbers = wizard.field(KateSQLConstants::Export::Fields::ExportLineNumbers).toBool();

    Options opt = NoOptions;

    if (exportColumnNames) {
        opt |= ExportColumnNames;
    }
    if (exportLineNumbers) {
        opt |= ExportLineNumbers;
    }

    bool quoteStrings = wizard.field(KateSQLConstants::Export::Fields::CheckQuoteStrings).toBool();
    bool quoteNumbers = wizard.field(KateSQLConstants::Export::Fields::CheckQuoteNumbers).toBool();

    QChar stringsQuoteChar = (quoteStrings) ? wizard.field(KateSQLConstants::Export::Fields::QuoteStringsChar).toString().at(0) : u'\0';
    QChar numbersQuoteChar = (quoteNumbers) ? wizard.field(KateSQLConstants::Export::Fields::QuoteNumbersChar).toString().at(0) : u'\0';

    QString fieldDelimiter = wizard.field(KateSQLConstants::Export::Fields::FieldDelimiter)
                                 .toString()
                                 .replace(KateSQLConstants::Export::Delimiters::Tab, QLatin1String("\t"))
                                 .replace(KateSQLConstants::Export::Delimiters::CarriageReturn, QLatin1String("\r"))
                                 .replace(KateSQLConstants::Export::Delimiters::Newline, QLatin1String("\n"));

    if (outputInDocument) {
        KTextEditor::MainWindow *mw = KTextEditor::Editor::instance()->application()->activeMainWindow();
        KTextEditor::View *kv = mw->activeView();

        if (!kv) {
            return;
        }

        QString text;
        QTextStream stream(&text);

        exportData(stream, stringsQuoteChar, numbersQuoteChar, fieldDelimiter, opt);

        kv->insertText(text);
        kv->setFocus();
    } else if (outputInClipboard) {
        QString text;
        QTextStream stream(&text);

        exportData(stream, stringsQuoteChar, numbersQuoteChar, fieldDelimiter, opt);

        QApplication::clipboard()->setText(text);
    } else if (outputInFile) {
        QString url = wizard.field(KateSQLConstants::Export::Fields::OutputFileUrl).toString();
        QFile file(url);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream stream(&file);

            exportData(stream, stringsQuoteChar, numbersQuoteChar, fieldDelimiter, opt);

            stream.flush();
        } else {
            KMessageBox::error(this, xi18nc("@info", "Unable to open file <filename>%1</filename>", url));
        }
    }
}

void DataOutputWidget::slotSave()
{
    m_view->commitCurrentEditorData();

    if (!m_model) {
        return;
    }

    auto *tableModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (!tableModel) {
        return;
    }

    if (!tableModel->submitAll()) {
        QSqlError err = tableModel->lastError();
        KMessageBox::error(this, xi18nc("@info", "Failed to save changes: <message>%1</message>", err.text()));
        return;
    }
    // tableModel->refresh()
}

void DataOutputWidget::slotRefresh()
{
    if (m_model != nullptr) {
        m_model->refresh();
    }
}

void DataOutputWidget::slotInsertRow()
{
    if (m_model == nullptr || !m_isEditable) {
        return;
    }

    auto *sqlModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (sqlModel == nullptr) {
        return;
    }

    int row = 0; // sqlModel->rowCount();
    if (!sqlModel->insertRow(row)) {
        return;
    }

    // Scroll to the new row and start editing
    QModelIndex newIndex = sqlModel->index(row, 0);
    m_view->scrollTo(newIndex);
    m_view->setCurrentIndex(newIndex);
}

void DataOutputWidget::slotDuplicateRows()
{
    if (m_model == nullptr || !m_isEditable) {
        return;
    }

    auto *sqlModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (sqlModel == nullptr) {
        return;
    }

    QItemSelectionModel *selectionModel = m_view->selectionModel();
    if (selectionModel == nullptr) {
        return;
    }

    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        return;
    }
    QSet<int> rowsToDuplicate;
    rowsToDuplicate.reserve(selectedIndexes.length());
    for (const QModelIndex &index : selectedIndexes) {
        rowsToDuplicate.insert(index.row());
    }

    QList<QSqlRecord> recordsToInsert;
    recordsToInsert.reserve(rowsToDuplicate.size());

    for (int row : rowsToDuplicate) {
        recordsToInsert.append(sqlModel->record(row));
    }

    QSqlIndex primary = sqlModel->primaryKey();
    for (QSqlRecord record : recordsToInsert) {
        for (int i = 0; i < primary.count(); ++i) {
            QString col = primary.fieldName(i);
            record.setNull(col);
        }
        sqlModel->insertRecord(0, record);
    }

    int firstPrimaryCol = primary.count() ? sqlModel->fieldIndex(primary.fieldName(0)) : 0;

    QModelIndex newIndex = sqlModel->index(0, firstPrimaryCol);
    m_view->scrollTo(newIndex);
    m_view->setCurrentIndex(newIndex);
}

void DataOutputWidget::slotRemoveSelectedRows()
{
    if (m_model == nullptr || !m_isEditable) {
        return;
    }

    auto *sqlModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (sqlModel == nullptr) {
        return;
    }

    QItemSelectionModel *selectionModel = m_view->selectionModel();
    if (selectionModel == nullptr) {
        return;
    }

    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        return;
    }
    // Extract unique row numbers
    QSet<int> rowsToRemove;

    rowsToRemove.reserve(selectedIndexes.length());
    for (const QModelIndex &index : selectedIndexes) {
        rowsToRemove.insert(index.row());
    }

    // Convert to list and sort in descending order
    QList<int> sortedRows = rowsToRemove.values();
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

    // Remove selected rows
    for (const int row : sortedRows) {
        sqlModel->removeRow(row);
        // m_view->hideRow(index.row()); to show this column when data is refreshed takes more state management than adding hihlighting which is gonna be the
        // next steps
    }
}

void DataOutputWidget::slotSetNull()
{
    if (m_model == nullptr || !m_isEditable) {
        return;
    }

    auto *sqlModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (sqlModel == nullptr) {
        return;
    }

    QItemSelectionModel *selectionModel = m_view->selectionModel();
    if (selectionModel == nullptr) {
        return;
    }

    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        return;
    }

    for (const QModelIndex &index : selectedIndexes) {
        sqlModel->setData(index, QVariant(), Qt::EditRole);
    }
}

void DataOutputWidget::slotUndo()
{
    if (m_model == nullptr || !m_isEditable) {
        return;
    }

    auto *sqlModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (sqlModel == nullptr) {
        return;
    }

    QItemSelectionModel *selectionModel = m_view->selectionModel();
    if (selectionModel == nullptr) {
        return;
    }

    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    if (selectedIndexes.isEmpty()) {
        // No selection: revert all changes
        sqlModel->revertAll();
    } else {
        // Revert changes for selected rows
        QSet<int> rowsToRevert;
        for (const QModelIndex &index : selectedIndexes) {
            rowsToRevert.insert(index.row());
        }

        for (const int row : rowsToRevert) {
            sqlModel->revertRow(row);
        }
    }
}

void DataOutputWidget::slotSetFilter()
{
    if (m_model == nullptr || !m_isEditable) {
        return;
    }

    auto *sqlModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (sqlModel == nullptr) {
        return;
    }

    QString filter = m_filterInput->text().trimmed();
    sqlModel->setFilter(filter);
    sqlModel->select();
}

void DataOutputWidget::exportData(QTextStream &stream,
                                  const QChar stringsQuoteChar,
                                  const QChar numbersQuoteChar,
                                  const QString fieldDelimiter,
                                  const Options opt)
{
    QItemSelectionModel *selectionModel = m_view->selectionModel();

    if (!selectionModel->hasSelection()) {
        return;
    }

    QElapsedTimer t;
    t.start();

    std::vector<int> columns;
    std::vector<int> rows;
    QHash<QPair<int, int>, QString> snapshot;

    const QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    snapshot.reserve(selectedIndexes.count());

    for (const QModelIndex &index : selectedIndexes) {
        const QVariant indexData = index.data(Qt::UserRole);

        const int col = index.column();
        const int row = index.row();

        columns.push_back(col);
        rows.push_back(row);

        if (indexData.typeId() < 7) // is numeric or boolean
        {
            if (numbersQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar) {
                snapshot[qMakePair(row, col)] = numbersQuoteChar + indexData.toString() + numbersQuoteChar;
            } else {
                snapshot[qMakePair(row, col)] = indexData.toString();
            }
        } else {
            if (stringsQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar) {
                snapshot[qMakePair(row, col)] = stringsQuoteChar + indexData.toString() + stringsQuoteChar;
            } else {
                snapshot[qMakePair(row, col)] = indexData.toString();
            }
        }
    }

    // uniquify
    std::sort(rows.begin(), rows.end());
    std::sort(columns.begin(), columns.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    columns.erase(std::unique(columns.begin(), columns.end()), columns.end());

    if (opt.testFlag(ExportColumnNames)) {
        if (opt.testFlag(ExportLineNumbers)) {
            stream << fieldDelimiter;
        }

        for (auto it = columns.begin(); it != columns.end(); ++it) {
            const QVariant headerData = abstractModel()->headerData(*it, Qt::Horizontal);

            if (stringsQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar) {
                stream << stringsQuoteChar + headerData.toString() + stringsQuoteChar;
            } else {
                stream << headerData.toString();
            }

            if (it + 1 != columns.end()) {
                stream << fieldDelimiter;
            }
        }
        stream << KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste;
    }

    for (const int row : std::as_const(rows)) {
        if (opt.testFlag(ExportLineNumbers)) {
            stream << row + 1 << fieldDelimiter;
        }

        for (auto it = columns.begin(); it != columns.end(); ++it) {
            stream << snapshot.value(qMakePair(row, *it));

            if (it + 1 != columns.end()) {
                stream << fieldDelimiter;
            }
        }
        stream << KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste;
    }

    qDebug("Export in %lld ms", t.elapsed());
}

void DataOutputWidget::importData(QTextStream &stream, const QChar stringsQuoteChar, const QString &fieldDelimiter, const QString &lineDelimiter)
{
    QItemSelectionModel *selectionModel = m_view->selectionModel();

    if (!selectionModel->hasSelection()) {
        return;
    }

    auto *sqlModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (sqlModel == nullptr) {
        return;
    }

    // Get the top-left selected cell
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        return;
    }

    // Sort to get the top-left position
    std::sort(selectedIndexes.begin(), selectedIndexes.end(), [](const QModelIndex &a, const QModelIndex &b) {
        if (a.row() != b.row())
            return a.row() < b.row();
        return a.column() < b.column();
    });

    QModelIndex startIndex = selectedIndexes.first();

    QString data = stream.readAll();
    QStringList lines = data.split(lineDelimiter, Qt::SkipEmptyParts);

    int startRow = startIndex.row();
    int startCol = startIndex.column();

    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        QString line = lines[lineIdx];
        QStringList fields;

        // Parse the line - handle quoted fields
        int pos = 0;
        while (pos < line.length()) {
            QString field;

            // Check if field starts with quote
            if (stringsQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar && pos < line.length() && line[pos] == stringsQuoteChar) {
                pos++; // Skip opening quote
                // Find closing quote
                while (pos < line.length() && line[pos] != stringsQuoteChar) {
                    field += line[pos];
                    pos++;
                }
                if (pos < line.length()) {
                    pos++; // Skip closing quote
                }
                // Skip delimiter if present
                if (line.mid(pos, fieldDelimiter.length()) == fieldDelimiter) {
                    pos += fieldDelimiter.length();
                }
            } else {
                // Read until delimiter or end
                while (pos < line.length() && line.mid(pos, fieldDelimiter.length()) != fieldDelimiter) {
                    field += line[pos];
                    pos++;
                }
                // Skip delimiter
                if (pos < line.length()) {
                    pos += fieldDelimiter.length();
                }
            }

            fields.append(field);
        }

        int targetRow = startRow + lineIdx;

        // Check if we're past the end of the model
        if (targetRow >= sqlModel->rowCount()) {
            break;
        }

        for (int fieldIdx = 0; fieldIdx < fields.size(); ++fieldIdx) {
            int targetCol = startCol + fieldIdx;

            if (targetCol >= sqlModel->columnCount()) {
                break;
            }

            QModelIndex targetIndex = sqlModel->index(targetRow, targetCol);
            sqlModel->setData(targetIndex, fields[fieldIdx]);
        }
    }
}

void DataOutputWidget::slotPaste()
{
    if (!m_isEditable) {
        return;
    }

    QString text = QApplication::clipboard()->text();
    if (text.isEmpty()) {
        return;
    }

    QTextStream stream(&text);

    importData(stream,
               KateSQLConstants::Export::DefaultValues::QuoteStringCharForCopyPaste,
               KateSQLConstants::Export::DefaultValues::FieldDelimiterForCopyPaste,
               KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste);
}

void DataOutputWidget::slotUpdateMakeColumnPresentableAction(const QPoint &pos, int column)
{
    Q_UNUSED(pos);

    if (!m_makeColumnPresentableAction) {
        return;
    }

    // Check if this column can have a relation set up
    m_makeColumnPresentableAction->setEnabled(m_currentPresentableColumn != column);
}

void DataOutputWidget::slotMakeColumnPresentable()
{
    if (!m_model || !m_isEditable) {
        return;
    }

    auto *editableModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (!editableModel) {
        return;
    }

    // Get the current column from the selection
    const QModelIndex currentIndex = m_view->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    const int column = currentIndex.column();

    if (m_currentPresentableColumn == column) {
        return;
    }

    m_currentPresentableColumn = column;

    const QString columnName = editableModel->record().fieldName(column);
    const QString tableName = editableModel->tableName();

    m_tableToDisplayColumnMap[tableName] = columnName;

    Q_EMIT displayColumnMapChanged(tableName, columnName);
}

#include "moc_dataoutputwidget.cpp"
