#include "gitblametest.h"
#include "../gitblameparser.h"

#include <QDir>
#include <QFile>

void GitBlameTest::testBlameFiles()
{
    QDir rootDir(QStringLiteral(":/"));
    const QFileInfoList files = rootDir.entryInfoList(QDir::Files);
    for (const auto &fileInfo : files) {
        qDebug() << fileInfo.absoluteFilePath();
        QFile in(fileInfo.absoluteFilePath());
        QVERIFY(in.open(QIODeviceBase::ReadOnly));
        KateGitBlameParser parser;
        QVERIFY(parser.parseGitBlame(in.readAll()));
        QVERIFY(parser.hasBlameInfo());
    }
    QCOMPARE(1, 1);
}

QTEST_MAIN(GitBlameTest)

#include "moc_gitblametest.cpp"
