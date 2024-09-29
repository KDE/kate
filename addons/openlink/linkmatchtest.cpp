/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "matchers.h"

#include <QFile>
#include <QTest>

bool operator==(OpenLinkRange l, OpenLinkRange r)
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
        QTest::addRow("1") << "Line has https://google.com" << R{OpenLinkRange{9, 27, HttpLink}};
        QTest::addRow("2") << "Line has https://google.com and https://google.com" << R{OpenLinkRange{9, 27, HttpLink}, OpenLinkRange{32, 50, HttpLink}};

        QFile file(QDir::current().absoluteFilePath(QStringLiteral("testfile")));
        file.open(QFile::WriteOnly);
        file.write("abc");
        file.close();

        QString t = QLatin1String("Text has filepath: %1").arg(file.fileName());
        QTest::addRow("3") << t << R{OpenLinkRange{19, (int)(19 + file.fileName().size()), FileLink}};

        t = QLatin1String("// Text has filepath: %1").arg(file.fileName());
        QTest::addRow("4") << t << R{OpenLinkRange{22, (int)(22 + file.fileName().size()), FileLink}};

        t = QLatin1String("// Text has filepath: %1 -- /non/existent/path").arg(file.fileName());
        QTest::addRow("4") << t << R{OpenLinkRange{22, (int)(22 + file.fileName().size()), FileLink}};

        t = QLatin1String("// Text has filepath: %1 -- second: %1").arg(file.fileName());
        QTest::addRow("4") << t
                           << R{
                                  OpenLinkRange{22, (int)(22 + file.fileName().size()), FileLink},
                                  OpenLinkRange{(int)(22 + file.fileName().size() + 12), (int)(22 + (file.fileName().size() * 2) + 12), FileLink},
                              };
    }

    void test()
    {
        QFETCH(QString, line);
        QFETCH(std::vector<OpenLinkRange>, expected);

        std::vector<OpenLinkRange> ranges;
        matchLine(line, &ranges);

        // output on failure
        if (ranges != expected) {
            qDebug() << "Failed line:" << line;
            qDebug().nospace() << "Actual: ";
            for (auto [a, b, c] : ranges) {
                qDebug() << a << b << c;
            }
            qDebug() << "----";
            qDebug().nospace() << "Expected: ";
            for (auto [a, b, c] : expected) {
                qDebug() << a << b << c;
            }
        }

        QCOMPARE(ranges, expected);
    }
};

QTEST_MAIN(LinkMatchTest)
#include "linkmatchtest.moc"
