/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>
SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QVariant>

#include <string>

#include "../helpers/dataoutputimportexporthelpers.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

// Compile-time cell marker — used in consteval grid builders
constexpr const char *KeepMarker = "<keep>";
// QLatin1String alias for runtime QString comparisons (resolveExpected)
static const QLatin1String QKeepMarker(KeepMarker);

// Grid-encoding delimiters (internal to the test helpers)
static constexpr char GridFieldDelim = ',';
static constexpr char GridLineDelim = ';';

// Stream delimiters — used in consteval stream builders (tab / newline)
static constexpr char StreamFieldDelim = '\t';
static constexpr char StreamLineDelim = '\n';

// Runtime QString aliases for importData() calls
static const QString StreamFieldDelimStr = QString(QLatin1Char(StreamFieldDelim));
static const QString StreamLineDelimStr = QString(QLatin1Char(StreamLineDelim));
namespace Defaults = KateSQLConstants::Export::DefaultValues;
static const QChar StreamQuoteChar = Defaults::QuoteStringCharForCopyPaste;

// ─────────────────────────────────────────────────────────────────────────────
// Constexpr grid builders — produce std::string (compile-time when args are constants)
// ─────────────────────────────────────────────────────────────────────────────
//
// Usage:
//   grid(gridRow("A", "B", "C"),
//        gridRow("D", "E", "F"))
//   →  "A,B,C;D,E,F"  (std::string)
//
//   keepRow(3)  →  "<keep>,<keep>,<keep>"  (std::string)
//
//   streamGrid(streamRow("A", "B"),
//              streamRow("C", "D"))
//   →  "A\tB\nC\tD"  (std::string)
//
//   streamGrid("A", "B", "C")
//   →  "A\nB\nC"  (single-column stream data)
//
// All functions are constexpr — evaluated at compile time when arguments
// are constants. Wrap with QString::fromStdString(...) at the call site.
// ─────────────────────────────────────────────────────────────────────────────

template<typename... Args>
constexpr std::string gridRow(Args... cells)
{
    std::string result;
    bool first = true;
    for (const auto &cell : {cells...}) {
        if (!first)
            result += GridFieldDelim;
        result += cell;
        first = false;
    }
    return result;
}

constexpr std::string keepRow(int cols)
{
    std::string result;
    for (int i = 0; i < cols; ++i) {
        if (i > 0)
            result += GridFieldDelim;
        result += KeepMarker;
    }
    return result;
}

template<typename... Rows>
constexpr std::string grid(Rows... rows)
{
    std::string result;
    bool first = true;
    for (const auto &row : {rows...}) {
        if (!first)
            result += GridLineDelim;
        result += row;
        first = false;
    }
    return result;
}

template<typename... Args>
constexpr std::string streamRow(Args... cells)
{
    std::string result;
    bool first = true;
    for (const auto &cell : {cells...}) {
        if (!first)
            result += StreamFieldDelim;
        result += cell;
        first = false;
    }
    return result;
}

template<typename... Rows>
constexpr std::string streamGrid(Rows... rows)
{
    std::string result;
    bool first = true;
    for (const auto &row : {rows...}) {
        if (!first)
            result += StreamLineDelim;
        result += row;
        first = false;
    }
    return result;
}

// ─── Constexpr selection builders ────────────────────────────────────────────
//
// Usage:
//   sel(1, 2)                        →  "1,2"              (single cell)
//   sel(0, 0, 3, 3)                  →  "0,0;0,1;...;3,3"  (rectangular range, inclusive)
//   combine(sel(0,0), sel(2,3))      →  "0,0;2,3"          (non-contiguous)
//
// Wrap with QString::fromStdString(...) at the call site.
// ─────────────────────────────────────────────────────────────────────────────

constexpr std::string intToStr(unsigned int un)
{
    if (un == 0)
        return "0";
    char buf[12] = {};
    int pos = 11;
    while (un > 0) {
        buf[--pos] = static_cast<char>('0' + un % 10);
        un /= 10;
    }
    return std::string(&buf[pos]);
}

constexpr std::string sel(int row, int col)
{
    return intToStr(row) + GridFieldDelim + intToStr(col);
}

constexpr std::string sel(int startRow, int startCol, int endRow, int endCol)
{
    std::string result;
    for (int r = startRow; r <= endRow; ++r) {
        for (int c = startCol; c <= endCol; ++c) {
            if (!result.empty())
                result += GridLineDelim;
            result += intToStr(r);
            result += GridFieldDelim;
            result += intToStr(c);
        }
    }
    return result;
}

