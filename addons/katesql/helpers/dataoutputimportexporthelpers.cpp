/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#include "dataoutputimportexporthelpers.h"
#include "katesqlconstants.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLatin1StringView>
#include <QList>
#include <QModelIndex>
#include <QSqlTableModel>
#include <QVariant>
#include <algorithm>

// During the import we assume that the data array is an array with the same number of columns in each row
// so if selection is not contiguous here we are adding empty values as highlighted by delimiters
void DataOutputImportExportHelpers::exportData(const QAbstractItemModel *model,
                                               const QModelIndexList selectedIndexes,
                                               QTextStream &stream,
                                               const QChar stringsQuoteChar,
                                               const QChar numbersQuoteChar,
                                               const QString fieldDelimiter,
                                               const Options opt)
{
    std::vector<int> columns;
    std::vector<int> rows;
    QHash<QPair<int, int>, QString> snapshot;

    snapshot.reserve(selectedIndexes.count());

    for (const QModelIndex &index : selectedIndexes) {
        const QVariant indexData = index.data(Qt::UserRole);

        const int col = index.column();
        const int row = index.row();

        columns.push_back(col);
        rows.push_back(row);

        if (isNumeric(indexData) || indexData.toString() == KateSQLConstants::NullDisplayString) // is numeric or boolean
        {
            const QString data = escapeString(indexData.toString(), numbersQuoteChar, EscapeChar);
            if (numbersQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar) {
                snapshot[qMakePair(row, col)] = numbersQuoteChar + data + numbersQuoteChar;
            } else {
                snapshot[qMakePair(row, col)] = data;
            }
        } else {
            const QString data = escapeString(indexData.toString(), stringsQuoteChar, EscapeChar);
            if (stringsQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar) {
                snapshot[qMakePair(row, col)] = stringsQuoteChar + data + stringsQuoteChar;
            } else {
                snapshot[qMakePair(row, col)] = data;
            }
        }
    }

    // uniquify
    std::ranges::sort(rows);
    std::ranges::sort(columns);
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    columns.erase(std::unique(columns.begin(), columns.end()), columns.end());

    const int minRow = rows.front();
    const int maxRow = rows.back();
    const int minCol = columns.front();
    const int maxCol = columns.back();

    if (opt.testFlag(ExportColumnNames)) {
        if (opt.testFlag(ExportLineNumbers)) {
            stream << fieldDelimiter;
        }
        for (int col = minCol; col <= maxCol; ++col) {
            if (std::binary_search(columns.begin(), columns.end(), col)) {
                const QVariant headerData = model->headerData(col, Qt::Horizontal);

                if (stringsQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar) {
                    const QString escapedHeader = escapeString(headerData.toString(), stringsQuoteChar, EscapeChar);
                    stream << stringsQuoteChar + escapedHeader + stringsQuoteChar;
                } else {
                    stream << headerData.toString();
                }
            }

            if (col < maxCol) {
                stream << fieldDelimiter;
            }
        }
        stream << KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste;
    }

    for (int row = minRow; row <= maxRow; ++row) {
        if (std::binary_search(rows.begin(), rows.end(), row)) {
            if (opt.testFlag(ExportLineNumbers)) {
                stream << row + 1 << fieldDelimiter;
            }

            for (int col = minCol; col <= maxCol; ++col) {
                stream << snapshot.value(qMakePair(row, col));

                if (col < maxCol) {
                    stream << fieldDelimiter;
                }
            }
        }
        stream << KateSQLConstants::Export::DefaultValues::LineDelimiterForCopyPaste;
    }
}

void DataOutputImportExportHelpers::importData(QSqlTableModel *sqlModel,
                                               QModelIndexList selectedIndexes,
                                               QTextStream &stream,
                                               const QChar stringsQuoteChar,
                                               const QString &fieldDelimiter,
                                               const QString &lineDelimiter)
{
    if (sqlModel == nullptr) {
        return;
    }

    if (selectedIndexes.isEmpty()) {
        return;
    }

    QList<QVariantList> dataArray = parseStream(stream, stringsQuoteChar, fieldDelimiter, lineDelimiter);

    importData(sqlModel, selectedIndexes, dataArray);
}

