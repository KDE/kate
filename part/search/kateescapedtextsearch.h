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

#ifndef _KATE_ESCAPEDTEXTSEARCH_H_
#define _KATE_ESCAPEDTEXTSEARCH_H_

#include <QtCore/QObject>

#include <ktexteditor/range.h>

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

class KateEscapedTextSearch : public QObject
{
  Q_OBJECT

  public:
    explicit KateEscapedTextSearch (KateDocument *document, Qt::CaseSensitivity caseSensitivity, bool wholeWords);
    ~KateEscapedTextSearch ();

  //
  // KTextEditor::SearchInterface stuff
  //
  public Q_SLOTS:
    QVector<KTextEditor::Range> search (const KTextEditor::Range & inputRange,
        const QString &text, bool backwards = false);

  //
  // internal implementation....
  //
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
    static QString escapePlaintext(const QString & text, QList<ReplacementPart> * parts = NULL,
        bool replacementGoodies = false);

  private:
    KateDocument *const m_document;
    Qt::CaseSensitivity m_caseSensitivity;
    bool m_wholeWords;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

