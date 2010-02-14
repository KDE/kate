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

#ifndef _KATE_REGEXPSEARCH_H_
#define _KATE_REGEXPSEARCH_H_

#include <QtCore/QObject>

#include "kateregexp.h"

#include <ktexteditor/range.h>

namespace KTextEditor {
  class Document;
}

class KateRegExpSearch : public QObject
{
  Q_OBJECT

  public:
    explicit KateRegExpSearch (KTextEditor::Document *document, Qt::CaseSensitivity caseSensitivity);
    ~KateRegExpSearch ();

  //
  // KTextEditor::SearchInterface stuff
  //
  public Q_SLOTS:
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

  private:
    KTextEditor::Document *const m_document;
    Qt::CaseSensitivity m_caseSensitivity;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