QList<QVariantList>
DataOutputImportExportHelpers::parseStream(QTextStream &stream, const QChar stringsQuoteChar, const QString &fieldDelimiter, const QString &lineDelimiter)
{
    const QString data = stream.readAll().trimmed();
    QStringList lines = data.split(lineDelimiter, Qt::KeepEmptyParts);

    QList<QList<QVariant>> dataArray(lines.size(), QList<QVariant>());
    for (int lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        QString line = lines[lineIdx];

        if (line.isEmpty()) {
            continue;
        }

        // Parse the line - handle quoted fields
        int pos = 0;
        while (pos < line.length()) {
            QString field;

            bool fieldValueHasNoQuotes = true;
            // Check if field starts with quote
            if (stringsQuoteChar != KateSQLConstants::Export::DefaultValues::NoQuotingChar && pos < line.length() && line[pos] == stringsQuoteChar) {
                pos++; // Skip opening quote
                // Read until an unescaped closing quote
                while (pos < line.length()) {
                    if (line[pos] == EscapeChar && pos + 1 < line.length()) {
                        // Escaped character: map known escape sequences back,
                        // or take the character after the backslash literally
                        const QChar nextChar = line[pos + 1];
                        if (nextChar == u'n')
                            field += u'\n';
                        else if (nextChar == u'r')
                            field += u'\r';
                        else if (nextChar == u't')
                            field += u'\t';
                        else
                            field += nextChar;
                        pos += 2;
                    } else if (line[pos] == stringsQuoteChar) {
                        fieldValueHasNoQuotes = true;
                        // Unescaped quote -> closing quote
                        break;
                    } else {
                        field += line[pos];
                        pos++;
                    }
                }
                if (pos < line.length()) {
                    pos++; // Skip closing quote
                }
                // Skip delimiter if present (no allocation — QStringView comparison)
                if (QStringView(line).sliced(pos).startsWith(fieldDelimiter)) {
                    pos += fieldDelimiter.length();
                }
            } else {
                // Read until delimiter or end, handling escapes
                while (pos < line.length()) {
                    if (line[pos] == EscapeChar && pos + 1 < line.length()) {
                        // Escaped character: map known escape sequences back,
                        // or take the character after the backslash literally
                        const QChar nextChar = line[pos + 1];
                        if (nextChar == u'n')
                            field += u'\n';
                        else if (nextChar == u'r')
                            field += u'\r';
                        else if (nextChar == u't')
                            field += u'\t';
                        else
                            field += nextChar;
                        pos += 2;
                    } else if (QStringView(line).sliced(pos).startsWith(fieldDelimiter)) {
                        break;
                    } else {
                        field += line[pos];
                        pos++;
                    }
                }
                // Skip delimiter
                if (pos < line.length()) {
                    pos += fieldDelimiter.length();
                }
            }

            // A truly empty field (nothing between delimiters) is stored as a
            // null QVariant so that setData can be skipped for it during import.
            // Quoted empty strings (e.g. "") are *not* null — they are intentional.
            // the actual null values from sql are represented as KateSQLConstants::NullDisplayString

            dataArray[lineIdx].append(field.isEmpty() && fieldValueHasNoQuotes ? QVariant() : QVariant(field));
        }

        // If the line ends with the field delimiter, the loop above advanced past
        // the trailing delimiter without appending the (empty) final field.
        if (!line.isEmpty() && line.endsWith(fieldDelimiter)) {
            dataArray[lineIdx].append(QVariant()); // Trailing delimiter → truly empty field
        }
    }
    return dataArray;
}

