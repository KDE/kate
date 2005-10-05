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

#include <kdelibs_export.h>

class QString;
class QRegExp;

namespace KTextEditor
{

class Document;

/**
 * Search interface extension for the Document.
 *
 * <b>Introduction</b>\n
 *
 * The SearchInterface provides methods to search for a given text pattern in
 * a Document. You can either search for a simple text or for a regular
 * expression by using a QRegExp, see searchText().
 *
 * <b>Accessing the SearchInterface</b>\n
 *
 * The SearchInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface @e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * @code
 *   // doc is of type KTextEditor::Document*
 *   KTextEditor::SearchInterface *iface =
 *       qobject_cast<KTextEditor::SearchInterface*>( doc );
 *
 *   if( iface ) {
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * @endcode
 *
 * @see KTextEditor::Document
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT SearchInterface
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~SearchInterface() {}

  public:
    /**
     * Search for the given @p text beginning from @p startPosition taking
     * into account whether to search @p casesensitive and @p backwards.
     *
     * @param startPosition start cursor position
     * @param text text to search for
     * @param casesensitive if @e true, the search is performed case
     *        sensitive, otherwise case insensitive
     * @param backwards if @e true, the search will be backwards
     * @return The valid range of the matched text if @p text was found. If
     *        the @p text was not found, the returned range is not valid
     *        (see Range::isValid()).
     * @see KTextEditor::Range
     */
    virtual KTextEditor::Range searchText (const KTextEditor::Cursor& startPosition,
                                           const QString &text,
                                           bool casesensitive = true,
                                           bool backwards = false) = 0;

    /**
     * Search for the regular expression @p regexp beginning from
     * @p startPosition, if @p backwards is @e true, the search direction will
     * be reversed.
     *
     * @param startPosition start cursor position
     * @param regexp text to search for
     * @param backwards if @e true, the search will be backwards
     * @return The valid range of the matched @p regexp. If the search was not
     *        successful, the returned range is not valid
     *        (see Range::isValid()).
     * @see KTextEditor::Range, QRegExp
     */
    virtual KTextEditor::Range searchText (const KTextEditor::Cursor& startPosition,
                                           const QRegExp &regexp,
                                           bool backwards = false) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SearchInterface, "org.kde.KTextEditor.SearchInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
