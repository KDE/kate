/*
    SPDX-FileCopyrightText: 2026 The Kate Developers

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KIO/FileCopyJob>
#include <KIO/MkpathJob>
#include <KIO/SimpleJob>

#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class DeploymentTransferTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void uploadThroughTemporaryFile();
};

static bool waitForJob(KJob *job)
{
    QEventLoop loop;
    QObject::connect(job, &KJob::result, &loop, &QEventLoop::quit);
    loop.exec();
    return job->error() == KJob::NoError;
}

void DeploymentTransferTest::uploadThroughTemporaryFile()
{
    QTemporaryDir sourceDir;
    QTemporaryDir destinationDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(destinationDir.isValid());

    const QString sourcePath = sourceDir.filePath(QStringLiteral("source.txt"));
    QFile source(sourcePath);
    QVERIFY(source.open(QFile::WriteOnly));
    QCOMPARE(source.write("deployed content"), 16);
    source.close();

    const QUrl destinationRoot = QUrl::fromLocalFile(destinationDir.path());
    const QUrl parentUrl = QUrl::fromLocalFile(destinationDir.filePath(QStringLiteral("missing/nested")));
    QVERIFY(waitForJob(KIO::mkpath(parentUrl, destinationRoot, KIO::HideProgressInfo)));

    const QUrl temporaryUrl = QUrl::fromLocalFile(destinationDir.filePath(QStringLiteral("missing/nested/.target.txt.kate-upload")));
    const QUrl finalUrl = QUrl::fromLocalFile(destinationDir.filePath(QStringLiteral("missing/nested/target.txt")));
    QVERIFY(waitForJob(KIO::file_copy(QUrl::fromLocalFile(sourcePath), temporaryUrl, -1, KIO::Overwrite | KIO::HideProgressInfo)));
    QVERIFY(waitForJob(KIO::rename(temporaryUrl, finalUrl, KIO::Overwrite | KIO::HideProgressInfo)));

    QFile deployed(finalUrl.toLocalFile());
    QVERIFY(deployed.open(QFile::ReadOnly));
    QCOMPARE(deployed.readAll(), QByteArray("deployed content"));
    QVERIFY(!QFile::exists(temporaryUrl.toLocalFile()));
}

QTEST_GUILESS_MAIN(DeploymentTransferTest)

#include "deploymenttransfertest.moc"