// recursive version that analyzes the selected indexes and data array to determine how to better import it
//  May import one value into multiple contiguous and non-contiguous cells
//  may import multiple values into multiple contiguous and non-contiguous cells as groups
//  may expand indexes to fit the data array or wrap data array to paste into every selected index
// Built on the assumtion that we are recieving rows x cols data array. PLEASE, DO NOT BREAK THIS ASSUMPTION
void DataOutputImportExportHelpers::importData(QSqlTableModel *sqlModel, QModelIndexList selectedIndexes, QList<QVariantList> dataArray)
{
    if (sqlModel == nullptr) {
        return;
    }

    if (selectedIndexes.isEmpty()) {
        return;
    }

    if (dataArray.isEmpty()) {
        return;
    }
    // Sort to get the top-left position
    std::ranges::sort(selectedIndexes, [](const QModelIndex &a, const QModelIndex &b) {
        if (a.row() != b.row())
            return a.row() < b.row();
        return a.column() < b.column();
    });

    bool everyRowEmpty = std::all_of(dataArray.begin(), dataArray.end(), [](const QList<QVariant> &row) {
        return row.isEmpty();
    });
    if (everyRowEmpty) {
        return;
    }

    const int dataArrayRows = dataArray.size();
    const int dataArrayCols = dataArray[0].size();

    UniqueSequences uniqueSequences = getUniqueSequence(selectedIndexes);

    const SelectedIndexesInfo info = getSelectedIndexesInfo(uniqueSequences);

    const bool checkNoSkipsCols = checkNoSkips(uniqueSequences.cols);
    const bool checkNoSkipsRows = checkNoSkips(uniqueSequences.rows);

    const bool anySkipsDetected = !checkNoSkipsCols || !checkNoSkipsRows;

    if (anySkipsDetected) {
        QList<QModelIndexList> groups = separateSelectionGroups(selectedIndexes, info, dataArrayRows, dataArrayCols);

        if (!groups.isEmpty()) {
            for (const QModelIndexList &group : std::as_const(groups)) {
                importData(sqlModel, group, dataArray);
            }
            return;
        }
    }

    // wrapping the data array to fit into the selected indexes
    for (int row = info.firstSelectedRow; row < std::max(info.lastSelectedRow + 1, dataArrayRows + info.firstSelectedRow); ++row) {
        const int dataRow = (row - info.firstSelectedRow) % dataArrayRows;

        // An empty data row (produced by an empty line in the pasted text)
        // means this row gap should be preserved — skip setData entirely.
        if (dataArray[dataRow].isEmpty()) {
            continue;
        }

        const int rowCols = dataArray[dataRow].size();
        const int endCol = std::max(info.lastSelectedColumn + 1, info.firstSelectedColumn + rowCols);

        for (int col = info.firstSelectedColumn; col < endCol; ++col) {
            const int dataCol = (col - info.firstSelectedColumn) % rowCols;
            const QVariant value = dataArray[dataRow][dataCol];

            setData(sqlModel, row, col, value);
        }
    }
}

bool inline DataOutputImportExportHelpers::isNumeric(const QVariant &value)
{
    if (value.isNull()) {
        return true;
    }
    switch (value.typeId()) {
    case QMetaType::Type::Bool:
    case QMetaType::Type::Short:
    case QMetaType::Type::Int:
    case QMetaType::Type::UInt:
    case QMetaType::Type::Long:
    case QMetaType::Type::LongLong:
    case QMetaType::Type::ULongLong:
    case QMetaType::Type::Float16:
    case QMetaType::Type::Float:
    case QMetaType::Type::Double:
        return true;
    case QMetaType::Type::QByteArray:
    case QMetaType::Type::QDate:
    case QMetaType::Type::QTime:
    case QMetaType::Type::QDateTime:
    case QMetaType::Type::QString:
    default:
        return false;
    }
}

QString DataOutputImportExportHelpers::escapeString(QString value, QChar stringsQuoteChar, QChar EscapeChar)
{
    // Escape the escape character itself first, then the quote char.
    // Order matters: if we escape quotes first, a literal backslash
    // before a quote would produce \" which the reader would
    // interpret as an escaped quote rather than \\".
    return value.replace(EscapeChar, EscapeChar + EscapeChar)
        .replace(stringsQuoteChar, EscapeChar + stringsQuoteChar)
        .replace(u'\r', EscapeChar + u'r')
        .replace(u'\n', EscapeChar + u'n')
        .replace(u'\t', EscapeChar + u't');
}

