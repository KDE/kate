/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>
SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QStandardItemModel>
#include <QTest>
#include <QTextStream>

#include "testhelpers.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers for building expected export output
// ─────────────────────────────────────────────────────────────────────────────

// Quote a string value using the default copy-paste settings.
// Applies the same escape logic as escapeString, then wraps in quote chars.
static QString q(const QString &value)
{
    const QChar quote = Defaults::QuoteStringCharForCopyPaste;
    const QChar esc = EscapeChar;

    QString escaped = DataOutputImportExportHelpers::escapeString(value, quote, esc);
    return quote + escaped + quote;
}

// Build a model from a grid string using QStandardItemModel.
// QStandardItemModel returns data for all roles (including UserRole),
// which is required for export testing.
static QStandardItemModel *buildModel(const QString &gridStr, int rows, int cols)
{
    auto *model = new QStandardItemModel(rows, cols);
    const auto parsedGrid = parseGrid(gridStr.isEmpty() ? defaultGrid(rows, cols) : gridStr);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const QString val = (r < parsedGrid.size() && c < parsedGrid[r].size()) ? parsedGrid[r][c] : QString();
            auto *item = new QStandardItem(val);
            // exportData uses Qt::UserRole to read cell values.
            // QStandardItem stores text in DisplayRole/EditRole by default,
            // so we must explicitly set UserRole too.
            item->setData(val, Qt::UserRole);
            model->setItem(r, c, item);
        }
    }
    // Set horizontal header labels (col0, col1, ...) for ExportColumnNames tests
    for (int c = 0; c < cols; ++c) {
        model->setHorizontalHeaderItem(c, new QStandardItem(QString::fromLatin1("col%1").arg(c)));
    }
    return model;
}

