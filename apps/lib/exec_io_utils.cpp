/*
 *    SPDX-FileCopyrightText: 2025 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *
 *    SPDX-License-Identifier: MIT
 */

#include "exec_io_utils.h"

#include <QMap>
#include <QSaveFile>
#include <QTemporaryFile>

#include "kate_exec_debug.h"

namespace Utils
{
static constexpr char ENV_KATE_EXEC_DATA[] = "KATE_EXEC_DATA";

class ExecPrefixManager::ExecPrefixManagerPrivate
{
    QMap<QString, QStringList> m_args;
    QTemporaryFile m_workerConfig;
    QString m_workerConfigPath;

public:
    ExecPrefixManagerPrivate()
    {
        // prime temp file
        m_workerConfig.open();
        m_workerConfigPath = m_workerConfig.fileName();
        m_workerConfig.close();

        // provide forked workers with info where to find config
        qputenv(ENV_KATE_EXEC_DATA, m_workerConfigPath.toLocal8Bit());

        // NOTE alternatively, WorkerConfig might be an option
        // but looks like that is not exposed by KIO
        // so we have to come up with our own
        // another option might be to update the KConfig rc file
        // but not yet for now
    }

    void update_config()
    {
        QSaveFile save(m_workerConfigPath);
        // QTemporaryFile tmp;
        // tmp.open();
        save.open(QFile::WriteOnly);
        QDataStream stream(&save);
        QMap<QString, QStringList> data;
        stream << m_args;
        auto ret = save.commit();
        qCDebug(LibKateExec) << "save exec config" << data << ret;
    }

    void update(QString name, QStringList args)
    {
        // FIXME check for different args for same name ??
        // it depends on whether from same or different origin
        // -> undefined behaviour, no diagnostic required
        m_args[name] = args;
        update_config();
    }
};

ExecPrefixManager &ExecPrefixManager::instance()
{
    static auto instance = new ExecPrefixManager();
    return *instance;
}

ExecPrefixManager::ExecPrefixManager()
    : d(new ExecPrefixManagerPrivate())
{
}

ExecPrefixManager::~ExecPrefixManager()
{
    delete d;
}

void ExecPrefixManager::update(QString name, QStringList args)
{
    return d->update(name, args);
}

QStringList ExecPrefixManager::load(QString name)
{
    auto fname = qEnvironmentVariable(ENV_KATE_EXEC_DATA);
    qCDebug(LibKateExec) << "in worker" << fname;
    QFile fdata(fname);
    fdata.open(QFile::ReadOnly);
    QDataStream stream(&fdata);
    QMap<QString, QStringList> data;
    stream >> data;

    auto it = data.find(name);
    if (it != data.end()) {
        auto args = *it;
        if (args.size()) {
            qCDebug(LibKateExec) << "found" << args;
            return args;
        }
    }

    return {};
}

}
