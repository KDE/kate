/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "matchers.h"

#include <QFile>
#include <QTest>

static bool operator==(OpenLinkRange l, OpenLinkRange r)
{
    return l.start == r.start && l.end == r.end && l.type == r.type && l.link == r.link;
}

class LinkMatchTest : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

private:
    QTemporaryDir m_dir;

private Q_SLOTS:
    void cleanupTestCase()
    {
        QFile::remove(QDir::current().absoluteFilePath(QStringLiteral("testfile")));
    }

    void test_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<std::vector<OpenLinkRange>>("expected");

        using R = std::vector<OpenLinkRange>;

        QTest::addRow("1") << "Line has https://google.com"
                           << R{OpenLinkRange{.start = 9, .end = 27, .link = QStringLiteral("https://google.com"), .type = HttpLink}};
        QTest::addRow("2") << "Line has https://google.com and https://google.com"
                           << R{OpenLinkRange{.start = 9, .end = 27, .link = QStringLiteral("https://google.com"), .type = HttpLink},
                                OpenLinkRange{.start = 32, .end = 50, .link = QStringLiteral("https://google.com"), .type = HttpLink}};

        QFile file(QDir::current().absoluteFilePath(QStringLiteral("testfile")));
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("abc");
        file.close();
        const QString filePath = file.fileName();
        const int fileLen = filePath.size();

        QString t = QLatin1String("Text has filepath: %1").arg(filePath);
        QTest::addRow("3") << t << R{OpenLinkRange{.start = 19, .end = (19 + fileLen), .link = filePath, .type = FileLink}};

        t = QLatin1String("// Text has filepath: %1").arg(filePath);
        QTest::addRow("4") << t << R{OpenLinkRange{.start = 22, .end = (22 + fileLen), .link = filePath, .type = FileLink}};

        t = QLatin1String("// Text has filepath: %1 -- /non/existent/path").arg(filePath);
        QTest::addRow("5") << t << R{OpenLinkRange{.start = 22, .end = (22 + fileLen), .link = filePath, .type = FileLink}};

        t = QLatin1String("// Text has filepath: %1 -- second: %1").arg(filePath);
        QTest::addRow("6") << t
                           << R{
                                  OpenLinkRange{.start = 22, .end = (22 + fileLen), .link = filePath, .type = FileLink},
                                  OpenLinkRange{.start = (22 + fileLen + 12), .end = (22 + (fileLen * 2) + 12), .link = filePath, .type = FileLink},
                              };
        t = QStringLiteral("text \"/");
        QTest::addRow("7") << t << R{};

        t = QLatin1String("Text has filepath: %1:12").arg(filePath);
        QTest::addRow("8") << t << R{OpenLinkRange{.start = 19, .end = (19 + fileLen + 3), .link = filePath, .startPos = {12, 0}, .type = FileLink}};

        t = QLatin1String("Text has filepath: %1:13:25 ").arg(filePath);
        QTest::addRow("9") << t << R{OpenLinkRange{.start = 19, .end = (19 + fileLen + 6), .link = filePath, .startPos = {13, 25}, .type = FileLink}};

        t = QLatin1String("Text has filepath: \"%1:13:25\" ").arg(filePath);
        QTest::addRow("10") << t << R{OpenLinkRange{.start = 20, .end = (20 + fileLen + 6), .link = filePath, .startPos = {13, 25}, .type = FileLink}};

        t = QLatin1String("Text has filepath: %1:").arg(filePath);
        QTest::addRow("11") << t << R{OpenLinkRange{.start = 19, .end = (19 + fileLen) + 1, .link = filePath, .type = FileLink}};

        t = QLatin1String("Text has filepath: %1:x12:9xx").arg(filePath);
        QTest::addRow("12") << t << R{};

        t = QLatin1String("Text has filepath: %1:x12:").arg(filePath);
        QTest::addRow("13") << t << R{};

        t = QLatin1String("Text has filepath: %1:x12").arg(filePath);
        QTest::addRow("14") << t << R{};

        t = QLatin1String("%1").arg(filePath);
        QTest::addRow("15") << t << R{OpenLinkRange{.start = 0, .end = fileLen, .link = filePath, .type = FileLink}};

        t = QLatin1String("Text has filepath: \"%1:13:25:\" ").arg(filePath);
        QTest::addRow("16") << t << R{OpenLinkRange{.start = 20, .end = (20 + fileLen + 7), .link = filePath, .startPos = {13, 25}, .type = FileLink}};

        QTest::addRow("17") << QStringLiteral("[ccc](https://cullmann.dev)")
                            << R{OpenLinkRange{.start = 6, .end = 26, .link = QStringLiteral("https://cullmann.dev"), .type = HttpLink}};

        QTest::addRow("18") << QStringLiteral("Visit 'https://cullmann.dev'")
                            << R{OpenLinkRange{.start = 7, .end = 27, .link = QStringLiteral("https://cullmann.dev"), .type = HttpLink}};

        QTest::addRow("19") << QStringLiteral("Visit \"https://cullmann.dev\"")
                            << R{OpenLinkRange{.start = 7, .end = 27, .link = QStringLiteral("https://cullmann.dev"), .type = HttpLink}};

        QTest::addRow("20") << QStringLiteral("The web site https://cullmann.dev.")
                            << R{OpenLinkRange{.start = 13, .end = 33, .link = QStringLiteral("https://cullmann.dev"), .type = HttpLink}};

        QTest::addRow("21") << QStringLiteral("<https://cullmann.dev>")
                            << R{OpenLinkRange{.start = 1, .end = 21, .link = QStringLiteral("https://cullmann.dev"), .type = HttpLink}};

        QTest::addRow("22") << QStringLiteral("<https://cullmann.dev> xxx <https://hello.dev>")
                            << R{OpenLinkRange{.start = 1, .end = 21, .link = QStringLiteral("https://cullmann.dev"), .type = HttpLink},
                                 OpenLinkRange{.start = 28, .end = 45, .link = QStringLiteral("https://hello.dev"), .type = HttpLink}};

        // something like: (/home/user/projects/file/Extensions/xyz/File.cpp:713,
        QTest::addRow("23") << QLatin1String("(%1:713,").arg(filePath)
                            << R{OpenLinkRange{.start = 1, .end = 1 + fileLen + 4, .link = filePath, .startPos = {713, 0}, .type = FileLink}};

        QTest::addRow("24")
            << QStringLiteral("(for [#3695](https://github.com/pbek/QOwnNotes/issues/3695))")
            << R{OpenLinkRange{.start = 13, .end = 58, .link = QStringLiteral("https://github.com/pbek/QOwnNotes/issues/3695"), .type = HttpLink}};

        // balanced parens in the url are kept
        QTest::addRow("25") << QStringLiteral("see https://en.wikipedia.org/wiki/Link_(film)")
                            << R{OpenLinkRange{.start = 4, .end = 45, .link = QStringLiteral("https://en.wikipedia.org/wiki/Link_(film)"), .type = HttpLink}};
    }

    void test()
    {
        QFETCH(QString, line);
        QFETCH(std::vector<OpenLinkRange>, expected);

        std::vector<OpenLinkRange> ranges;
        matchLine(line, &ranges);

        // output on failure
        if (ranges != expected) {
            qDebug("Failed line: %ls", qUtf16Printable(line));
            QString dbg;
            for (const auto &[start, end, _, cursor, type] : ranges) {
                dbg.append(QStringLiteral("%1 %2 %4 %3\n").arg(start).arg(end).arg(type).arg(cursor.toString()));
            }
            qDebug("Actual: %ls", qUtf16Printable(dbg));
            qDebug("----");

            dbg.clear();
            for (const auto &[start, end, _, cursor, type] : expected) {
                dbg.append(QStringLiteral("%1 %2 %4 %3\n").arg(start).arg(end).arg(type).arg(cursor.toString()));
            }
            qDebug("Expected: %ls", qUtf16Printable(dbg));
        }

        QCOMPARE(ranges, expected);
    }

    void test_relative_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<QString>("baseDir");
        QTest::addColumn<std::vector<OpenLinkRange>>("expected");

        using R = std::vector<OpenLinkRange>;

        QVERIFY(m_dir.isValid());
        const QDir root(m_dir.path());
        QVERIFY(root.mkpath(QStringLiteral("sub")));
        const auto touch = [](const QString &path) {
            QFile f(path);
            if (!f.open(QFile::WriteOnly)) {
                return false;
            }
            f.write("abc");
            f.close();
            return true;
        };
        QVERIFY(touch(root.filePath(QStringLiteral("testfile"))));
        QVERIFY(touch(root.filePath(QStringLiteral("sub/nested.txt"))));

        const QString base = root.path();
        const QString sub = root.filePath(QStringLiteral("sub"));
        const QString absFile = root.filePath(QStringLiteral("testfile"));
        const int absLen = absFile.size();

        QTest::addRow("dot-slash") << QStringLiteral("See ./testfile") << base
                                   << R{OpenLinkRange{.start = 4, .end = 14, .link = QStringLiteral("./testfile"), .type = FileLink}};

        QTest::addRow("line-start") << QStringLiteral("./testfile") << base
                                    << R{OpenLinkRange{.start = 0, .end = 10, .link = QStringLiteral("./testfile"), .type = FileLink}};

        QTest::addRow("dot-dot") << QStringLiteral("See ../testfile") << sub
                                 << R{OpenLinkRange{.start = 4, .end = 15, .link = QStringLiteral("../testfile"), .type = FileLink}};

        QTest::addRow("nested") << QStringLiteral("see ./sub/nested.txt") << base
                                << R{OpenLinkRange{.start = 4, .end = 20, .link = QStringLiteral("./sub/nested.txt"), .type = FileLink}};

        QTest::addRow("in-parens") << QStringLiteral("see (../sub/nested.txt)") << sub
                                   << R{OpenLinkRange{.start = 5, .end = 22, .link = QStringLiteral("../sub/nested.txt"), .type = FileLink}};

        QTest::addRow("quoted") << QStringLiteral("open \"./testfile\" now") << base
                                << R{OpenLinkRange{.start = 6, .end = 16, .link = QStringLiteral("./testfile"), .type = FileLink}};

        QTest::addRow("line") << QStringLiteral("./testfile:12") << base
                              << R{OpenLinkRange{.start = 0, .end = 13, .link = QStringLiteral("./testfile"), .startPos = {12, 0}, .type = FileLink}};

        QTest::addRow("line-col-comma")
            << QStringLiteral("(./testfile:13:25,") << base
            << R{OpenLinkRange{.start = 1, .end = 17, .link = QStringLiteral("./testfile"), .startPos = {13, 25}, .type = FileLink}};

        QTest::addRow("trailing-period") << QStringLiteral("Open ./testfile.") << base
                                         << R{OpenLinkRange{.start = 5, .end = 15, .link = QStringLiteral("./testfile"), .type = FileLink}};

        QTest::addRow("two-links") << QStringLiteral("./testfile and ./sub/nested.txt") << base
                                   << R{OpenLinkRange{.start = 0, .end = 10, .link = QStringLiteral("./testfile"), .type = FileLink},
                                        OpenLinkRange{.start = 15, .end = 31, .link = QStringLiteral("./sub/nested.txt"), .type = FileLink}};

        // relative first, absolute second: results must be ordered by position
        QTest::addRow("relative-then-absolute") << QStringLiteral("./testfile and %1").arg(absFile) << base
                                                << R{OpenLinkRange{.start = 0, .end = 10, .link = QStringLiteral("./testfile"), .type = FileLink},
                                                     OpenLinkRange{.start = 15, .end = 15 + absLen, .link = absFile, .type = FileLink}};

        QTest::addRow("nonexistent") << QStringLiteral("see ./does-not-exist") << base << R{};
        QTest::addRow("prev-char-not-acceptable") << QStringLiteral("x./testfile") << base << R{};
        QTest::addRow("three-dots") << QStringLiteral(".../testfile") << base << R{};
        QTest::addRow("unterminated-quote") << QStringLiteral("text \"./testfile") << base << R{};