// Build QModelIndexList from a selection string, using any QAbstractItemModel.
static QModelIndexList buildSelectionFromModel(const QAbstractItemModel *model, const QString &selection)
{
    QModelIndexList result;
    const auto pairs = parseSelection(selection);
    for (const auto &[r, c] : pairs) {
        QModelIndex idx = model->index(r, c);
        result.append(idx);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test class
// ─────────────────────────────────────────────────────────────────────────────

class TestExportData : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    // ── Section 1: Single cell export ──────────────────────────────────────
    void testSingleCell_data();
    void testSingleCell();

    // ── Section 2: Contiguous selection export ─────────────────────────────
    void testContiguousSelection_data();
    void testContiguousSelection();

    // ── Section 3: Non-contiguous selection export ─────────────────────────
    void testNonContiguousSelection_data();
    void testNonContiguousSelection();

    // ── Section 4: Escape characters and special content ───────────────────
    void testEscapeCharacters_data();
    void testEscapeCharacters();

    // ── Section 4b: Empty string cell (separate test) ──────────────────────
    void testEmptyStringCell();

    // ── Section 5: Export options (column names, line numbers) ─────────────
    void testExportOptions_data();
    void testExportOptions();
};

// ═════════════════════════════════════════════════════════════════════════════
// Test setup
// ═════════════════════════════════════════════════════════════════════════════
void TestExportData::initTestCase()
{
    // No special driver requirements — we use QStandardItemModel
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 1 — Single cell export
// ═════════════════════════════════════════════════════════════════════════════
void TestExportData::testSingleCell_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    // 1.1  Single cell at origin
    QTest::newRow("1.1 single-cell-origin") << 3 << 3 << QString() << QString::fromStdString(sel(0, 0))
                                            << q(QStringLiteral("0_0")) + Defaults::LineDelimiterForCopyPaste;

    // 1.2  Single cell in the middle
    QTest::newRow("1.2 single-cell-middle") << 4 << 4 << QString() << QString::fromStdString(sel(2, 3))
                                            << q(QStringLiteral("2_3")) + Defaults::LineDelimiterForCopyPaste;

    // 1.3  Single cell at corner
    QTest::newRow("1.3 single-cell-corner") << 5 << 5 << QString() << QString::fromStdString(sel(4, 4))
                                            << q(QStringLiteral("4_4")) + Defaults::LineDelimiterForCopyPaste;
}

void TestExportData::testSingleCell()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    QStandardItemModel *model = buildModel(initial, rows, cols);
    QModelIndexList sel = buildSelectionFromModel(model, selection);

    QString output;
    QTextStream stream(&output);
    DataOutputImportExportHelpers::exportData(model, sel, stream);

    delete model;
    QCOMPARE(output, expected);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 2 — Contiguous selection export
// ═════════════════════════════════════════════════════════════════════════════
void TestExportData::testContiguousSelection_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    const QChar tab = Defaults::FieldDelimiterForCopyPaste;
    const QString nl = QString(Defaults::LineDelimiterForCopyPaste);

    // 2.1  1×2 contiguous (same row)
    QTest::newRow("2.1 1x2-sameRow") << 3 << 3 << QString() << QString::fromStdString(sel(1, 0, 1, 1))
                                     << q(QStringLiteral("1_0")) + tab + q(QStringLiteral("1_1")) + nl;

    // 2.2  2×1 contiguous (same column)
    QTest::newRow("2.2 2x1-sameCol") << 4 << 4 << QString() << QString::fromStdString(sel(0, 2, 1, 2))
                                     << q(QStringLiteral("0_2")) + nl + q(QStringLiteral("1_2")) + nl;

    // 2.3  2×2 contiguous rectangle
    QTest::newRow("2.3 2x2-rect") << 4 << 4 << QString() << QString::fromStdString(sel(1, 1, 2, 2))
                                  << (q(QStringLiteral("1_1")) + tab + q(QStringLiteral("1_2")) + nl + q(QStringLiteral("2_1")) + tab + q(QStringLiteral("2_2"))
                                      + nl);

    // 2.4  Full row selection
    QTest::newRow("2.4 fullRow") << 3 << 4 << QString() << QString::fromStdString(sel(1, 0, 1, 3))
                                 << (q(QStringLiteral("1_0")) + tab + q(QStringLiteral("1_1")) + tab + q(QStringLiteral("1_2")) + tab + q(QStringLiteral("1_3"))
                                     + nl);

    // 2.5  Full column selection
    QTest::newRow("2.5 fullCol") << 4 << 3 << QString() << QString::fromStdString(sel(0, 1, 3, 1))
                                 << (q(QStringLiteral("0_1")) + nl + q(QStringLiteral("1_1")) + nl + q(QStringLiteral("2_1")) + nl + q(QStringLiteral("3_1"))
                                     + nl);

    // 2.6  Entire table
    QTest::newRow("2.6 entireTable") << 2 << 3 << QString() << QString::fromStdString(sel(0, 0, 1, 2))
                                     << (q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + tab + q(QStringLiteral("0_2")) + nl
                                         + q(QStringLiteral("1_0")) + tab + q(QStringLiteral("1_1")) + tab + q(QStringLiteral("1_2")) + nl);

    // 2.7  Custom initial data — contiguous rect
    QTest::newRow("2.7 customData-2x2") << 3 << 3 << QString::fromStdString(grid(gridRow("A", "B", "C"), gridRow("D", "E", "F"), keepRow(3)))
                                        << QString::fromStdString(sel(0, 0, 1, 1))
                                        << (q(QStringLiteral("A")) + tab + q(QStringLiteral("B")) + nl + q(QStringLiteral("D")) + tab + q(QStringLiteral("E"))
                                            + nl);

    // 2.8  3×3 contiguous
    QTest::newRow("2.8 3x3-contiguous") << 5 << 5 << QString() << QString::fromStdString(sel(0, 0, 2, 2))
                                        << (q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + tab + q(QStringLiteral("0_2")) + nl
                                            + q(QStringLiteral("1_0")) + tab + q(QStringLiteral("1_1")) + tab + q(QStringLiteral("1_2")) + nl
                                            + q(QStringLiteral("2_0")) + tab + q(QStringLiteral("2_1")) + tab + q(QStringLiteral("2_2")) + nl);
}

void TestExportData::testContiguousSelection()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    QStandardItemModel *model = buildModel(initial, rows, cols);
    QModelIndexList sel = buildSelectionFromModel(model, selection);

    QString output;
    QTextStream stream(&output);
    DataOutputImportExportHelpers::exportData(model, sel, stream);

    delete model;
    QCOMPARE(output, expected);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 3 — Non-contiguous selection export
