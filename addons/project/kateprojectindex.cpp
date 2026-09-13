/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectindex.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

#include "asyncjob.h"
#include "hostprocess.h"

/**
 * include ctags reading
 */
#include "ctags/readtags.c"

using tagFilePtr = std::shared_ptr<tagFile>;

class KateProjectIndex::KateProjectIndexPrivate
{
public:
    KateProjectIndex *q;
    std::vector<tagFilePtr> m_tagsIndexHandles;

    KateProjectIndexPrivate(KateProjectIndex *self)
        : q(self)
    {
    }

    tagFilePtr takeHandle()
    {
        if (m_tagsIndexHandles.empty()) {
            auto deleter = [](tagFile *h) {
                tagsClose(h);
            };
            tagFileInfo info;
            memset(&info, 0, sizeof(tagFileInfo));
            auto h = tagsOpen(q->m_ctagsIndexFile->fileName().toLocal8Bit().constData(), &info);
            m_tagsIndexHandles.push_back(std::shared_ptr<tagFile>(h, deleter));
        }

        auto ptr = m_tagsIndexHandles.back();
        m_tagsIndexHandles.pop_back();
        return ptr;
    }

    void putHandle(tagFilePtr t)
    {
        m_tagsIndexHandles.push_back(t);
    }

    void clearHandles()
    {
        // also arranges for proper close (see above)
        m_tagsIndexHandles.clear();
    }
};

KateProjectIndex::KateProjectIndex(const QString &baseDir, const QString &indexDir, const QStringList &files, const QVariantMap &ctagsMap, bool force)
    : d(new KateProjectIndexPrivate(this))
{
    // allow project to override and specify a (re-usable) indexfile
    // otherwise fall-back to a temporary file if nothing specified
    auto ctagsFile = ctagsMap.value(QStringLiteral("index_file"));
    if (ctagsFile.userType() == QMetaType::QString) {
        auto path = ctagsFile.toString();
        if (!QDir::isAbsolutePath(path)) {
            path = QDir(baseDir).absoluteFilePath(path);
        }
        m_ctagsIndexFile = std::make_unique<QFile>(path);
    } else {
        if (baseDir == QDir::homePath() || baseDir == QDir::rootPath()) {
            // avoid dumb stuff, dont index the full home/root dir
            return;
        }
        // indexDir is typically QDir::tempPath() or otherwise specified in configuration
        m_ctagsIndexFile.reset(new QTemporaryFile(
            indexDir + QStringLiteral("/kate.project.ctags.%1.%2").arg(QDir(baseDir).dirName(), QString::number(QCoreApplication::applicationPid()))));
    }

    /**
     * load ctags
     */
    loadCtags(files, ctagsMap, force);
}

KateProjectIndex::~KateProjectIndex() = default;

void KateProjectIndex::loadCtags(const QStringList &files, const QVariantMap &ctagsMap, bool force)
{
    /**
     * only overwrite existing index upon reload
     * (a temporary index file will never exist)
     */
    if (m_ctagsIndexFile->exists() && !force) {
        openCtags();
        return;
    }

    /**
     * create temporary file
     * if not possible, fail
     */
    if (!m_ctagsIndexFile->open(QIODevice::ReadWrite)) {
        return;
    }

    /**
     * close file again, other process will use it
     */
    m_ctagsIndexFile->close();

    // only use ctags from PATH
    static const auto fullExecutablePath = safeExecutableName(QStringLiteral("ctags"));
    if (fullExecutablePath.isEmpty()) {
        return;
    }

    /**
     * try to run ctags for all files in this project
     * output to our ctags index file
     */
    QProcess ctags;
    QStringList args;
    args << QStringLiteral("-L") << QStringLiteral("-") << QStringLiteral("-f") << m_ctagsIndexFile->fileName() << QStringLiteral("--fields=+K+n");
    const QString keyOptions = QStringLiteral("options");
    const auto opts = ctagsMap[keyOptions].toList();
    for (const QVariant &optVariant : opts) {
        args << optVariant.toString();
    }
    startHostProcess(ctags, fullExecutablePath, args);
    if (!ctags.waitForStarted()) {
        return;
    }

    /**
     * write files list and close write channel
     */
    ctags.write(files.join(u'\n').toLocal8Bit());
    ctags.closeWriteChannel();

    /**
     * wait for done
     */
    if (!ctags.waitForFinished(-1)) {
        return;
    }

    openCtags();
}

