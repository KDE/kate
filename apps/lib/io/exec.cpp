/*
    SPDX-FileCopyrightText: 2025 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include <KIO/WorkerBase>

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QUrl>

#include <KLocalizedString>

#include "../exec_io_utils.h"

#include "kate_exec_debug.h"

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.exec" FILE "exec.json")
};

class ExecProtocol : public QObject, public KIO::WorkerBase
{
public:
    ExecProtocol(const QByteArray &protocol, const QByteArray &pool, const QByteArray &app)
        : WorkerBase(protocol, pool, app)
    {
    }

    std::pair<QString, QStringList> loadPrefix(const QUrl &url)
    {
        auto args = Utils::ExecPrefixManager::load(url.host());
        if (args.size()) {
            auto program = args.front();
            args.erase(args.begin());
            return {program, args};
        }

        return {};
    }

    KIO::WorkerResult exec(QProcess &process, QString program, const QStringList &args, KIO::Error error, const QUrl &url)
    {
        qCDebug(LibKateExec) << program << args;
        process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        process.start(program, args);

        // ok to block as we are in a worker thread
        // this protocol is meant as a convenient comfortable fallback
        // the latter hardly qualifies as such if we can not make it in some finite time
        // so bail out and give up after some time
        int exitCode = process.waitForFinished(10000) ? 0 : -1;
        if (process.state() == QProcess::Running) {
            exitCode = -1;
            error = KIO::ERR_SERVER_TIMEOUT;
            process.terminate();
            if (!process.waitForFinished(1000))
                process.kill();
        }

        // check proper exit
        if (!exitCode) {
            exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
        }

        // there is semi-official chroot exit code convention
        // (also followed by podman, docker, ssh) to return high exit code
        // if something went wrong "along the way", e.g. could not find/execute the specified command
        // so let's kind of assume that is in place here
        if (exitCode > 120) {
            return KIO::WorkerResult::fail(KIO::ERR_UNSUPPORTED_ACTION, program);
        }

        return !exitCode ? KIO::WorkerResult::pass() : KIO::WorkerResult::fail(error, url.toString());
    }

    KIO::WorkerResult copyPut(const QUrl &src, const QUrl &dest, KIO::JobFlags flags)
    {
        auto destfile = dest.path();
        auto [program, args] = loadPrefix(dest);
        if (program.isEmpty()) {
            return KIO::WorkerResult::fail(KIO::ERR_SERVICE_NOT_AVAILABLE, dest.toString());
        }

        auto srcfile = src.toLocalFile();
        QFileInfo info(srcfile);
        if (!info.isReadable()) {
            return KIO::WorkerResult::fail(KIO::ERR_CANNOT_OPEN_FOR_READING, srcfile);
        }

        QProcess process;
        args += {QLatin1String("dd"), QLatin1String("status=none")};
        QStringList convs;
        if (!(flags & KIO::Overwrite))
            convs.push_back(QLatin1String("excl"));
        if (convs.size())
            args.push_back(QStringLiteral("conv=%1").arg(convs.join(QLatin1Char(','))));
        args.push_back(QStringLiteral("of=%1").arg(destfile));

        process.setStandardInputFile(srcfile);
        return exec(process, program, args, KIO::ERR_CANNOT_WRITE, src);
    }

    KIO::WorkerResult copy(const QUrl &src, const QUrl &dest, int, KIO::JobFlags flags) override
    {
        const bool isSourceLocal = src.isLocalFile();
        const bool isDestinationLocal = dest.isLocalFile();

        // one needs to be local
        if (!isSourceLocal && !isDestinationLocal) {
            return KIO::WorkerResult::fail(KIO::ERR_UNSUPPORTED_ACTION);
        }

        if (isSourceLocal) {
            return copyPut(src, dest, flags);
        }

        auto destfile = dest.toLocalFile();
        if (!(flags & KIO::Overwrite)) {
            // Checks if the destination exists and return an error if it does.
            if (QFile::exists(destfile)) {
                return KIO::WorkerResult::fail(KIO::ERR_FILE_ALREADY_EXIST, destfile);
            }
        }

        auto [program, args] = loadPrefix(src);
        if (program.isEmpty()) {
            return KIO::WorkerResult::fail(KIO::ERR_SERVICE_NOT_AVAILABLE, src.toString());
        }

        QProcess process;
        args += {QLatin1String("cat"), src.path()};

        process.setStandardOutputFile(destfile);
        return exec(process, program, args, KIO::ERR_CANNOT_READ, src);
    }

    KIO::WorkerResult listDir(const QUrl &url) override
    {
        auto [program, args] = loadPrefix(url);
        if (program.isEmpty()) {
            return KIO::WorkerResult::fail(KIO::ERR_SERVICE_NOT_AVAILABLE, url.toString());
        }

        QProcess process;
        // of course, many possible flavors of ls, but let's hope for this one
        // single line, append / to directory, pass all control characters raw
        args += {QLatin1String("ls"), QLatin1String("-a"), QLatin1String("-1"), QLatin1String("-N"), QLatin1String("-p"), url.path()};

        QByteArray data;
        connect(&process, &QProcess::readyReadStandardOutput, this, [&]() {
            data.append(process.readAllStandardOutput());
        });

        auto result = exec(process, program, args, KIO::ERR_CANNOT_OPEN_FOR_READING, url);

        if (result.error())
            return result;

        // indeed, this will not work for filenames with embedded newline
        // but we never claimed this protocol is perfect
        // it should conveniently cover and provide for most common cases
        // also, an error is not fatal nor leads to corruption, etc
        for (const auto &e : data.split('\n')) {
            KIO::UDSEntry entry;
            // last line
            if (e.isEmpty())
                continue;
            int chop = 0;
            if (e.endsWith('/')) {
                chop = 1;
            }
            auto name = QFile::decodeName(e.chopped(chop));
            entry.fastInsert(KIO::UDSEntry::UDS_NAME, name);
            // we do not know if it is really a regular file, so let's not mark it as such
            // which seems to work well enough in practice
            entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, chop ? S_IFDIR : 0);
            listEntry(entry);
        }

        return KIO::WorkerResult::pass();
    }
};

extern "C" {
int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kio_exec"));

    // start the worker
    ExecProtocol worker(argv[1], argv[2], argv[3]);
    worker.dispatchLoop();
    return 0;
}
}

#include "exec.moc"