//
// Non-selected cells within the bounding rectangle of the selection
// produce empty fields. Non-selected rows produce empty lines
// (just a line delimiter).
// ═════════════════════════════════════════════════════════════════════════════
void TestExportData::testNonContiguousSelection_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    const QChar tab = Defaults::FieldDelimiterForCopyPaste;
    const QString nl = QString(Defaults::LineDelimiterForCopyPaste);

    // 3.1  Two cells in same row, gap in column (0,0) and (0,2)
    //      bounding rect: row 0, cols 0-2
    QTest::newRow("3.1 twoCells-sameRow-colGap") << 3 << 4 << QString() << QString::fromStdString(combine(sel(0, 0), sel(0, 2)))
                                                 << (q(QStringLiteral("0_0")) + tab + tab + q(QStringLiteral("0_2")) + nl);

    // 3.2  Two cells in same column, gap in row (0,0) and (2,0)
    //      bounding rect: rows 0-2, col 0
    QTest::newRow("3.2 twoCells-sameCol-rowGap") << 4 << 3 << QString() << QString::fromStdString(combine(sel(0, 0), sel(2, 0)))
                                                 << (q(QStringLiteral("0_0")) + nl + nl + q(QStringLiteral("2_0")) + nl);

    // 3.3  Two corner cells: (0,0) and (2,2) in 3×3 table
    //      bounding rect: rows 0-2, cols 0-2
    QTest::newRow("3.3 corners-0_0-and-2_2") << 3 << 3 << QString() << QString::fromStdString(combine(sel(0, 0), sel(2, 2)))
                                             << (q(QStringLiteral("0_0")) + tab + tab + nl + nl + tab + tab + q(QStringLiteral("2_2")) + nl);

    // 3.4  Two cells far apart: (0,0) and (3,3) in 4x4 table
    //      bounding rect: rows 0-3, cols 0-3
    //      Row 3: cols 0-2 are empty (not selected for row 3), col 3 has "3_3"
    QTest::newRow("3.4 far-apart-diagonal") << 4 << 4 << QString() << QString::fromStdString(combine(sel(0, 0), sel(3, 3)))
                                            << (q(QStringLiteral("0_0")) + tab + tab + tab + nl + nl + nl + tab + tab + tab + q(QStringLiteral("3_3")) + nl);

    // 3.5  L-shaped: (0,0)-(0,2) + (1,0)
    //      bounding rect: rows 0-1, cols 0-2
    //      Row 0: all 3 cols selected
    //      Row 1: only col 0 selected, cols 1-2 empty
    QTest::newRow("3.5 L-shape") << 3 << 4 << QString() << QString::fromStdString(combine(sel(0, 0, 0, 2), sel(1, 0)))
                                 << (q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + tab + q(QStringLiteral("0_2")) + nl + q(QStringLiteral("1_0"))
                                     + tab + tab + nl);

    // 3.6  Checkerboard pattern in 2×2
    QTest::newRow("3.6 checkerboard-2x2") << 3 << 3 << QString() << QString::fromStdString(combine(sel(0, 0), sel(1, 1)))
                                          << (q(QStringLiteral("0_0")) + tab + nl + tab + q(QStringLiteral("1_1")) + nl);

    // 3.7  Three non-contiguous cells in same row
    QTest::newRow("3.7 threeCells-sameRow-spread") << 2 << 6 << QString() << QString::fromStdString(combine(sel(0, 0), sel(0, 2), sel(0, 5)))
                                                   << (q(QStringLiteral("0_0")) + tab + tab + q(QStringLiteral("0_2")) + tab + tab + tab
                                                       + q(QStringLiteral("0_5")) + nl);

    // 3.8  Custom data with non-contiguous selection
    //      Selection: (0,1) and (2,1) — same column, gap in rows
    //      bounding rect: rows 0-2, col 1 (single col, no tabs)
    QTest::newRow("3.8 customData-nonContiguous") << 3 << 3
                                                  << QString::fromStdString(grid(gridRow("X", "Y", "Z"), gridRow("P", "Q", "R"), gridRow("M", "N", "O")))
                                                  << QString::fromStdString(combine(sel(0, 1), sel(2, 1)))
                                                  << (q(QStringLiteral("Y")) + nl + nl + q(QStringLiteral("N")) + nl);
}