void KateProjectIndex::openCtags()
{
    /**
     * file not openable, bad
     */
    if (!m_ctagsIndexFile->open(QIODevice::ReadOnly)) {
        return;
    }

    /**
     * get size
     */
    qint64 size = m_size = m_ctagsIndexFile->size();

    /**
     * close again
     */
    m_ctagsIndexFile->close();

    /**
     * empty file, bad
     */
    if (!size) {
        return;
    }

    /**
     * close current
     */
    d->clearHandles();

    /**
     * try to open ctags file
     */
    d->putHandle(d->takeHandle());
}

static void
findMatches(tagFile *tags, QStandardItemModel &model, const QString &searchWord, KateProjectIndex::MatchType type, int options, std::stop_token stop)
{
    /**
     * abort if no ctags index
     */
    if (!tags) {
        return;
    }

    if (stop.stop_requested()) {
        return;
    }

    /**
     * word to complete
     * abort if empty
     */
    QByteArray word = searchWord.toLocal8Bit();
    if (word.isEmpty()) {
        return;
    }

    /**
     * try to search entry
     * fail if none found
     */
    tagEntry entry;
    if (options == -1) {
        options = TAG_PARTIALMATCH | TAG_OBSERVECASE;
    }
    if (tagsFind(tags, &entry, word.constData(), options) != TagSuccess) {
        return;
    }

    /**
     * set to show words only once for completion matches
     */
    QSet<QString> guard;

    /**
     * loop over all found tags
     * first one is filled by above find, others by find next
     */
    do {
        /**
         * skip if no name
         */
        if (!entry.name) {
            continue;
        }

        /**
         * get name
         */
        QString name(QString::fromLocal8Bit(entry.name));

        /**
         * construct right items
         */
        switch (type) {
        case KateProjectIndex::CompletionMatches:
            /**
             * add new completion item, if new name
             */
            if (!guard.contains(name)) {
                model.appendRow(new QStandardItem(name));
                guard.insert(name);
            }
            break;

        case KateProjectIndex::FindMatches:
            /**
             * add new find item, contains of multiple columns
             */
            QList<QStandardItem *> items;
            items << new QStandardItem(name);
            items << new QStandardItem(entry.kind ? QString::fromLocal8Bit(entry.kind) : QString());
            items << new QStandardItem(entry.file ? QString::fromLocal8Bit(entry.file) : QString());
            items << new QStandardItem(QString::number(entry.address.lineNumber));
            model.appendRow(items);
            break;
        }
    } while (!stop.stop_requested() && tagsFindNext(tags, &entry) == TagSuccess);
}

void KateProjectIndex::findMatches(QStandardItemModel &model, const QString &searchWord, MatchType type, int options)
{
    /* avoid tying down mainloop in expensive lookup */
    if (m_size > 50 * 1024 * 1024 && type == CompletionMatches) {
        return;
    }

    auto th = d->takeHandle();
    ::findMatches(th.get(), model, searchWord, type, options, {});
    d->putHandle(th);
}

std::stop_source KateProjectIndex::findMatchesAsync(const QObject *context,
                                                    std::function<void(QStandardItemModel &&)> cb,
                                                    const QString &searchWord,
                                                    MatchType type,
                                                    bool automatic,
                                                    int options)
{
    // this will probably take a long time, so let's forego this if not explicitly requested
    bool skipFind = m_size > 50 * 1024 * 1024 && type == CompletionMatches && automatic;

    auto model = std::make_shared<QStandardItemModel>();
    auto th = d->takeHandle();
    auto match = [tags = th, model, searchWord, type, options, skipFind](const std::stop_token &token) {
        if (!skipFind)
            ::findMatches(tags.get(), *model, searchWord, type, options, token);
    };
    auto done = [d = d, th, model, cb = std::move(cb)](bool cancel) {
        // try to return handle to cache
        d->putHandle(th);
        if (!cancel)
            cb(std::move(*model));
    };

    return Utils::runAsyncJob(*QThreadPool::globalInstance(), match, context, done);
}

// #include "kateprojectindex.moc"
