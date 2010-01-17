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

#ifndef _KATE_PLAINTEXTSEARCH_H_
#define _KATE_PLAINTEXTSEARCH_H_

#include <QtCore/QObject>

#include <ktexteditor/range.h>

class KateDocument;

class KatePlainTextSearch : public QObject
{
  Q_OBJECT

  public:
    explicit KatePlainTextSearch (KateDocument *document, Qt::CaseSensitivity caseSensitivity, bool wholeWords);
    ~KatePlainTextSearch ();

  //
  // KTextEditor::SearchInterface stuff
  //
  public Q_SLOTS:
    QVector<KTextEditor::Range> search (const KTextEditor::Range & inputRange,
        const QString & text, bool backwards = false);

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
        const QString & text, bool backwards = false);

  private:
    KateDocument *m_document;
    Qt::CaseSensitivity m_caseSensitivity;
    bool m_wholeWords;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