#ifdef Q_OS_WIN
        QTest::addRow("win-dot-backslash") << QStringLiteral("See .\\testfile") << base << R{OpenLinkRange {
            .start = 4,
            .end = 14,
            .link = QStringLiteral(".\\testfile"),
            .type = FileLink
        }};

        QTest::addRow("win-dotdot-backslash") << QStringLiteral("See ..\\testfile") << sub << R{OpenLinkRange {
            .start = 4,
            .end = 15,
            .link = QStringLiteral("..\\testfile"),
            .type = FileLink
        }};

        QTest::addRow("win-nested-backslash") << QStringLiteral("see .\\sub\\nested.txt:3") << base << R{OpenLinkRange {
            .start = 4,
            .end = 22,
            .link = QStringLiteral(".\\sub\\nested.txt"),
            .startPos = {3, 0},
            .type = FileLink
        }};
#else
        // backslash is not a separator on Linux
        QTest::addRow("linux-backslash-ignored") << QStringLiteral("See .\\testfile") << base << R{};
#endif
    }

    void test_relative()
    {
        QFETCH(QString, line);
        QFETCH(QString, baseDir);
        QFETCH(std::vector<OpenLinkRange>, expected);

        std::vector<OpenLinkRange> ranges;
        matchLine(line, &ranges, baseDir);
        QCOMPARE(ranges, expected);
    }

    // two-argument form: relative to the current working directory
    void test_relative_default_base()
    {
        const QString name = QStringLiteral("relative_default_base_testfile");
        QFile file(QDir::current().absoluteFilePath(name));
        QVERIFY(file.open(QFile::WriteOnly));
        file.write("abc");
        file.close();
        const auto cleanup = qScopeGuard([&] {
            QFile::remove(file.fileName());
        });

        std::vector<OpenLinkRange> ranges;
        matchLine(QStringLiteral("see ./%1:7").arg(name), &ranges);

        const std::vector<OpenLinkRange> expected{
            OpenLinkRange{.start = 4, .end = 4 + 2 + int(name.size()) + 2, .link = QStringLiteral("./") + name, .startPos = {7, 0}, .type = FileLink}};
        QCOMPARE(ranges, expected);
    }
};

QTEST_MAIN(LinkMatchTest)
#include "linkmatchtest.moc"
