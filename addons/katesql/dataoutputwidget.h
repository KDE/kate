/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "dataoutputmodelinterface.h"
#include "exportwizard.h"
#include "dataoutputstylehelper.h"

#include <QAction>
#include <QList>
#include <QtAssert>
#include <QAbstractItemModel>
#include <QWidget>

class QAbstractItemModel;
class QTextStream;
class QVBoxLayout;
class QSqlQuery;
class DataOutputView;
class QLineEdit;

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

    DataOutputWidget(QWidget *parent);
    ~DataOutputWidget() override;

    void exportData(QTextStream &stream,
                    const QChar stringsQuoteChar = defaultExportValues.quoteStringCharForCopyPaste,
                    const QChar numbersQuoteChar = defaultExportValues.quoteNumbersCharForCopyPaste,
                    const QString fieldDelimiter = defaultExportValues.fieldDelimiterForCopyPaste,
                    const Options opt = NoOptions);

    void importData(QTextStream &stream,
                    const QChar stringsQuoteChar = defaultExportValues.quoteStringCharForCopyPaste,
                    const QString &fieldDelimiter = QString(defaultExportValues.fieldDelimiterForCopyPaste),
                    const QString &lineDelimiter = QString(defaultExportValues.lineDelimiterForCopyPaste));

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
    void showQueryResultSets(QSqlQuery &query);
    void showEditableTable(DataOutputModelInterface *model);
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

private:

    QAbstractItemModel *abstractModel() const
    {
        Q_ASSERT(m_model != nullptr);
        return qobject_cast<QAbstractItemModel *>(m_model->asQObject());
    }
    DataOutputStyleHelper m_styleHelper;
    QVBoxLayout *m_dataLayout;

    DataOutputModelInterface *m_model;
    DataOutputView *m_view;
    QWidget *m_editableSection;
    QLineEdit *m_filterInput;

    bool m_isEditable;

    QList<QAction *> m_editableOnlyRightClickActions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DataOutputWidget::Options)