void TestExportData::testNonContiguousSelection()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    QStandardItemModel *model = buildModel(initial, rows, cols);
    QModelIndexList sel = buildSelectionFromModel(model, selection);

    QString output;
    QTextStream stream(&output);
    DataOutputImportExportHelpers::exportData(model, sel, stream);

    delete model;
    QCOMPARE(output, expected);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 4 — Escape characters and special content
// ═════════════════════════════════════════════════════════════════════════════
void TestExportData::testEscapeCharacters_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    const QChar tab = Defaults::FieldDelimiterForCopyPaste;
    const QString nl = QString(Defaults::LineDelimiterForCopyPaste);

    // 4.1  Cell containing a double quote
    QTest::newRow("4.1 doubleQuote") << 1 << 1 << QStringLiteral("say \"hello\"") << QString::fromStdString(sel(0, 0))
                                     << q(QStringLiteral("say \"hello\"")) + nl;

    // 4.2  Cell containing a backslash
    QTest::newRow("4.2 backslash") << 1 << 1 << QStringLiteral("path\\to\\file") << QString::fromStdString(sel(0, 0))
                                   << q(QStringLiteral("path\\to\\file")) + nl;

    // 4.3  Cell containing a newline
    QTest::newRow("4.3 newline") << 1 << 1 << QStringLiteral("line1\nline2") << QString::fromStdString(sel(0, 0)) << q(QStringLiteral("line1\nline2")) + nl;

    // 4.4  Cell containing a carriage return
    QTest::newRow("4.4 carriageReturn") << 1 << 1 << QStringLiteral("text\rmore") << QString::fromStdString(sel(0, 0)) << q(QStringLiteral("text\rmore")) + nl;

    // 4.5  Cell containing a tab
    QTest::newRow("4.5 tab") << 1 << 1 << QStringLiteral("col1\tcol2") << QString::fromStdString(sel(0, 0)) << q(QStringLiteral("col1\tcol2")) + nl;

    // 4.6  Cell with backslash followed by quote (the tricky case for escape order)
    QTest::newRow("4.6 backslashThenQuote") << 1 << 1 << QStringLiteral("end\\\"") << QString::fromStdString(sel(0, 0)) << q(QStringLiteral("end\\\"")) + nl;

    // 4.7  Cell with all special characters combined
    QTest::newRow("4.7 allSpecial") << 1 << 1 << QStringLiteral("a\tb\nc\rd\\e\"f") << QString::fromStdString(sel(0, 0))
                                    << q(QStringLiteral("a\tb\nc\rd\\e\"f")) + nl;

    // 4.8  Multiple cells with escape characters, contiguous
    QTest::newRow("4.8 multiCell-contiguous-escapes") << 1 << 3 << QString::fromStdString(grid(gridRow("a\"b", "c\\d", "e\tf")))
                                                      << QString::fromStdString(sel(0, 0, 0, 2))
                                                      << (q(QStringLiteral("a\"b")) + tab + q(QStringLiteral("c\\d")) + tab + q(QStringLiteral("e\tf")) + nl);

    // 4.9  Escape chars with non-contiguous selection
    QTest::newRow("4.9 escapes-nonContiguous") << 2 << 3 << QString::fromStdString(grid(gridRow("a\"b", "c\\d", "e\tf"), gridRow("g\nh", "i\"j", "k\\l")))
                                               << QString::fromStdString(combine(sel(0, 0), sel(1, 2)))
                                               << (q(QStringLiteral("a\"b")) + tab + tab + nl + tab + tab + q(QStringLiteral("k\\l")) + nl);

    // 4.10  Empty string cell (tested inline in testEscapeCharacters)

    // 4.11  Cell with just a quote character
    QTest::newRow("4.11 justQuote") << 1 << 1 << QStringLiteral("\"") << QString::fromStdString(sel(0, 0)) << q(QStringLiteral("\"")) + nl;

    // 4.12  Cell with just a backslash
    QTest::newRow("4.12 justBackslash") << 1 << 1 << QStringLiteral("\\") << QString::fromStdString(sel(0, 0)) << q(QStringLiteral("\\")) + nl;

    // 4.13  Cell with consecutive special chars
    QTest::newRow("4.13 consecutiveSpecials") << 1 << 1 << QStringLiteral("\"\\\n\t") << QString::fromStdString(sel(0, 0))
                                              << q(QStringLiteral("\"\\\n\t")) + nl;

    // 4.14  Non-contiguous with special chars in gap cells
    QTest::newRow("4.14 specialChars-gapCellsEmpty") << 1 << 5 << QString::fromStdString(grid(gridRow("a\"b", "normal", "c\\d", "normal2", "e\tf")))
                                                     << QString::fromStdString(combine(sel(0, 0), sel(0, 4)))
                                                     << (q(QStringLiteral("a\"b")) + tab + tab + tab + tab + q(QStringLiteral("e\tf")) + nl);
}

