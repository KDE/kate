/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "dataoutput/dataoutputmodelinterface.h"
#include "helpers/dataoutputstylehelper.h"
#include "katesqlconstants.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QList>
#include <QMap>
#include <QWidget>
#include <QtAssert>

class QAbstractItemModel;
class QTextStream;
class QVBoxLayout;
class QSqlQuery;
class DataOutputView;
class QLineEdit;
class KActionCollection;
class KToolBar;

class DataOutputWidget : public QWidget
{
    Q_OBJECT

public:
    enum Option {
        NoOptions = 0x0,
        ExportColumnNames = 0x1,
        ExportLineNumbers = 0x2
    };

    Q_DECLARE_FLAGS(Options, Option)

    DataOutputWidget(QWidget *parent, KActionCollection *actionCollection);
    ~DataOutputWidget() override;

    void exportData(QTextStream &stream,
                    const QChar stringsQuoteChar = KateSQLConstants::Export::DefaultValues::QuoteStringCharForCopyPaste,
                    const QChar numbersQuoteChar = KateSQLConstants::Export::DefaultValues::QuoteNumbersCharForCopyPaste,
                    const QString fieldDelimiter = KateSQLConstants::Export::DefaultValues::FieldDelimiterForCopyPaste,
                    const Options opt = NoOptions);

    void importData(QTextStream &stream,
                    const QChar stringsQuoteChar = KateSQLConstants::Export::DefaultValues::QuoteStringCharForCopyPaste,
                    const QString &fieldDelimiter = QString(KateSQLConstants::Export::DefaultValues::FieldDelimiterForCopyPaste),
                    const QString &lineDelimiter = QString(KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste));

    DataOutputModelInterface *model() const
    {
        return m_model;
    }
    DataOutputView *view() const
    {
        return m_view;
    }
    DataOutputStyleHelper *styleHelper()
    {
        return &m_styleHelper;
    }

public Q_SLOTS:
    void showQueryResultSets(QSqlQuery &query, const QString &connectionName);
    void showEditableTable(DataOutputModelInterface *model, const QString &connectionName, const QMap<QString, QString> &tableToDisplayColumnMap);
    void resizeColumnsToContents();
    void resizeRowsToContents();
    void clearResults();
    void readConfig();

    void slotToggleLocale();
    void slotCopySelected();
    void slotPaste();
    void slotExport();
    void slotSave();
    void slotRefresh();
    void slotInsertRow();
    void slotDuplicateRows();
    void slotRemoveSelectedRows();
    void slotSetNull();
    void slotUndo();
    void slotSetFilter();
    void slotMakeColumnPresentable();
    void slotUpdateMakeColumnPresentableAction(const QPoint &pos, int column);

protected:
    void changeEvent(QEvent *event) override;

private:
    QAbstractItemModel *abstractModel() const
    {
        Q_ASSERT(m_model != nullptr);
        return qobject_cast<QAbstractItemModel *>(m_model->asQObject());
    }
    DataOutputStyleHelper m_styleHelper;
    QVBoxLayout *m_dataLayout;

    KActionCollection *m_actionCollection;

    DataOutputModelInterface *m_model;
    DataOutputView *m_view;
    QWidget *m_editableSection;
    QLineEdit *m_filterInput;
    KToolBar *m_verticalToolbar;
    KToolBar *m_editableToolbar;

    bool m_isEditable;
    bool m_relationalTablesEnabled = false;

    QList<QAction *> m_editableOnlyRightClickActions;
    QAction *m_makeColumnPresentableAction = nullptr;

    QString m_connectionName;
    int m_currentPresentableColumn = 0;
    QMap<QString, QString> m_tableToDisplayColumnMap;
    void adjustToEditableStateChange();
    void setupColumnDelegates();

Q_SIGNALS:
    void displayColumnMapChanged(const QString &tableName, const QString &columnName);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DataOutputWidget::Options)
