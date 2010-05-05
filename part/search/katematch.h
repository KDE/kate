/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
 *  Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>
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

#ifndef KATE_MATCH_H
#define KATE_MATCH_H

#include <ktexteditor/searchinterface.h>

class KateDocument;


class KateMatch
{
public:
    KateMatch(KateDocument *document, KTextEditor::Search::SearchOptions options);
    KTextEditor::Range searchText(const KTextEditor::Range &range, const QString &pattern);
    KTextEditor::Range replace(const QString &replacement, bool blockMode, int replacementCounter = 1);
    bool isValid() const;
    bool isEmpty() const;
    KTextEditor::Range range() const;

private:
    /**
     * Resolve references and escape sequences.
     */
    QString buildReplacement(const QString &replacement, bool blockMode, int replacementCounter) const;

private:
    KateDocument *const m_document;
    const KTextEditor::Search::SearchOptions m_options;
    QVector<KTextEditor::Range> m_resultRanges;
};

#endif // KATE_MATCH_H

// kate: space-indent on; indent-width 4; replace-tabs on;