DataOutputImportExportHelpers::SelectedIndexesInfo inline DataOutputImportExportHelpers::getSelectedIndexesInfo(const UniqueSequences &uniqueSequences)
{
    const int firstSelectedRow = std::ranges::min(uniqueSequences.rows);
    const int firstSelectedColumn = std::ranges::min(uniqueSequences.cols);

    const int lastSelectedRow = std::ranges::max(uniqueSequences.rows);
    const int lastselectedColumn = std::ranges::max(uniqueSequences.cols);

    const int totalSelectedRows = lastSelectedRow - firstSelectedRow + 1;
    const int totalSelectedColumns = lastselectedColumn - firstSelectedColumn + 1;

    return {firstSelectedRow, firstSelectedColumn, lastSelectedRow, lastselectedColumn, totalSelectedRows, totalSelectedColumns};
}

DataOutputImportExportHelpers::UniqueSequences inline DataOutputImportExportHelpers::getUniqueSequence(const QModelIndexList &selectedIndexes)
{
    DataOutputImportExportHelpers::UniqueSequences uniqueSequences;
    for (const QModelIndex &index : selectedIndexes) {
        if (!uniqueSequences.rows.contains(index.row())) {
            uniqueSequences.rows.append(index.row());
        }
        if (!uniqueSequences.cols.contains(index.column())) {
            uniqueSequences.cols.append(index.column());
        }
    }
    return uniqueSequences;
}

bool inline DataOutputImportExportHelpers::checkNoSkips(QList<int> &uniqueSequence)
{
    std::ranges::sort(uniqueSequence);

    for (int row = 0; row < uniqueSequence.size(); ++row) {
        if (row == 0) {
            continue;
        }
        const int currentRow = uniqueSequence[row];
        const int previousRow = uniqueSequence[row - 1];
        if (currentRow - 1 != previousRow) {
            return false;
        }
    }
    return true;
}

QList<QModelIndexList> DataOutputImportExportHelpers::separateSelectionGroups(const QModelIndexList &selectedIndexes,
                                                                              const SelectedIndexesInfo &info,
                                                                              const int dataRows,
                                                                              const int dataCols)
{
    if (dataRows == 0 || dataCols == 0) {
        return {};
    }

    if (dataRows >= info.rowSelectionHeight && dataCols >= info.columnSelectionWidth) {
        return {};
    }

    // first I'm separating all selected indexes that fit into the rectangle info.rowsSelecionWidth X info.columnsSelectionHeight
    //  Then creating more groups looking beyond the borders of the first group and making the first index - a beginning of the new rectangle group
    //  Then repeat until all selected indexes are grouped

    QList<QModelIndexList> result;
    QModelIndexList remainingIndexes = selectedIndexes;
    while (!remainingIndexes.isEmpty()) {
        QModelIndexList group;

        const QModelIndex &firstIndex = remainingIndexes.first();
        group.append(firstIndex);
        const int startRow = firstIndex.row(), startCol = firstIndex.column();
        const int endRow = startRow + dataRows, endCol = startCol + dataCols;
        remainingIndexes.removeFirst();

        for (int i = 0; i < remainingIndexes.size(); ++i) {
            const QModelIndex &index = remainingIndexes[i];
            if (index.row() < endRow && index.column() < endCol) {
                group.append(index);
                remainingIndexes.removeAt(i);
                --i;
            }
        }
        result.append(group);
    }

    return result;
}

void inline DataOutputImportExportHelpers::setData(QAbstractItemModel *model, const int row, const int col, const QVariant &value)
{
    // A null QVariant means the field was truly empty (nothing between
    // delimiters) — skip setData so the existing cell value is preserved.
    // the actual null values from sql are represented as KateSQLConstants::NullDisplayString
    if (value.isNull()) {
        return;
    }

    if (row > model->rowCount() - 1) {
        return;
    }

    if (col > model->columnCount() - 1) {
        return;
    }
    const QModelIndex index = model->index(row, col);

    if (!index.isValid()) {
        return;
    }

    if (!model->setData(index, value)) {
        qWarning("Failed to set data at row %d, column %d", row, col);
    }
}
