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

#ifndef _KATE_REGEXPSEARCH_H_
#define _KATE_REGEXPSEARCH_H_

#include <QtCore/QObject>

#include <ktexteditor/range.h>

#include "katepartprivate_export.h"

namespace KTextEditor {
  class Document;
}

/**
 * Object to help to search for regexp.
 * This should be NO QObject, it is created to often!
 * I measured that, if you create it 20k times to replace for example " " in a document, that takes seconds on a modern machine!
 */
class KATEPART_TESTS_EXPORT KateRegExpSearch
{
  public:
    explicit KateRegExpSearch (KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity);
    ~KateRegExpSearch ();

  //
  // KTextEditor::SearchInterface stuff
  //
  public:
    /**
     * Search for the regular expression \p regexp inside the range
     * \p inputRange. If \p backwards is \e true, the search direction will
     * be reversed.
     *
     * \param regexp text to search for
     * \param inputRange Range to search in
     * \param backwards if \e true, the search will be backwards
     * \return Vector of ranges, one for each capture. The first range (index zero)
     *        spans the full match. If the pattern does not match the vector
     *        has length 1 and holds the invalid range (see Range::isValid()).
     * \see KTextEditor::Range, QRegExp
     */
    QVector<KTextEditor::Range> search (const QString &pattern,
        const KTextEditor::Range & inputRange, bool backwards = false);

    /**
     * Returns a mofified version of text where escape sequences are resolved, e.g. "\\n" to "\n".
     *
     * \param text text containing escape sequences
     * \return text with resolved escape sequences
     */
    static QString escapePlaintext(const QString &text);

    /**
     * Returns a mofified version of text where
     * \li escape sequences are resolved, e.g. "\\n" to "\n",
     * \li references are resolved, e.g. "\\1" to <i>1st entry in capturedTexts</i>, and
     * \li counter sequences are resolved, e.g. "\\#...#" to <i>replacementCounter</i>.
     *
     * \param text text containing escape sequences, references, and counter sequences
     * \param capturedTexts list of substitutes for references
     * \param replacementCounter value for replacement counter
     * \return resolved text
     */
    static QString buildReplacement(const QString &text, const QStringList &capturedTexts, int replacementCounter);

  private:
    /**
     * Implementation of escapePlainText() and public buildReplacement().
     *
     * \param text text containing escape sequences and possibly references and counters
     * \param capturedTexts list of substitutes for references
     * \param replacementCounter value for replacement counter (only used when replacementGoodies == true)
     * \param replacementGoodies <code>true</code> for buildReplacement(), <code>false</code> for escapePlainText()
     * \return resolved text
     */
    static QString buildReplacement(const QString &text, const QStringList &capturedTexts, int replacementCounter, bool replacementGoodies);

  private:
    KTextEditor::Document *const m_document;
    Qt::CaseSensitivity m_caseSensitivity;
    class ReplacementStream;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