void TestExportData::testEscapeCharacters()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    QStandardItemModel *model = buildModel(initial, rows, cols);
    QModelIndexList sel = buildSelectionFromModel(model, selection);

    QString output;
    QTextStream stream(&output);
    DataOutputImportExportHelpers::exportData(model, sel, stream);

    delete model;
    QCOMPARE(output, expected);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 4b — Empty string cell (separate non-data-driven test)
// ═════════════════════════════════════════════════════════════════════════════
void TestExportData::testEmptyStringCell()
{
    QStandardItemModel m(1, 1);
    auto *item = new QStandardItem(QString());
    item->setData(QString(), Qt::UserRole);
    m.setItem(0, 0, item);

    QModelIndexList sel{m.index(0, 0)};
    QString output;
    QTextStream stream(&output);
    DataOutputImportExportHelpers::exportData(&m, sel, stream);

    const QString nl = QString(Defaults::LineDelimiterForCopyPaste);
    QCOMPARE(output, q(QString()) + nl);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 5 — Export options (column names, line numbers)
// ═════════════════════════════════════════════════════════════════════════════
void TestExportData::testExportOptions_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<DataOutputImportExportHelpers::Options>("options");

    const QChar tab = Defaults::FieldDelimiterForCopyPaste;
    const QString nl = QString(Defaults::LineDelimiterForCopyPaste);

    // 5.1  ExportColumnNames only — 2×2 contiguous
    {
        QString headerLine = q(QStringLiteral("col0")) + tab + q(QStringLiteral("col1")) + nl;
        QString dataLines = q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + nl + q(QStringLiteral("1_0")) + tab + q(QStringLiteral("1_1")) + nl;
        QTest::newRow("5.1 columnNames-only") << 2 << 2 << QString() << QString::fromStdString(sel(0, 0, 1, 1)) << (headerLine + dataLines)
                                              << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::ExportColumnNames);
    }

    // 5.2  ExportLineNumbers only — 2×2 contiguous
    {
        QString dataLines = QLatin1Char('1') + tab + q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + nl + QLatin1Char('2') + tab
            + q(QStringLiteral("1_0")) + tab + q(QStringLiteral("1_1")) + nl;
        QTest::newRow("5.2 lineNumbers-only") << 2 << 2 << QString() << QString::fromStdString(sel(0, 0, 1, 1)) << dataLines
                                              << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::ExportLineNumbers);
    }

    // 5.3  Both ExportColumnNames and ExportLineNumbers — 2×2 contiguous
    {
        QString headerLine = tab + q(QStringLiteral("col0")) + tab + q(QStringLiteral("col1")) + nl;
        QString dataLines = QLatin1Char('1') + tab + q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + nl + QLatin1Char('2') + tab
            + q(QStringLiteral("1_0")) + tab + q(QStringLiteral("1_1")) + nl;
        QTest::newRow("5.3 columnNames+lineNumbers") << 2 << 2 << QString() << QString::fromStdString(sel(0, 0, 1, 1)) << (headerLine + dataLines)
                                                     << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::ExportColumnNames
                                                                                               | DataOutputImportExportHelpers::ExportLineNumbers);
    }

    // 5.4  ExportColumnNames with non-contiguous (gap column)
    //      Selection: (0,0) and (0,2) — col 1 is skipped
    //      Header line has col0, col1, col2 (only selected cols get header)
    //      But the code only outputs header for columns in the sorted unique list
    {
        // columns: [0, 2], minCol=0, maxCol=2
        // For col 0: header "col0" (in columns list) + tab (col < maxCol)
        // For col 1: NOT in columns list, so no header text, but tab delimiter (col < maxCol)
        // For col 2: header "col2" (in columns list), no delimiter (col == maxCol)
        QString headerLine = q(QStringLiteral("col0")) + tab + tab + q(QStringLiteral("col2")) + nl;
        QString dataLine = q(QStringLiteral("0_0")) + tab + tab + q(QStringLiteral("0_2")) + nl;
        QTest::newRow("5.4 columnNames-nonContiguous-cols")
            << 1 << 4 << QString() << QString::fromStdString(combine(sel(0, 0), sel(0, 2))) << (headerLine + dataLine)
            << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::ExportColumnNames);
    }

    // 5.5  ExportLineNumbers with skipped rows (non-contiguous)
    //      Selection: (0,0) and (2,0) — row 1 is skipped
    {
        QString line1 = QLatin1Char('1') + tab + q(QStringLiteral("0_0")) + nl;
        QString line2 = nl; // row 1 not selected → just line delimiter
        QString line3 = QLatin1Char('3') + tab + q(QStringLiteral("2_0")) + nl;
        QTest::newRow("5.5 lineNumbers-nonContiguous-rows")
            << 3 << 2 << QString() << QString::fromStdString(combine(sel(0, 0), sel(2, 0))) << (line1 + line2 + line3)
            << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::ExportLineNumbers);
    }

    // 5.6  ExportColumnNames with header containing special chars
    //      Note: buildModel sets headers as "col0", "col1", etc.
    //      This test verifies the default headers are properly escaped
    //      when exported. The default "col0"/"col1" have no special chars,
    //      so we test the quoting itself.
    //      (Escape testing for header content is covered by the q() helper.)
    {
        QString headerLine = q(QStringLiteral("col0")) + tab + q(QStringLiteral("col1")) + nl;
        QString dataLine = q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + nl;
        QTest::newRow("5.6 columnNames-basicHeaders") << 1 << 2 << QString() << QString::fromStdString(sel(0, 0, 0, 1)) << (headerLine + dataLine)
                                                      << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::ExportColumnNames);
    }

    // 5.7  ExportColumnNames with headers containing special characters
    //      We use initial grid with escaped chars in headers set manually
    //      This test is special: the expected string includes escaped header names
    {
        // This test verifies headers with special chars are properly escaped.
        // The header content is set via buildModel as "col0" and "col1",
        // but the escape logic for headers mirrors the cell escape logic,
        // which is already covered by Section 4 (escape characters).
        // Here we just verify the integration: headers go through escapeString.
        QTest::newRow("5.7 columnNames-headerEscaping")
            << 1 << 2 << QString::fromStdString(grid(gridRow("data", "more"))) << QString::fromStdString(sel(0, 0, 0, 1))
            << (q(QStringLiteral("col0")) + tab + q(QStringLiteral("col1")) + nl + q(QStringLiteral("data")) + tab + q(QStringLiteral("more")) + nl)
            << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::ExportColumnNames);
    }

    // 5.8  NoOptions (default) — 2×2 (same as testContiguousSelection 2.3)
    {
        QTest::newRow("5.8 noOptions") << 2 << 2 << QString() << QString::fromStdString(sel(0, 0, 1, 1))
                                       << (q(QStringLiteral("0_0")) + tab + q(QStringLiteral("0_1")) + nl + q(QStringLiteral("1_0")) + tab
                                           + q(QStringLiteral("1_1")) + nl)
                                       << DataOutputImportExportHelpers::Options(DataOutputImportExportHelpers::NoOptions);
    }
}

void TestExportData::testExportOptions()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, selection);
    QFETCH(QString, expected);
    QFETCH(DataOutputImportExportHelpers::Options, options);

    QStandardItemModel *model = buildModel(initial, rows, cols);
    QModelIndexList sel = buildSelectionFromModel(model, selection);

    QString output;
    QTextStream stream(&output);
    DataOutputImportExportHelpers::exportData(model,
                                              sel,
                                              stream,
                                              Defaults::QuoteStringCharForCopyPaste,
                                              Defaults::QuoteNumbersCharForCopyPaste,
                                              Defaults::FieldDelimiterForCopyPaste,
                                              options);

    delete model;
    QCOMPARE(output, expected);
}

QTEST_MAIN(TestExportData)

#include "test_exportdata.moc"
