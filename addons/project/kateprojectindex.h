/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_PROJECT_INDEX_H
#define KATE_PROJECT_INDEX_H

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QStringList>
#include <QTemporaryFile>
#include <QStandardItemModel>

/**
 * ctags reading
 */
#include "ctags/readtags.h"

/**
 * Class representing the index of a project.
 * This includes knowledge from ctags and Co.
 * Allows you to search for stuff and to get some useful auto-completion.
 * Is created in Worker thread in the background, then passed to project in
 * the main thread for usage.
 */
class KateProjectIndex
{
public:
    /**
     * construct new index for given files
     * @param files files to index
     * @param ctagsMap ctags section for extra options
     */
    KateProjectIndex(const QStringList &files, const QVariantMap &ctagsMap);

    /**
     * deconstruct project
     */
    ~KateProjectIndex();

    /**
     * Which kind of match items should be created in the passed model
     * of the findMatches function?
     */
    enum MatchType {
        /**
         * Completion matches, containing only name and same name only once
         */
        CompletionMatches,

        /**
         * Find matches, containing name, kind, file, line, ...
         */
        FindMatches
    };

    /**
     * Fill in completion matches for given view/range.
     * Uses e.g. ctags index.
     * @param model model to fill with matches
     * @param searchWord word to search for
     * @param type type of matches
     */
    void findMatches(QStandardItemModel &model, const QString &searchWord, MatchType type);

    /**
     * Check if running ctags was successful. This can be used
     * as indicator whether ctags is installed or not.
     * @return true if a valid index exists, otherwise false
     */
    bool isValid() const {
        return m_ctagsIndexHandle;
    }

private:
    /**
     * Load ctags tags.
     * @param files files to index
     * @param ctagsMap ctags section for extra options
     */
    void loadCtags(const QStringList &files, const QVariantMap &ctagsMap);

private:
    /**
     * ctags index file
     */
    QTemporaryFile m_ctagsIndexFile;

    /**
     * handle to ctags file for querying, if possible
     */
    tagFile *m_ctagsIndexHandle;
};

#endif

