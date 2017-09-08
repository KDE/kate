/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprojectindex.h"

#include <QProcess>
#include <QDir>

/**
 * include ctags reading
 */
#include "ctags/readtags.c"

KateProjectIndex::KateProjectIndex(const QStringList &files, const QVariantMap &ctagsMap)
    : m_ctagsIndexFile(QDir::tempPath() + QStringLiteral("/kate.project.ctags"))
    , m_ctagsIndexHandle(nullptr)
{
    /**
     * load ctags
     */
    loadCtags(files, ctagsMap);
}

KateProjectIndex::~KateProjectIndex()
{
    /**
     * delete ctags handle if any
     */
    if (m_ctagsIndexHandle) {
        tagsClose(m_ctagsIndexHandle);
        m_ctagsIndexHandle = nullptr;
    }
}

void KateProjectIndex::loadCtags(const QStringList &files, const QVariantMap &ctagsMap)
{
    /**
     * create temporary file
     * if not possible, fail
     */
    if (!m_ctagsIndexFile.open()) {
        return;
    }

    /**
     * close file again, other process will use it
     */
    m_ctagsIndexFile.close();

    /**
     * try to run ctags for all files in this project
     * output to our ctags index file
     */
    QProcess ctags;
    QStringList args;
    args << QStringLiteral("-L") << QStringLiteral("-") << QStringLiteral("-f") << m_ctagsIndexFile.fileName() << QStringLiteral("--fields=+K+n");
    const QString keyOptions = QStringLiteral("options");
    for (const QVariant &optVariant : ctagsMap[keyOptions].toList()) {
        args << optVariant.toString();
    }
    ctags.start(QStringLiteral("ctags"), args);
    if (!ctags.waitForStarted()) {
        return;
    }

    /**
     * write files list and close write channel
     */
    ctags.write(files.join(QStringLiteral("\n")).toLocal8Bit());
    ctags.closeWriteChannel();

    /**
     * wait for done
     */
    if (!ctags.waitForFinished()) {
        return;
    }

    /**
     * file not openable, bad
     */
    if (!m_ctagsIndexFile.open()) {
        return;
    }

    /**
     * get size
     */
    qint64 size = m_ctagsIndexFile.size();

    /**
     * close again
     */
    m_ctagsIndexFile.close();

    /**
     * empty file, bad
     */
    if (!size) {
        return;
    }

    /**
     * try to open ctags file
     */
    tagFileInfo info;
    memset(&info, 0, sizeof(tagFileInfo));
    m_ctagsIndexHandle = tagsOpen(m_ctagsIndexFile.fileName().toLocal8Bit().constData(), &info);
}

void KateProjectIndex::findMatches(QStandardItemModel &model, const QString &searchWord, MatchType type)
{
    /**
     * abort if no ctags index
     */
    if (!m_ctagsIndexHandle) {
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
    if (tagsFind(m_ctagsIndexHandle, &entry, word.constData(), TAG_PARTIALMATCH  | TAG_OBSERVECASE) != TagSuccess) {
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
        case CompletionMatches:
            /**
             * add new completion item, if new name
             */
            if (!guard.contains(name)) {
                model.appendRow(new QStandardItem(name));
                guard.insert(name);
            }
            break;

        case FindMatches:
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
    } while (tagsFindNext(m_ctagsIndexHandle, &entry) == TagSuccess);
}

