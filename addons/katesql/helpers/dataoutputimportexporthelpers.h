/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/
#pragma once

#include "katesqlconstants.h"

#include <QAbstractItemModel>
#include <QList>
#include <QSqlTableModel>
#include <qcontainerfwd.h>

class DataOutputImportExportHelpers
{
public:
    enum Option {
        NoOptions = 0x0,
        ExportColumnNames = 0x1,
        ExportLineNumbers = 0x2
    };

    Q_DECLARE_FLAGS(Options, Option)

    void static exportData(const QAbstractItemModel *model,
                           const QModelIndexList selectedIndexes,
                           QTextStream &stream,
                           const QChar stringsQuoteChar = KateSQLConstants::Export::DefaultValues::QuoteStringCharForCopyPaste,
                           const QChar numbersQuoteChar = KateSQLConstants::Export::DefaultValues::QuoteNumbersCharForCopyPaste,
                           const QString fieldDelimiter = KateSQLConstants::Export::DefaultValues::FieldDelimiterForCopyPaste,
                           const Options opt = NoOptions);

    void static importData(QSqlTableModel *sqlModel,
                           QModelIndexList selectedIndexes,
                           QTextStream &stream,
                           const QChar stringsQuoteChar = KateSQLConstants::Export::DefaultValues::QuoteStringCharForCopyPaste,
                           const QString &fieldDelimiter = QString(KateSQLConstants::Export::DefaultValues::FieldDelimiterForCopyPaste),
                           const QString &lineDelimiter = QString(KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste));

    static QString escapeString(QString value, QChar stringsQuoteChar, QChar escapeChar);

private:
    QList<QVariantList> static parseStream(QTextStream &stream,
                                           const QChar stringsQuoteChar = KateSQLConstants::Export::DefaultValues::QuoteStringCharForCopyPaste,
                                           const QString &fieldDelimiter = QString(KateSQLConstants::Export::DefaultValues::FieldDelimiterForCopyPaste),
                                           const QString &lineDelimiter = QString(KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste));
    void static importData(QSqlTableModel *sqlModel, QModelIndexList selectedIndexes, QList<QVariantList> data);

    static bool inline isNumeric(const QVariant &value);

    struct UniqueSequences {
        QList<int> rows, cols;
    };
    static UniqueSequences inline getUniqueSequence(const QModelIndexList &selectedIndexes);
    static bool inline checkNoSkips(QList<int> &uniqueSequence);

    struct SelectedIndexesInfo {
        int firstSelectedRow = 0, firstSelectedColumn = 0;
        int lastSelectedRow = 0, lastSelectedColumn = 0;
        int rowSelectionHeight = 0, columnSelectionWidth = 0;
    };
    static SelectedIndexesInfo inline getSelectedIndexesInfo(const UniqueSequences &uniqueSequences);

    static QList<QModelIndexList>
    separateSelectionGroups(const QModelIndexList &selectedIndexes, const SelectedIndexesInfo &info, const int dataRows, const int dataCols);

    static void inline setData(QAbstractItemModel *model, const int row, const int col, const QVariant &value);
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DataOutputImportExportHelpers::Options)
