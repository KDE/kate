/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputwidget.h"
#include "dataoutputeditablemodel.h"
#include "dataoutputmodel.h"
#include "dataoutputmodelinterface.h"
#include "dataoutputstylehelper.h"
#include "dataoutputview.h"
#include "exportwizard.h"
#include "katesqlconstants.h"

#include <algorithm>

#include <KMessageBox>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

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
#include <QSqlTableModel>
#include <QStyle>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QtTypes>

DataOutputWidget::DataOutputWidget(QWidget *parent)
    : QWidget(parent)
    , m_model(nullptr)
    , m_view(new DataOutputView(this))
    , m_editableSection(nullptr)
    , m_isEditable(false)
    , m_editableOnlyRightClickActions(QList<QAction *>(
          qsizetype(6))) // change once we have more than insertRowAction + duplicateRowAction + removeRowAction + setNullAction + undoAction + pasteAction
{
    readConfig();
    m_view->setModel(nullptr);

    auto *layout = new QHBoxLayout(this);
    m_dataLayout = new QVBoxLayout();

    // Create vertical toolbar on the left
    auto *verticalToolbar = new KToolBar(this);
    verticalToolbar->setOrientation(Qt::Vertical);
    verticalToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    // ensure reasonable icons sizes, like e.g. the quick-open and co. icons
    // the normal toolbar sizes are TOO large, e.g. for scaled stuff even more!
    const int iconSize = style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, this);
    verticalToolbar->setIconSize(QSize(iconSize, iconSize));

    /// TODO: disable actions if no results are displayed or selected

    QAction *action;

    QAction *refreshAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh), i18nc("@action:intoolbar", "Refresh"), this);
    verticalToolbar->addAction(refreshAction);
    connect(refreshAction, &QAction::triggered, this, &DataOutputWidget::slotRefresh);
    m_view->addAction(refreshAction);
    refreshAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    refreshAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(refreshAction);

    auto *resizeColumnsAction =
        new QAction(QIcon::fromTheme(QLatin1String("distribute-horizontal-x")), i18nc("@action:intoolbar", "Resize columns to contents"), this);
    verticalToolbar->addAction(resizeColumnsAction);
    connect(resizeColumnsAction, &QAction::triggered, this, &DataOutputWidget::resizeColumnsToContents);

    action = new QAction(QIcon::fromTheme(QLatin1String("distribute-vertical-y")), i18nc("@action:intoolbar", "Resize rows to contents"), this);
    verticalToolbar->addAction(action);
    connect(action, &QAction::triggered, this, &DataOutputWidget::resizeRowsToContents);

    action = new QAction(QIcon::fromTheme(QLatin1String("document-export-table")), i18nc("@action:intoolbar", "Export..."), this);
    verticalToolbar->addAction(action);
    m_view->addAction(action);
    connect(action, &QAction::triggered, this, &DataOutputWidget::slotExport);

    action = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy), i18nc("@action:intoolbar", "Copy"), this);
    verticalToolbar->insertAction(resizeColumnsAction, action);
    m_view->addAction(action);
    connect(action, &QAction::triggered, this, &DataOutputWidget::slotCopySelected);
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(action);

    action = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditClear), i18nc("@action:intoolbar", "Clear"), this);
    verticalToolbar->addAction(action);
    connect(action, &QAction::triggered, this, &DataOutputWidget::clearResults);

    verticalToolbar->addSeparator();

    auto *toggleAction = new KToggleAction(QIcon::fromTheme(QIcon::ThemeIcon::FormatTextDirectionRtl), i18nc("@action:intoolbar", "Use system locale"), this);
    verticalToolbar->addAction(toggleAction);
    connect(toggleAction, &QAction::triggered, this, &DataOutputWidget::slotToggleLocale);

    // === Section for Editable Table (only visible for editable tables) ===
    m_editableSection = new QWidget(this);
    auto *editableLayout = new QHBoxLayout(m_editableSection);
    editableLayout->setContentsMargins(0, 0, 0, 0);

    auto *editableToolbar = new KToolBar(m_editableSection);
    editableToolbar->setOrientation(Qt::Horizontal);
    editableToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    editableToolbar->setIconSize(QSize(iconSize, iconSize));

    QAction *pasteAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditPaste), i18nc("@action:intoolbar", "Paste"), this);
    m_editableOnlyRightClickActions.push_back(pasteAction);
    editableToolbar->addAction(pasteAction);

    connect(pasteAction, &QAction::triggered, this, &DataOutputWidget::slotPaste);
    pasteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
    pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(pasteAction);

    QAction *setNullAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentRevert), i18nc("@action:intoolbar", "Set Null"), this);
    m_editableOnlyRightClickActions.push_back(setNullAction);
    editableToolbar->addAction(setNullAction);
    connect(setNullAction, &QAction::triggered, this, &DataOutputWidget::slotSetNull);
    setNullAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X));
    setNullAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(setNullAction);

    QAction *insertRowAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd), i18nc("@action:intoolbar", "Insert Row"), this);
    m_editableOnlyRightClickActions.push_back(insertRowAction);
    editableToolbar->addAction(insertRowAction);
    connect(insertRowAction, &QAction::triggered, this, &DataOutputWidget::slotInsertRow);
    insertRowAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Insert));
    insertRowAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(insertRowAction);

    QAction *removeRowAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::ListRemove), i18nc("@action:intoolbar", "Remove Selected Rows"), this);
    m_editableOnlyRightClickActions.push_back(removeRowAction);
    editableToolbar->addAction(removeRowAction);
    connect(removeRowAction, &QAction::triggered, this, &DataOutputWidget::slotRemoveSelectedRows);
    removeRowAction->setShortcut(QKeySequence(Qt::Key_Delete));
    removeRowAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(removeRowAction);

    QAction *duplicateRowAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy), i18nc("@action:intoolbar", "Duplicate Selected Rows"), this);
    m_editableOnlyRightClickActions.push_back(duplicateRowAction);
    editableToolbar->addAction(duplicateRowAction);
    connect(duplicateRowAction, &QAction::triggered, this, &DataOutputWidget::slotDuplicateRows);
    duplicateRowAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    duplicateRowAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(duplicateRowAction);

    QAction *undoAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::EditUndo), i18nc("@action:intoolbar", "Undo"), this);
    m_editableOnlyRightClickActions.push_back(undoAction);
    editableToolbar->addAction(undoAction);
    connect(undoAction, &QAction::triggered, this, &DataOutputWidget::slotUndo);
    undoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    undoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(undoAction);

    QAction *saveAction = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave), i18nc("@action:intoolbar", "Save"), this);
    editableToolbar->addAction(saveAction);
    connect(saveAction, &QAction::triggered, this, &DataOutputWidget::slotSave);
    saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    saveAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(saveAction);

    // Separator between sections
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    editableToolbar->addWidget(separator);

    auto *whereLabel = new QLabel(i18nc("@label", "Where:"), this);
    QPalette palette = whereLabel->palette();
    palette.setColor(QPalette::WindowText, palette.color(QPalette::Highlight));
    whereLabel->setPalette(palette);
    whereLabel->setContentsMargins(QMargins(16, 0, 8, 0));
    editableToolbar->addWidget(whereLabel);

    m_filterInput = new QLineEdit(this);
    m_filterInput->setPlaceholderText(i18nc("@info:placeholder", "id > 0"));
    m_filterInput->setToolTip(i18nc("@info:tooltip", "Enter SQL WHERE clause to filter data"));
    m_filterInput->setClearButtonEnabled(true);
    m_filterInput->setMaximumWidth(250);
    editableToolbar->addWidget(m_filterInput);
    connect(m_filterInput, &QLineEdit::returnPressed, this, &DataOutputWidget::slotSetFilter);

    editableLayout->addWidget(editableToolbar);
    m_editableSection->setLayout(editableLayout);
    m_editableSection->setHidden(true); // Hidden by default

    // Add horizontal toolbar container above the view
    m_dataLayout->addWidget(m_editableSection);
    m_dataLayout->addWidget(m_view);

    layout->addWidget(verticalToolbar);
    layout->addLayout(m_dataLayout);
    layout->setContentsMargins(0, 0, 0, 0);

    setLayout(layout);
}

