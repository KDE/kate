/*
 * This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2025 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *
 *  SPDX-License-Identifier: MIT
 */

#include <QCoreApplication>
#include <QString>
#include <QTemporaryFile>
#include <QTest>

#include <KIO/FileCopyJob>
#include <KIO/ListJob>
#include <KIO/StatJob>

#include "../exec_io_utils.h"

class ExecIOTest : public QObject
{
    Q_OBJECT

    QTemporaryFile m_tempFile;

public:
    ExecIOTest()
    {
        setup();
    }

    void setup()
    {
        Utils::ExecPrefixManager::instance().update(QLatin1String("identity"), {QLatin1String("env")});
    }

    void makeRemote(QUrl &url)
    {
        url.setScheme(Utils::ExecPrefixManager::scheme());
        url.setHost(QLatin1String("identity"));
    }

    void doCopy(bool toFile)
    {
        QByteArray data{"foobar"};

        // remote
        QTemporaryFile remote;
        QVERIFY(remote.open());
        auto remoteUrl = QUrl::fromLocalFile(remote.fileName());
        makeRemote(remoteUrl);

        // local
        QTemporaryFile local;
        QVERIFY(local.open());
        auto localUrl = QUrl::fromLocalFile(local.fileName());

        auto &srcUrl = toFile ? remoteUrl : localUrl;
        auto &destUrl = toFile ? localUrl : remoteUrl;
        auto &dest = toFile ? local : remote;
        auto &src = toFile ? remote : local;

        src.write(data);
        src.flush();
        src.close();

        QEventLoop loop;

        // copy
        auto job = QScopedPointer(KIO::file_copy(srcUrl, destUrl, 0600, KIO::Overwrite));

        QObject::connect(job.get(), &KJob::result, this, [&](KJob *job) {
            loop.quit();
            QVERIFY(!job->error());
            dest.seek(0);
            QCOMPARE(dest.readAll(), data);
        });

        loop.exec();

        // should reject overwrite
        job.reset(KIO::file_copy(srcUrl, destUrl, 0600, KIO::DefaultFlags));

        QObject::connect(job.get(), &KJob::result, this, [&](KJob *job) {
            loop.quit();
            QVERIFY(job->error());
            dest.seek(0);
            QCOMPARE(dest.readAll(), data);
        });

        loop.exec();
    }

private Q_SLOTS:

    void testCopyToFile()
    {
        doCopy(true);
    }

    void testCopyFromFile()
    {
        doCopy(false);
    }

    void testListDir()
    {
        QTemporaryDir tempdir;

        QVERIFY(tempdir.isValid());

        auto url = QUrl::fromLocalFile(tempdir.path());
        makeRemote(url);

        QDir topdir(tempdir.path());
        // add some content
        topdir.mkdir(topdir.path() + QLatin1String("/subdir"));
        QFile file(topdir.path() + QLatin1String("/subfile"));
        QVERIFY(file.open(QFile::WriteOnly));
        file.close();

        auto job = QScopedPointer(KIO::listDir(url));

        QEventLoop loop;

        KIO::UDSEntryList list;
        QObject::connect(job.get(), &KIO::ListJob::entries, this, [&](KIO::Job *job, const KIO::UDSEntryList &entries) {
            (void)job;
            list.append(entries);
        });

        QObject::connect(job.get(), &KJob::result, this, [&](KJob *job) {
            loop.quit();
            QVERIFY(!job->error());
        });

        loop.exec();

        // check contents
        QCOMPARE(list.size(), 4);

        for (const auto &e : list) {
            auto name = e.stringValue(KIO::UDSEntry::UDS_NAME);
            auto isDir = e.isDir();
            if (name == QLatin1String(".") || name == QLatin1String("..") || name == QLatin1String("subdir")) {
                QVERIFY(isDir);
            } else {
                QVERIFY(!isDir);
                QCOMPARE(name, QFileInfo(file.fileName()).baseName());
            }
        }
    }
};

QTEST_MAIN(ExecIOTest)

#include "io_tests.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
