/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "matchers.h"

#include <QFile>
#include <QTest>

static bool operator==(OpenLinkRange l, OpenLinkRange r)
{
    return l.start == r.start && l.end == r.end && l.type == r.type;
}

class LinkMatchTest : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

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
        file.open(QFile::WriteOnly);
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
        t = QLatin1String("text \"/");
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
            for (auto [start, end, _, cursor, type] : ranges) {
                dbg.append(QStringLiteral("%1 %2 %4 %3\n").arg(start).arg(end).arg(type).arg(cursor.toString()));
            }
            qDebug("Actual: %ls", qUtf16Printable(dbg));
            qDebug("----");

            dbg.clear();
            for (auto [start, end, _, cursor, type] : expected) {
                dbg.append(QStringLiteral("%1 %2 %4 %3\n").arg(start).arg(end).arg(type).arg(cursor.toString()));
            }
            qDebug("Expected: %ls", qUtf16Printable(dbg));
        }

        QCOMPARE(ranges, expected);
    }
};

QTEST_MAIN(LinkMatchTest)
#include "linkmatchtest.moc"
