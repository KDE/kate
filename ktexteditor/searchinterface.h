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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ktexteditor_searchinterface_h__
#define __ktexteditor_searchinterface_h__

class QRegExp;
class QString;

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class SearchInterface
{
  friend class PrivateSearchInterface;

  public:
    SearchInterface();
    virtual ~SearchInterface();

    unsigned int searchInterfaceNumber () const;

  //
  // slots !!!
  //
  public:
    virtual bool searchText (unsigned int startLine, unsigned int startCol, const QString &text, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive = true, bool backwards = false) = 0;
    virtual bool searchText (unsigned int startLine, unsigned int startCol, const QRegExp &regexp, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool backwards = false) = 0;

  private:
    class PrivateSearchInterface *d;
    static unsigned int globalSearchInterfaceNumber;
    unsigned int mySearchInterfaceNumber;
};

SearchInterface *searchInterface (class Document *doc);

};

#endif
