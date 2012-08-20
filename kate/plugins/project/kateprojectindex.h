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
     */
    KateProjectIndex (const QStringList &files);

    /**
     * deconstruct project
     */
    ~KateProjectIndex ();

    /**
     * Fill in completion matches for given view/range.
     * Uses e.g. ctags index.
     * @param model model to fill with completion matches
     * @param view view we complete for
     * @param range range we complete for
     */
    void completionMatches (QStandardItemModel &model, KTextEditor::View *view, const KTextEditor::Range & range);
    
  private:
    /**
     * Load ctags tags.
     * @param files files to index
     */
    void loadCtags (const QStringList &files);
    
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

// kate: space-indent on; indent-width 2; replace-tabs on;
