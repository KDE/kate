/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
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
 * This is an interface to search throught a KTextEditor::Document object.
 * It allows to search for given text or regular expression
 */
class KTEXTEDITOR_EXPORT SearchInterface
{
  public:
    /**
     * Virtual Destructor
     */
    virtual ~SearchInterface() {}

  public:
    /**
     * search for given text
     * @param startLine line to start search
     * @param startCol column to start search
     * @param text text to search for
     * @param casesensitive should we search casesensitive?
     * @param backwards should we search backwards?
     * @return valid range of match if text found, otherwise inValid() range.
     */
    virtual KTextEditor::Range searchText (const KTextEditor::Cursor& startPosition, const QString &text, bool casesensitive = true, bool backwards = false) = 0;

    /**
     * search for given regular expression
     * @param startLine line to start search
     * @param startCol column to start search
     * @param regexp expression to search for
     * @param foundAtLine line where match is found
     * @param foundAtCol line where match is found
     * @param matchLen match length
     * @param backwards should we search backwards?
     * @return valid range of match if text found, otherwise inValid() range.
     */
    virtual KTextEditor::Range searchText (const KTextEditor::Cursor& startPosition, const QRegExp &regexp, bool backwards = false) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SearchInterface, "org.kde.KTextEditor.SearchInterface")

#endif
