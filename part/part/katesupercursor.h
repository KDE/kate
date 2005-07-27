/* This file is part of the KDE libraries
   Copyright (C) 2003,2004,2005 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATESUPERCURSOR_H
#define KATESUPERCURSOR_H

#include "katecursor.h"

#include <QObject>

class KateDocument;
class KateView;

/**
 * TODO:
 *  - use connect/disconnectNotify to know when to be bothered tracking/emitting positions.
 *
 * Possible additional features:
 * - Notification when a cursor enters or exits a view
 * - suggest something :)
 *
 * Unresolved issues:
 * - testing, testing, testing
 *   - ie. everything which hasn't already been tested, you can see that which has inside the #ifdefs
 * - api niceness
 */

/**
 * A cursor which updates and gives off various interesting signals.
 *
 * This aims to be a working version of what KateCursor was originally intended to be.
 *
 * @author Hamish Rodda
 **/
class KateSuperCursor : public QObject, public KateDocCursor
{
  Q_OBJECT

public:
    /**
     * bool privateC says: if private, than don't show to apps using the cursorinterface in the list,
     * all internally only used SuperCursors should be private or people could modify them from the
     * outside breaking kate's internals
     */
    KateSuperCursor(KateDocument* doc, bool privateC, const KTextEditor::Cursor& cursor, QObject* parent = 0L, const char* name = 0L);
    KateSuperCursor(KateDocument* doc, bool privateC, int lineNum = 0, int col = 0, QObject* parent = 0L, const char* name = 0L);
    virtual ~KateSuperCursor();

    bool insertText(const QString& text);
    bool removeText(uint numberOfCharacters);
    QChar currentChar() const;

    /**
    * @returns true if the cursor is situated at the start of the line, false if it isn't.
    */
    bool atStartOfLine() const;

    /**
    * @returns true if the cursor is situated at the end of the line, false if it isn't.
    */
    bool atEndOfLine() const;

    /**
    * Returns how this cursor behaves when text is inserted at the cursor.
    * Defaults to not moving on insert.
    */
    bool moveOnInsert() const;

    /**
    * Change the behavior of the cursor when text is inserted at the cursor.
    *
    * If @p moveOnInsert is true, the cursor will end up at the end of the insert.
    */
    void setMoveOnInsert(bool moveOnInsert);

    /**
    * Debug: output the position.
    */
    operator QString();
    inline KateSuperCursor& operator= (const KTextEditor::Cursor& rhs) { setPosition(rhs); return *this; }

    // Reimplementations;
    virtual void setPosition(const KTextEditor::Cursor& pos);
    virtual void setPosition (int line, int column) { setPosition(Cursor (line, column)); }
    virtual void setLine (int line) { setPosition(Cursor (line, column())); }
    virtual void setColumn (int column) { setPosition(Cursor (line(), column)); }

    /**
     * Returns the document this cursor is attached to...
     */
    KateDocument* doc() const;

  signals:
    /**
    * The cursor's position was directly changed by the program.
    */
    void positionDirectlyChanged();

    /**
    * The cursor's position was changed.
    */
    void positionChanged();

    /**
    * Athough an edit took place, the cursor's position was unchanged.
    */
    void positionUnChanged();

    /**
    * The cursor's surrounding characters were both deleted simultaneously.
    * The cursor is automatically placed at the start of the deleted region.
    */
    void positionDeleted();

    /**
    * A character was inserted immediately before the cursor.
    *
    * Whether the char was inserted before or after this cursor depends on
    * moveOnInsert():
    * @li true -> the char was inserted before
    * @li false -> the char was inserted after
    */
    void charInsertedAt();

    /**
    * The character immediately before the cursor was deleted.
    */
    void charDeletedBefore();

    /**
    * The character immediately after the cursor was deleted.
    */
    void charDeletedAfter();

  // BEGIN METHODS TO CALL FROM KATE DOCUMENT TO KEEP CURSOR UP TO DATE
  public:
    void editTextInserted ( uint line, uint col, uint len);
    void editTextRemoved ( uint line, uint col, uint len);

    void editLineWrapped ( uint line, uint col, bool newLine = true );
    void editLineUnWrapped ( uint line, uint col, bool removeLine = true, uint length = 0 );

    void editLineInserted ( uint line );
    void editLineRemoved ( uint line );
  // END

  private:
    KateDocument *m_doc;
    bool m_moveOnInsert : 1;
    bool m_lineRemoved : 1;
    bool m_privateCursor : 1;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
