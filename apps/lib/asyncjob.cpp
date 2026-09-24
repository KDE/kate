/*
 *    SPDX-FileCopyrightText: 2026 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *
 *    SPDX-License-Identifier: MIT
 */

#include "asyncjob.h"

#include <QPointer>
#include <QRunnable>
#include <QThreadPool>

#include <KTextEditor/Application>
#include <KTextEditor/Editor>

namespace Utils
{
class JobRunner : public QObject, public QRunnable
{
    Q_OBJECT

    std::function<void()> m_run;

public:
    JobRunner(std::function<void()> run)
        : m_run(run)
    {
    }

    void run() override
    {
        m_run();
        Q_EMIT done();
    }

    Q_SIGNAL void done();
};

std::stop_source runAsyncJob(QThreadPool &tp, JobFunction func, const QObject *context, JobResult cb)
{
    std::stop_source ss;

    auto run = [ss, func = std::move(func)]() {
        func(ss.get_token());
    };
    auto job = new JobRunner{run};

    QPointer<const QObject> ctx(context);
    auto done = [ctx, ss, func = std::move(func), cb = std::move(cb)]() {
        bool cancel = !ctx || ss.stop_requested();
        cb(cancel);
    };
    // context is guarded above
    // this signal should always make it through, so use app which should remain around
    auto app = KTextEditor::Editor::instance()->application();
    QObject::connect(job, &JobRunner::done, app, done, Qt::QueuedConnection);
    tp.start(job);

    return ss;
}

}

#include "asyncjob.moc"
