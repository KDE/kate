/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katematch.h"

#include "kateregexpsearch.h"
#include "katedocument.h"
#include <ktexteditor/movingrange.h>

KateMatch::KateMatch(KateDocument *document, KTextEditor::Search::SearchOptions options)
    : m_document(document)
    , m_options(options)
{
    m_resultRanges.append(KTextEditor::Range::invalid());
}


KTextEditor::Range KateMatch::searchText(const KTextEditor::Range &range, const QString &pattern)
{
    m_resultRanges = m_document->searchText(range, pattern, m_options);

    return m_resultRanges[0];
}


KTextEditor::Range KateMatch::replace(const QString &replacement, bool blockMode, int replacementCounter)
{
    // Placeholders depending on search mode
    const bool usePlaceholders = m_options.testFlag(KTextEditor::Search::Regex) ||
                                 m_options.testFlag(KTextEditor::Search::EscapeSequences);

    const QString finalReplacement = usePlaceholders ? buildReplacement(replacement, blockMode, replacementCounter)
                                                     : replacement;

    // Track replacement operation
    KTextEditor::MovingRange *const afterReplace = m_document->newMovingRange(range(), KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight);

    blockMode = blockMode && !range().onSingleLine();
    m_document->replaceText(range(), finalReplacement, blockMode);

    const KTextEditor::Range result = *afterReplace;
    delete afterReplace;

    return result;
}


KTextEditor::Range KateMatch::range() const
{
    if (m_resultRanges.size() > 0)
        return m_resultRanges[0];

    return KTextEditor::Range::invalid();
}


bool KateMatch::isEmpty() const
{
    return range().isEmpty();
}


bool KateMatch::isValid() const
{
    return range().isValid();
}


QString KateMatch::buildReplacement(const QString &replacement, bool blockMode, int replacementCounter) const {
    QStringList capturedTexts;
    foreach (const KTextEditor::Range &captureRange, m_resultRanges) {
        // Copy capture content
        capturedTexts << m_document->text(captureRange, blockMode);
    }

    return KateRegExpSearch::buildReplacement(replacement, capturedTexts, replacementCounter);
}


// kate: space-indent on; indent-width 4; replace-tabs on;
