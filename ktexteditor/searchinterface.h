/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_SEARCHINTERFACE_H
#define KDELIBS_KTEXTEDITOR_SEARCHINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/range.h>

class QString;
class QRegExp;

namespace KTextEditor
{

class Document;

/**
 * TODO
 */
namespace Search
{
  enum SearchOptionsEnum
  {
    // plaintext, case-sensitive, no escape
    // sequence processing, whole words off,
    // forward mode, non-block input range
    Default             = 0,

    // modes
    Regex               = 1 << 1,

    // options for all modes
    CaseInsensitive     = 1 << 4,
    Backwards           = 1 << 5,
    BlockInputRange     = 1 << 6,

    // options for plaintext
    EscapeSequences     = 1 << 10,
    WholeWords          = 1 << 11,

    // options for regex
    DotMatchesNewline   = 1 << 15
  };

  Q_DECLARE_FLAGS(SearchOptions, SearchOptionsEnum)
  Q_DECLARE_OPERATORS_FOR_FLAGS(SearchOptions)
}

/**
 * \brief Search interface extension for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section searchiface_intro Introduction
 *
 * The SearchInterface provides methods to search for a given text pattern in
 * a Document. You can either search for a simple text or for a regular
 * expression by using a QRegExp, see searchText().
 *
 * \section searchiface_access Accessing the SearchInterface
 *
 * The SearchInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // doc is of type KTextEditor::Document*
 * KTextEditor::SearchInterface *iface =
 *     qobject_cast<KTextEditor::SearchInterface*>( doc );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Document
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT SearchInterface
{
  public:
    SearchInterface();

    /**
     * Virtual destructor.
     */
    virtual ~SearchInterface();

  public:
    /**
     * TODO
     */
    virtual QVector<KTextEditor::Range> searchText(
        const KTextEditor::Range & range,
        const QString & pattern,
        const Search::SearchOptions options = Search::Default) = 0;

    /**
     * TODO
     */
    virtual Search::SearchOptions supportedSearchOptions() const = 0;

  private:
    class SearchInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SearchInterface, "org.kde.KTextEditor.SearchInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
