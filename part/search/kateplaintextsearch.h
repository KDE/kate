/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2009-2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>
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

#ifndef _KATE_PLAINTEXTSEARCH_H_
#define _KATE_PLAINTEXTSEARCH_H_

#include <QtCore/QObject>

#include <ktexteditor/range.h>

#include "katepartprivate_export.h"

namespace KTextEditor {
  class Document;
}

class KATEPART_TESTS_EXPORT KatePlainTextSearch : public QObject
{
  Q_OBJECT

  public:
    explicit KatePlainTextSearch (KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity, bool wholeWords);
    ~KatePlainTextSearch ();

  public:
    /**
     * Search for the given \p text inside the range \p inputRange taking
     * into account whether to search \p casesensitive and \p backwards.
     *
     * \param text text to search for
     * \param inputRange Range to search in
     * \param casesensitive if \e true, the search is performed case
     *        sensitive, otherwise case insensitive
     * \param backwards if \e true, the search will be backwards
     * \return The valid range of the matched text if \p text was found. If
     *        the \p text was not found, the returned range is not valid
     *        (see Range::isValid()).
     * \see KTextEditor::Range
     */
    KTextEditor::Range search (const QString & text,
        const KTextEditor::Range & inputRange, bool backwards = false);

  private:
    KTextEditor::Document *m_document;
    Qt::CaseSensitivity m_caseSensitivity;
    bool m_wholeWords;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