DataOutputWidget::~DataOutputWidget() = default;

void DataOutputWidget::showQueryResultSets(QSqlQuery &query)
{
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
    auto *tableModel = qobject_cast<QSqlTableModel *>(m_model->asQObject());
    if (tableModel) {
        delete m_model;
        m_model = nullptr;
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
    // Hide editable section for non-editable query results
    m_editableSection->setHidden(!m_isEditable);

    QTimer::singleShot(0, this, &DataOutputWidget::resizeColumnsToContents);

    raise();
}

void DataOutputWidget::showEditableTable(DataOutputModelInterface *model)
{
    if (!model) {
        return;
    }

    m_view->addActions(m_editableOnlyRightClickActions);

    if (m_model != nullptr) {
        // Delete current model
        delete m_model;
    }

    m_model = model;
    abstractModel()->setParent(this);

    // Set the shared style helper and autocompleter
    auto *editableModel = qobject_cast<DataOutputEditableModel *>(m_model->asQObject());
    if (editableModel) {
        editableModel->setStyleHelper(&m_styleHelper);
        editableModel->setEditStrategy(DataOutputEditableModel::EditStrategy::OnManualSubmit);

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

    m_view->setModel(static_cast<QAbstractItemModel *>(m_model->asQObject()));
    m_view->setSortingEnabled(true);

    m_isEditable = true;
    // Show editable section for editable tables
    m_editableSection->setHidden(!m_isEditable);

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
    m_editableSection->setHidden(true);
    m_view->setSortingEnabled(false);

    /// HACK needed to refresh headers. please correct if there's a better way
    m_view->horizontalHeader()->hide();
    m_view->verticalHeader()->hide();

    m_view->horizontalHeader()->show();
    m_view->verticalHeader()->show();
}

void DataOutputWidget::readConfig()
{
    m_styleHelper.readConfig();
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

#include "moc_dataoutputwidget.cpp"
