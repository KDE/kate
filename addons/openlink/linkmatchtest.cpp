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
#ifndef Q_OS_WIN
        QFile::remove(QDir::current().absoluteFilePath(QStringLiteral("testfile")));
#endif
    }

    void test_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<std::vector<OpenLinkRange>>("expected");

        using R = std::vector<OpenLinkRange>;
        QTest::addRow("1") << "Line has https://google.com" << R{OpenLinkRange{9, 27, HttpLink}};
        QTest::addRow("2") << "Line has https://google.com and https://google.com" << R{OpenLinkRange{9, 27, HttpLink}, OpenLinkRange{32, 50, HttpLink}};

#ifndef Q_OS_WIN
        QFile file(QDir::current().absoluteFilePath(QStringLiteral("testfile")));
        file.open(QFile::WriteOnly);
        file.write("abc");
        file.close();

        QString t = QLatin1String("Text has filepath: %1").arg(file.fileName());
        QTest::addRow("3") << t << R{OpenLinkRange{19, (int)(19 + file.fileName().size()), FileLink}};
#endif
    }

    void test()
    {
        QFETCH(QString, line);
        QFETCH(std::vector<OpenLinkRange>, expected);

        std::vector<OpenLinkRange> ranges;
        matchLine(line, &ranges);
        // for (auto [a, b, c] : ranges) {
        //     qDebug() << a << b << c;
        // }
        // qDebug() << "----";
        // for (auto [a, b, c] : expected) {
        //     qDebug() << a << b << c;
        // }
        QCOMPARE(ranges, expected);
    }
};

QTEST_MAIN(LinkMatchTest)
#include "linkmatchtest.moc"
