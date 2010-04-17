/* This file is part of the KDE libraries
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef kate_cursor_h
#define kate_cursor_h

#include <ktexteditor/cursor.h>



class KateDocument;
namespace KTextEditor { class Attribute; }

/**
  Cursor class with a pointer to its document.
*/
class KateDocCursor : public KTextEditor::Cursor
{
  public:
    explicit KateDocCursor(KateDocument *doc);
    KateDocCursor(const KTextEditor::Cursor &position, KateDocument *doc);
    KateDocCursor(int line, int col, KateDocument *doc);
    virtual ~KateDocCursor() {}

    bool validPosition(int line, int col);
    bool validPosition();

    bool gotoNextLine();
    bool gotoPreviousLine();
    bool gotoEndOfNextLine();
    bool gotoEndOfPreviousLine();

    int nbCharsOnLineAfter();
    bool moveForward(uint nbChar);
    bool moveBackward(uint nbChar);

    bool insertText(const QString& text);
    bool removeText(uint numberOfCharacters);
    QChar currentChar() const;

    uchar currentAttrib() const;

  protected:
    KateDocument *m_doc;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
