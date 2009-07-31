/* This file is part of the KDE libraries
   Copyright (C) 2009 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
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

#ifndef _KATE_SEARCH_H_
#define _KATE_SEARCH_H_

#include <QtCore/QObject>

#include <ktexteditor/range.h>
#include <ktexteditor/searchinterface.h>

#include <QRegExp>

class KateDocument;

// needed for parsing replacement text like "\1:\2"
struct ReplacementPart {
  enum Type {
    Reference, // \1..\9
    Text,
    UpperCase, // \U = Uppercase from now on
    LowerCase, // \L = Lowercase from now on
    KeepCase, // \E = back to original case
    Counter // \# = 1, 2, ... incremented for each replacement of <Replace All>
  };

  Type type;

  // Type in {Reference, Counter}
  int index; // [0..9] 0=full match, 1=first capture, ..

  // Type = Text
  QString text;
};

class KateSearch : public QObject
{
  Q_OBJECT

  public:
    explicit KateSearch (KateDocument *document);
    ~KateSearch ();

  //
  // KTextEditor::SearchInterface stuff
  //
  public Q_SLOTS:
    QVector<KTextEditor::Range> searchText(
        const KTextEditor::Range & range,
        const QString & pattern,
        const KTextEditor::Search::SearchOptions options);

    static KTextEditor::Search::SearchOptions supportedSearchOptions();

  //
  // internal implementation....
  //
  private:
    /**
     * Search for the given \p text inside the range \p inputRange taking
     * into account whether to search \p casesensitive and \p backwards.
     *
     * \param inputRange Range to search in
     * \param text text to search for
     * \param casesensitive if \e true, the search is performed case
     *        sensitive, otherwise case insensitive
     * \param backwards if \e true, the search will be backwards
     * \return The valid range of the matched text if \p text was found. If
     *        the \p text was not found, the returned range is not valid
     *        (see Range::isValid()).
     * \see KTextEditor::Range
     */
    KTextEditor::Range searchText (const KTextEditor::Range & inputRange,
        const QString &text, bool casesensitive = true, bool backwards = false);

    /**
     * Search for the regular expression \p regexp inside the range
     * \p inputRange. If \p backwards is \e true, the search direction will
     * be reversed.
     *
     * \param inputRange Range to search in
     * \param regexp text to search for
     * \param backwards if \e true, the search will be backwards
     * \return Vector of ranges, one for each capture. The first range (index zero)
     *        spans the full match. If the pattern does not match the vector
     *        has length 1 and holds the invalid range (see Range::isValid()).
     * \see KTextEditor::Range, QRegExp
     */
    QVector<KTextEditor::Range> searchRegex (const KTextEditor::Range & inputRange,
        QRegExp & regexp, bool backwards = false);

  /*
   * Public string processing helpers
   */
  public:
    /**
     * Resolves escape sequences (e.g. "\\n" to "\n") in <code>text</code>
     * if <code>parts</code> is NULL. Otherwise it leaves code>text</code>
     * unmodified and creates a list of text and capture references out of it.
     * These two modes are fused into one function to avoid code duplication.
     *
     * \param text                Text to process
     * \param parts               List of text and references
     * \param replacementGoodies  Enable \L, \E, \E and \#
     */
    static void escapePlaintext(QString & text, QList<ReplacementPart> * parts = NULL,
        bool replacementGoodies = false);

    /**
     * Repairs a regular Expression pattern.
     * This is a workaround to make "." and "\s" not match
     * newlines, which currently is the unconfigurable
     * default in QRegExp.
     *
     * \param pattern         Regular expression
     * \param stillMultiLine  Multi-line after reparation flag
     * \return                Number of replacements done
     */
    static int repairPattern(QString & pattern, bool & stillMultiLine);

  /*
   * Private string processing helpers
   */
  private:
    /**
     * This function is a replacement for QRegExp.lastIndexIn that
     * returns the last match that would have been found when
     * searching forwards, which QRegExp.lastIndexIn does not.
     * We need this behavior to allow the user to jump back to
     * the last match.
     *
     * \param matcher    QRegExp matcher to use
     * \param str        Text to search in
     * \param offset     Offset (-1 starts from end, -2 from one before the end)
     * \param caretMode  Meaning of caret (^) in the regular expression
     * \return           Index of match or -1 if no match is found
     */
    static int fixedLastIndexIn(const QRegExp & matcher, const QString & str,
        int offset = -1, QRegExp::CaretMode caretMode = QRegExp::CaretAtZero);

  private:
    KateDocument *const m_document;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

