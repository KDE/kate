/*
    SPDX-FileCopyrightText: 2026 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include "kateprivate_export.h"

#include <QObject>
#include <QThreadPool>

#include <functional>
#include <stop_token>

namespace Utils
{
using JobFunction = std::function<void(const std::stop_token &)>;
using JobResult = std::function<void(bool cancel)>;

/* Runs func in a worker thread of tp, which can be requested to abort/stop using the return source
 * (though it obviously depends on func how it actually responds to that).
 * When finished, cb is called in mainloop thread, where cancel argument is false iff context is gone or stop requested.
 */
KATE_PRIVATE_EXPORT std::stop_source runAsyncJob(QThreadPool &tp, JobFunction func, const QObject *context, JobResult cb);
}