template<typename... Sels>
constexpr std::string combine(Sels... sels)
{
    std::string result;
    bool first = true;
    for (const auto &s : {sels...}) {
        if (!first)
            result += GridLineDelim;
        result += s;
        first = false;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Runtime helpers — grid parsing / resolving
// ─────────────────────────────────────────────────────────────────────────────

// Build the default R_C grid string for `rows` × `cols`.
static QString defaultGrid(int rows, int cols)
{
    QStringList rowStrings;
    rowStrings.reserve(rows);
    for (int r = 0; r < rows; ++r) {
        QStringList cells;
        cells.reserve(cols);
        for (int c = 0; c < cols; ++c) {
            cells << QString::fromLatin1("%1_%2").arg(r).arg(c);
        }
        rowStrings << cells.join(QLatin1Char(GridFieldDelim));
    }
    return rowStrings.join(QLatin1Char(GridLineDelim));
}

// Parse "r,c;r,c;..." into a list of (row,col) pairs.
static QList<QPair<int, int>> parseSelection(const QString &selection)
{
    QList<QPair<int, int>> result;
    const auto parts = selection.split(QLatin1Char(GridLineDelim));
    for (const QString &part : parts) {
        const auto rc = part.split(QLatin1Char(GridFieldDelim));
        Q_ASSERT(rc.size() == 2);
        result.append({rc[0].toInt(), rc[1].toInt()});
    }
    return result;
}

// Parse a grid string into a 2-D list of cell values.
static QList<QStringList> parseGrid(const QString &g)
{
    QList<QStringList> result;
    const auto rows = g.split(QLatin1Char(GridLineDelim));
    for (const QString &row : rows) {
        result.append(row.split(QLatin1Char(GridFieldDelim)));
    }
    return result;
}

// Unique connection name counter — prevents QSqlDatabase warnings when
// multiple test cases run in the same process.
static int s_connectionCounter = 0;

// ─────────────────────────────────────────────────────────────────────────────
// TestContext — in-memory SQLite table pre-populated from a grid string
// ─────────────────────────────────────────────────────────────────────────────

struct TestContext {
    QSqlDatabase db;
    QSqlTableModel *model = nullptr;

    // Creates an in-memory SQLite table named "test_table" with `cols` TEXT
    // columns and `rows` rows pre-populated from `initialGrid`.
    //
    // If `initialGrid` is empty the default "R_C" grid is used.
    TestContext(int rows, int cols, const QString &initialGrid = {})
    {
        const QString connName = QString::fromLatin1("importdata_test_%1").arg(s_connectionCounter++);

        db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(QStringLiteral(":memory:"));
        if (!db.open()) {
            qWarning("Failed to open in-memory database: %s", qPrintable(db.lastError().text()));
            QFAIL("Could not open in-memory SQLite database");
        }

        // Build CREATE TABLE statement
        QStringList colDefs;
        colDefs.reserve(cols);
        for (int c = 0; c < cols; ++c) {
            colDefs << QString::fromLatin1("col%1 TEXT").arg(c);
        }
        QString createSql = QString::fromLatin1("CREATE TABLE test_table (%1)").arg(colDefs.join(QLatin1Char(',')));
        QSqlQuery q(db);
        if (!q.exec(createSql)) {
            qWarning("CREATE TABLE failed: %s", qPrintable(q.lastError().text()));
            QFAIL("CREATE TABLE failed");
        }

        // Resolve initial values
        QString g = initialGrid.isEmpty() ? defaultGrid(rows, cols) : initialGrid;
        const auto parsedGrid = parseGrid(g);

        // Insert rows
        for (int r = 0; r < rows; ++r) {
            QStringList placeholders;
            QStringList values;
            placeholders.reserve(cols);
            values.reserve(cols);
            for (int c = 0; c < cols; ++c) {
                placeholders << QString::fromLatin1("?");
                values << (r < parsedGrid.size() && c < parsedGrid[r].size() ? parsedGrid[r][c] : QString());
            }
            QString insertSql = QString::fromLatin1("INSERT INTO test_table VALUES (%1)").arg(placeholders.join(QLatin1Char(',')));
            QSqlQuery q(db);
            q.prepare(insertSql);
            for (const QString &v : values) {
                q.addBindValue(v);
            }
            if (!q.exec()) {
                qWarning("INSERT failed: %s", qPrintable(q.lastError().text()));
                QFAIL("INSERT failed");
            }
        }

        model = new QSqlTableModel(nullptr, db);
        model->setTable(QStringLiteral("test_table"));
        model->setEditStrategy(QSqlTableModel::OnFieldChange);
        if (!model->select()) {
            qWarning("model->select() failed: %s", qPrintable(model->lastError().text()));
            QFAIL("QSqlTableModel::select() failed");
        }
    }

    ~TestContext()
    {
        const QString connName = db.connectionName();
        delete model; // model must be deleted before db is removed
        db = QSqlDatabase(); // release reference
        QSqlDatabase::removeDatabase(connName);
    }

    // Build a QModelIndexList from "r,c;r,c;..." string.
    QModelIndexList buildSelection(const QString &selection) const
    {
        QModelIndexList result;
        const auto pairs = parseSelection(selection);
        for (const auto &[r, c] : pairs) {
            QModelIndex idx = model->index(r, c);
            Q_ASSERT(idx.isValid());
            result.append(idx);
        }
        return result;
    }

    // Read all model cells and return as a grid string.
    QString readGrid() const
    {
        const int rows = model->rowCount();
        const int cols = model->columnCount();
        QStringList rowStrings;
        rowStrings.reserve(rows);
        for (int r = 0; r < rows; ++r) {
            QStringList cells;
            cells.reserve(cols);
            for (int c = 0; c < cols; ++c) {
                cells << model->index(r, c).data().toString();
            }
            rowStrings << cells.join(QLatin1Char(GridFieldDelim));
        }
        return rowStrings.join(QLatin1Char(GridLineDelim));
    }
};
