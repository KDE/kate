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
        QTest::addRow("1") << "Line has https://google.com" << R{OpenLinkRange{.start = 9, .end = 27, .type = HttpLink}};
        QTest::addRow("2") << "Line has https://google.com and https://google.com"
                           << R{OpenLinkRange{.start = 9, .end = 27, .type = HttpLink}, OpenLinkRange{.start = 32, .end = 50, .type = HttpLink}};

        QFile file(QDir::current().absoluteFilePath(QStringLiteral("testfile")));
        file.open(QFile::WriteOnly);
        file.write("abc");
        file.close();

        QString t = QLatin1String("Text has filepath: %1").arg(file.fileName());
        QTest::addRow("3") << t << R{OpenLinkRange{.start = 19, .end = (int)(19 + file.fileName().size()), .type = FileLink}};

        t = QLatin1String("// Text has filepath: %1").arg(file.fileName());
        QTest::addRow("4") << t << R{OpenLinkRange{.start = 22, .end = (int)(22 + file.fileName().size()), .type = FileLink}};

        t = QLatin1String("// Text has filepath: %1 -- /non/existent/path").arg(file.fileName());
        QTest::addRow("4") << t << R{OpenLinkRange{.start = 22, .end = (int)(22 + file.fileName().size()), .type = FileLink}};

        t = QLatin1String("// Text has filepath: %1 -- second: %1").arg(file.fileName());
        QTest::addRow("4") << t
                           << R{
                                  OpenLinkRange{.start = 22, .end = (int)(22 + file.fileName().size()), .type = FileLink},
                                  OpenLinkRange{.start = (int)(22 + file.fileName().size() + 12),
                                                .end = (int)(22 + (file.fileName().size() * 2) + 12),
                                                .type = FileLink},
                              };
        t = QLatin1String("text \"/");
        QTest::addRow("5") << t << R{};
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
            for (auto [a, b, c] : ranges) {
                dbg.append(QStringLiteral("%1 %2 %3\n").arg(a).arg(b).arg(c));
            }
            qDebug("Actual: %ls", qUtf16Printable(dbg));
            qDebug("----");

            dbg.clear();
            for (auto [a, b, c] : expected) {
                dbg.append(QStringLiteral("%1 %2 %3\n").arg(a).arg(b).arg(c));
            }
            qDebug("Expected: %ls", qUtf16Printable(dbg));
        }

        QCOMPARE(ranges, expected);
    }
};

QTEST_MAIN(LinkMatchTest)
#include "linkmatchtest.moc"
