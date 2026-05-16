/*
SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>
SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QSqlDatabase>
#include <QTest>
#include <QTextStream>

#include "testhelpers.h"

// Resolve <keep> markers in `expectedGrid` against the default R_C values.
static QString resolveExpected(const QString &expectedGrid, int rows, int cols)
{
    const auto expected = parseGrid(expectedGrid);
    QStringList rowStrings;
    rowStrings.reserve(rows);
    for (int r = 0; r < rows; ++r) {
        QStringList cells;
        cells.reserve(cols);
        for (int c = 0; c < cols; ++c) {
            const QString &val = (r < expected.size() && c < expected[r].size()) ? expected[r][c] : QString(QKeepMarker);
            if (val == QKeepMarker) {
                cells << QString::fromLatin1("%1_%2").arg(r).arg(c);
            } else {
                cells << val;
            }
        }
        rowStrings << cells.join(QLatin1Char(GridFieldDelim));
    }
    return rowStrings.join(QLatin1Char(GridLineDelim));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test class
// ─────────────────────────────────────────────────────────────────────────────

class TestImportData : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    // ── Section 1: Copying a single value ──────────────────────────────────
    void testSingleValue_data();
    void testSingleValue();

    // ── Section 2: Copying multiple values, contiguous selection ───────────
    void testMultiValueContiguous_data();
    void testMultiValueContiguous();

    // ── Section 3: Copying multiple values, non-contiguous selection ───────
    void testMultiValueNonContiguous_data();
    void testMultiValueNonContiguous();

    // ── Section 4: Regression tests for getUniqueSequence deduplication ────
    void testUniqueSequenceRegression_data();
    void testUniqueSequenceRegression();
};

// ═════════════════════════════════════════════════════════════════════════════
// Test setup
// ═════════════════════════════════════════════════════════════════════════════
void TestImportData::initTestCase()
{
    const QString driverName = QStringLiteral("QSQLITE");
    if (!QSqlDatabase::isDriverAvailable(driverName)) {
        QSKIP("QSQLITE driver not available - skipping all import-data tests");
    }

    const QString dbName = QStringLiteral("initTestCase_probe");

    // The driver name may be registered even if the plugin binary is missing.
    // Verify by actually trying to open an in-memory database.
    QSqlDatabase db = QSqlDatabase::addDatabase(driverName, dbName);
    db.setDatabaseName(QStringLiteral(":memory:"));
    if (!db.open()) {
        db.close(); // Will this fix Windows CI???????
        QSqlDatabase::removeDatabase(dbName);
        QSKIP("QSQLITE driver could not be loaded - skipping all import-data tests");
    }
    db.close();
    db = QSqlDatabase(); // release reference before removeDatabase
    QSqlDatabase::removeDatabase(dbName);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 1 — Copying a single value
// ═════════════════════════════════════════════════════════════════════════════
void TestImportData::testSingleValue_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial"); // empty ⇒ default R_C grid
    QTest::addColumn<QString>("stream");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    // 1.1  Single value → single cell
    QTest::newRow("1.1 single-cell") << 4 << 4 << QString() << "X" << QString::fromStdString(sel(1, 1))
                                     << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "0_3"),
                                                                    gridRow("1_0", "X", "1_2", "1_3"),
                                                                    gridRow("2_0", "2_1", "2_2", "2_3"),
                                                                    gridRow("3_0", "3_1", "3_2", "3_3")));

    // 1.2  Single value → contiguous same row, different columns
    QTest::newRow("1.2 contiguous-sameRow") << 4 << 4 << QString() << "X" << QString::fromStdString(sel(1, 1, 1, 3))
                                            << QString::fromStdString(
                                                   grid(gridRow("0_0", "0_1", "0_2", "0_3"), gridRow("1_0", "X", "X", "X"), keepRow(4), keepRow(4)));

    // 1.3  Single value → contiguous same column, different rows
    QTest::newRow("1.3 contiguous-sameCol") << 4 << 4 << QString() << "X" << QString::fromStdString(sel(1, 1, 3, 1))
                                            << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "0_3"),
                                                                           gridRow("1_0", "X", "1_2", "1_3"),
                                                                           gridRow("2_0", "X", "2_2", "2_3"),
                                                                           gridRow("3_0", "X", "3_2", "3_3")));

    // 1.4  Single value → contiguous rectangle (different rows & cols)
    QTest::newRow("1.4 contiguous-rect")
        << 4 << 4 << QString() << "X" << QString::fromStdString(sel(1, 1, 2, 2))
        << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "0_3"), gridRow("1_0", "X", "X", "1_3"), gridRow("2_0", "X", "X", "2_3"), keepRow(4)));

    // 1.5  Single value → non-contiguous same row, different columns
    QTest::newRow("1.5 nonContiguous-sameRow") << 4 << 4 << QString() << "X" << QString::fromStdString(combine(sel(1, 0), sel(1, 3)))
                                               << QString::fromStdString(
                                                      grid(gridRow("0_0", "0_1", "0_2", "0_3"), gridRow("X", "1_1", "1_2", "X"), keepRow(4), keepRow(4)));

    // 1.6  Single value → non-contiguous same column, different rows
    //       Groups: [(0,1)] and [(3,1)], each gets the single value.
    QTest::newRow("1.6 nonContiguous-sameCol") << 4 << 4 << QString() << "X" << QString::fromStdString(combine(sel(0, 1), sel(3, 1)))
                                               << QString::fromStdString(
                                                      grid(gridRow("0_0", "X", "0_2", "0_3"), keepRow(4), keepRow(4), gridRow("3_0", "X", "3_2", "3_3")));

    // 1.7  Single value → non-contiguous 2D
    //       Groups: [(0,0)] and [(4,4)], each gets the single value.
    QTest::newRow("1.7 nonContiguous-2D")
        << 5 << 5 << QString() << "X" << QString::fromStdString(combine(sel(0, 0), sel(4, 4)))
        << QString::fromStdString(grid(gridRow("X", "0_1", "0_2", "0_3", "0_4"), keepRow(5), keepRow(5), keepRow(5), gridRow("4_0", "4_1", "4_2", "4_3", "X")));
}

void TestImportData::testSingleValue()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, stream);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    TestContext ctx(rows, cols, initial);
    QModelIndexList sel = ctx.buildSelection(selection);

    QString data = stream;
    QTextStream ts(&data);
    DataOutputImportExportHelpers::importData(ctx.model, sel, ts, StreamQuoteChar, StreamFieldDelimStr, StreamLineDelimStr);

    const QString actual = ctx.readGrid();
    const QString resolved = resolveExpected(expected, rows, cols);
    QCOMPARE(actual, resolved);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 2 — Copying multiple values, contiguous selection
// ═════════════════════════════════════════════════════════════════════════════
void TestImportData::testMultiValueContiguous_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("stream");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    // 2.1  1×3 data pasted into a single cell → extends beyond selection
    QTest::newRow("2.1 1x3-into-singleCell")
        << 4 << 5 << QString() << QString::fromStdString(streamRow("A", "B", "C")) << QString::fromStdString(sel(1, 1))
        << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "0_3", "0_4"), gridRow("1_0", "A", "B", "C", KeepMarker), keepRow(5), keepRow(5)));

    // 2.2  3×1 data pasted into a single cell → extends in row direction
    QTest::newRow("2.2 3x1-into-singleCell") << 5 << 4 << QString() << QString::fromStdString(streamGrid("A", "B", "C")) << QString::fromStdString(sel(1, 1))
                                             << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "0_3"),
                                                                            gridRow("1_0", "A", "1_2", "1_3"),
                                                                            gridRow("2_0", "B", "2_2", "2_3"),
                                                                            gridRow("3_0", "C", "3_2", "3_3"),
                                                                            keepRow(4)));

    // 2.3  2×3 data into 2×2 selection → data bigger → overwrites non-selected cells
    QTest::newRow("2.3 2x3-into-2x2-overwriteNonSelected")
        << 4 << 5 << QString() << QString::fromStdString(streamGrid(streamRow("A1", "A2", "A3"), streamRow("B1", "B2", "B3")))
        << QString::fromStdString(sel(0, 0, 1, 1))
        << QString::fromStdString(grid(gridRow("A1", "A2", "A3", "0_3", "0_4"), gridRow("B1", "B2", "B3", "1_3", "1_4"), keepRow(5), keepRow(5)));

    // 2.4  2×2 data into 4×4 selection → wrapping
    QTest::newRow("2.4 2x2-into-4x4-wrapping") << 5 << 5 << QString() << QString::fromStdString(streamGrid(streamRow("A", "B"), streamRow("C", "D")))
                                               << QString::fromStdString(sel(0, 0, 3, 3))
                                               << QString::fromStdString(grid(gridRow("A", "B", "A", "B", KeepMarker),
                                                                              gridRow("C", "D", "C", "D", KeepMarker),
                                                                              gridRow("A", "B", "A", "B", KeepMarker),
                                                                              gridRow("C", "D", "C", "D", KeepMarker),
                                                                              keepRow(5)));

    // 2.5  Data with an empty field (consecutive tabs) → existing cell preserved
    QTest::newRow("2.5 emptyField-preservesCell") << 3 << 4 << QString() << QString::fromStdString(streamRow("A", "", "C"))
                                                  << QString::fromStdString(sel(0, 0, 0, 2))
                                                  << QString::fromStdString(grid(gridRow("A", "0_1", "C", "0_3"), keepRow(4), keepRow(4)));

    // 2.6  1×3 data into 1×5 contiguous selection (same row) → wrapping
    QTest::newRow("2.6 1x3-into-1x5-sameRow-wrapping")
        << 3 << 6 << QString() << QString::fromStdString(streamRow("A", "B", "C")) << QString::fromStdString(sel(1, 0, 1, 4))
        << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "0_3", "0_4", "0_5"), gridRow("A", "B", "C", "A", "B", KeepMarker), keepRow(6)));

    // 2.7  3×1 data into 5×1 contiguous selection (same col) → wrapping
    QTest::newRow("2.7 3x1-into-5x1-sameCol-wrapping")
        << 6 << 3 << QString() << QString::fromStdString(streamGrid("A", "B", "C")) << QString::fromStdString(sel(0, 1, 4, 1))
        << QString::fromStdString(grid(gridRow("0_0", "A", "0_2"),
                                       gridRow("1_0", "B", "1_2"),
                                       gridRow("2_0", "C", "2_2"),
                                       gridRow("3_0", "A", "3_2"),
                                       gridRow("4_0", "B", "4_2"),
                                       keepRow(3)));

    // 2.8  2×2 data into 3×3 contiguous rect → wrapping both dimensions
    QTest::newRow("2.8 2x2-into-3x3-wrapping") << 5 << 5 << QString() << QString::fromStdString(streamGrid(streamRow("A", "B"), streamRow("C", "D")))
                                               << QString::fromStdString(sel(0, 0, 2, 2))
                                               << QString::fromStdString(grid(gridRow("A", "B", "A", KeepMarker, KeepMarker),
                                                                              gridRow("C", "D", "C", KeepMarker, KeepMarker),
                                                                              gridRow("A", "B", "A", KeepMarker, KeepMarker),
                                                                              keepRow(5),
                                                                              keepRow(5)));

    // 2.9  Empty stream → no changes
    QTest::newRow("2.9 emptyStream-noChanges") << 3 << 3 << QString() << QString() << QString::fromStdString(sel(1, 1))
                                               << QString::fromStdString(grid(keepRow(3), keepRow(3), keepRow(3)));
}

void TestImportData::testMultiValueContiguous()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, stream);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    TestContext ctx(rows, cols, initial);
    QModelIndexList sel = ctx.buildSelection(selection);

    QString data = stream;
    QTextStream ts(&data);
    DataOutputImportExportHelpers::importData(ctx.model, sel, ts, StreamQuoteChar, StreamFieldDelimStr, StreamLineDelimStr);

    const QString actual = ctx.readGrid();
    const QString resolved = resolveExpected(expected, rows, cols);
    QCOMPARE(actual, resolved);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 3 — Copying multiple values, non-contiguous selection
// ═════════════════════════════════════════════════════════════════════════════
void TestImportData::testMultiValueNonContiguous_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("stream");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    // 3.1  1×3 data, non-contiguous columns in same row (gap bigger than data)
    QTest::newRow("3.1 1x3-nonContiguousCols-gapPreserved")
        << 2 << 8 << QString() << QString::fromStdString(streamRow("A", "B", "C")) << QString::fromStdString(combine(sel(0, 0), sel(0, 5)))
        << QString::fromStdString(grid(gridRow("A", "B", "C", "0_3", "0_4", "A", "B", "C"), keepRow(8)));

    // 3.2  3×1 data, non-contiguous rows in same column (gap bigger than data)
    QTest::newRow("3.2 3x1-nonContiguousRows-gapPreserved")
        << 7 << 3 << QString() << QString::fromStdString(streamGrid("A", "B", "C")) << QString::fromStdString(combine(sel(0, 1), sel(4, 1)))
        << QString::fromStdString(grid(gridRow("0_0", "A", "0_2"),
                                       gridRow("1_0", "B", "1_2"),
                                       gridRow("2_0", "C", "2_2"),
                                       gridRow("3_0", "3_1", "3_2"),
                                       gridRow("4_0", "A", "4_2"),
                                       gridRow("5_0", "B", "5_2"),
                                       gridRow("6_0", "C", "6_2")));

    // 3.3  2×2 data, non-contiguous 2D selection (far apart)
    QTest::newRow("3.3 2x2-nonContiguous2D-farApart") << 6 << 6 << QString() << QString::fromStdString(streamGrid(streamRow("A", "B"), streamRow("C", "D")))
                                                      << QString::fromStdString(combine(sel(0, 0), sel(3, 3)))
                                                      << QString::fromStdString(grid(gridRow("A", "B", "0_2", "0_3", "0_4", "0_5"),
                                                                                     gridRow("C", "D", "1_2", "1_3", "1_4", "1_5"),
                                                                                     keepRow(6),
                                                                                     gridRow("3_0", "3_1", "3_2", "A", "B", "3_5"),
                                                                                     gridRow("4_0", "4_1", "4_2", "C", "D", "4_5"),
                                                                                     keepRow(6)));

    // 3.4  1×3 data, non-contiguous cols with data covering the gap
    QTest::newRow("3.4 1x3-nonContiguousCols-dataCoversGap")
        << 2 << 5 << QString() << QString::fromStdString(streamRow("A", "B", "C")) << QString::fromStdString(combine(sel(0, 0), sel(0, 2)))
        << QString::fromStdString(grid(gridRow("A", "B", "C", "0_3", "0_4"), keepRow(5)));

    // 3.5  2×2 data, two non-contiguous blocks that are close
    QTest::newRow("3.5 2x2-nonContiguousCols-closeGap")
        << 3 << 6 << QString() << QString::fromStdString(streamGrid(streamRow("A", "B"), streamRow("C", "D")))
        << QString::fromStdString(combine(sel(0, 0), sel(0, 3)))
        << QString::fromStdString(grid(gridRow("A", "B", "0_2", "A", "B", "0_5"), gridRow("C", "D", "1_2", "C", "D", "1_5"), keepRow(6)));

    // 3.6  1×1 data (single value), multiple non-contiguous cells
    QTest::newRow("3.6 singleValue-threeNonContiguousCells")
        << 3 << 5 << QString() << "X" << QString::fromStdString(combine(sel(0, 0), sel(1, 2), sel(2, 4)))
        << QString::fromStdString(
               grid(gridRow("X", "0_1", "0_2", "0_3", "0_4"), gridRow("1_0", "1_1", "X", "1_3", "1_4"), gridRow("2_0", "2_1", "2_2", "2_3", "X")));

    // 3.7  1×3 data, three non-contiguous cols (every other col)
    QTest::newRow("3.7 1x3-threeNonContiguousCols") << 2 << 7 << QString() << QString::fromStdString(streamRow("A", "B", "C"))
                                                    << QString::fromStdString(combine(sel(0, 0), sel(0, 2), sel(0, 4)))
                                                    << QString::fromStdString(grid(gridRow("A", "B", "C", "0_3", "A", "B", "C"), keepRow(7)));

    // 3.8  2×3 data, non-contiguous in both rows and cols
    QTest::newRow("3.8 2x3-nonContiguous2D-largeGap") << 7 << 8 << QString()
                                                      << QString::fromStdString(streamGrid(streamRow("A1", "A2", "A3"), streamRow("B1", "B2", "B3")))
                                                      << QString::fromStdString(combine(sel(0, 0), sel(4, 5)))
                                                      << QString::fromStdString(grid(gridRow("A1", "A2", "A3", "0_3", "0_4", "0_5", "0_6", "0_7"),
                                                                                     gridRow("B1", "B2", "B3", "1_3", "1_4", "1_5", "1_6", "1_7"),
                                                                                     keepRow(8),
                                                                                     keepRow(8),
                                                                                     gridRow("4_0", "4_1", "4_2", "4_3", "4_4", "A1", "A2", "A3"),
                                                                                     gridRow("5_0", "5_1", "5_2", "5_3", "5_4", "B1", "B2", "B3"),
                                                                                     keepRow(8)));
}

void TestImportData::testMultiValueNonContiguous()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, stream);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    TestContext ctx(rows, cols, initial);
    QModelIndexList sel = ctx.buildSelection(selection);

    QString data = stream;
    QTextStream ts(&data);
    DataOutputImportExportHelpers::importData(ctx.model, sel, ts, StreamQuoteChar, StreamFieldDelimStr, StreamLineDelimStr);

    const QString actual = ctx.readGrid();
    const QString resolved = resolveExpected(expected, rows, cols);
    QCOMPARE(actual, resolved);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 4 — Regression tests for getUniqueSequence deduplication
//
// getUniqueSequence must return *unique* row/column indices.
// Previously it returned duplicates, causing checkNoSkips to falsely
// detect gaps in contiguous selections and triggering unnecessary
// separateSelectionGroups splitting.
// ═════════════════════════════════════════════════════════════════════════════
void TestImportData::testUniqueSequenceRegression_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<QString>("initial");
    QTest::addColumn<QString>("stream");
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("expected");

    // R.1  Many cells in the same row should NOT trigger grouping.
    QTest::newRow("R.1 sameRow-manyCells-noFalseGrouping")
        << 4 << 6 << QString() << "Q" << QString::fromStdString(sel(2, 0, 2, 5))
        << QString::fromStdString(grid(keepRow(6), keepRow(6), gridRow("Q", "Q", "Q", "Q", "Q", "Q"), keepRow(6)));

    // R.2  Many cells in the same column should NOT trigger grouping.
    QTest::newRow("R.2 sameCol-manyCells-noFalseGrouping") << 6 << 4 << QString() << "Q" << QString::fromStdString(sel(0, 3, 4, 3))
                                                           << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "Q"),
                                                                                          gridRow("1_0", "1_1", "1_2", "Q"),
                                                                                          gridRow("2_0", "2_1", "2_2", "Q"),
                                                                                          gridRow("3_0", "3_1", "3_2", "Q"),
                                                                                          gridRow("4_0", "4_1", "4_2", "Q"),
                                                                                          keepRow(4)));

    // R.3  Large rectangular selection with 1×1 data should NOT group.
    QTest::newRow("R.3 largeRect-singleValue-noFalseGrouping") << 5 << 5 << QString() << "Z" << QString::fromStdString(sel(0, 0, 3, 3))
                                                               << QString::fromStdString(grid(gridRow("Z", "Z", "Z", "Z", KeepMarker),
                                                                                              gridRow("Z", "Z", "Z", "Z", KeepMarker),
                                                                                              gridRow("Z", "Z", "Z", "Z", KeepMarker),
                                                                                              gridRow("Z", "Z", "Z", "Z", KeepMarker),
                                                                                              keepRow(5)));

    // R.4  1×4 data into 1×4 contiguous selection (same row) — no grouping,
    //      exact fit, no wrapping needed.
    QTest::newRow("R.4 1x4-into-1x4-exactFit-noFalseGrouping")
        << 3 << 5 << QString() << QString::fromStdString(streamRow("W", "X", "Y", "Z")) << QString::fromStdString(sel(1, 0, 1, 3))
        << QString::fromStdString(grid(gridRow("0_0", "0_1", "0_2", "0_3", "0_4"), gridRow("W", "X", "Y", "Z", KeepMarker), keepRow(5)));

    // R.5  3×3 data into 4×4 contiguous selection — wrapping, no grouping.
    QTest::newRow("R.5 3x3-into-4x4-contiguous-wrapping")
        << 5 << 5 << QString() << QString::fromStdString(streamGrid(streamRow("A1", "A2", "A3"), streamRow("B1", "B2", "B3"), streamRow("C1", "C2", "C3")))
        << QString::fromStdString(sel(0, 0, 3, 3))
        << QString::fromStdString(grid(gridRow("A1", "A2", "A3", "A1", KeepMarker),
                                       gridRow("B1", "B2", "B3", "B1", KeepMarker),
                                       gridRow("C1", "C2", "C3", "C1", KeepMarker),
                                       gridRow("A1", "A2", "A3", "A1", KeepMarker),
                                       keepRow(5)));
}

void TestImportData::testUniqueSequenceRegression()
{
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(QString, initial);
    QFETCH(QString, stream);
    QFETCH(QString, selection);
    QFETCH(QString, expected);

    TestContext ctx(rows, cols, initial);
    QModelIndexList sel = ctx.buildSelection(selection);

    QString data = stream;
    QTextStream ts(&data);
    DataOutputImportExportHelpers::importData(ctx.model, sel, ts, StreamQuoteChar, StreamFieldDelimStr, StreamLineDelimStr);

    const QString actual = ctx.readGrid();
    const QString resolved = resolveExpected(expected, rows, cols);
    QCOMPARE(actual, resolved);
}

QTEST_MAIN(TestImportData)

#include "test_importdata.moc"
