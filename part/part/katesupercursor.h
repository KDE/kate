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

#include <ktexteditor/cursor.h>

#include <QObject>

#include "kateedit.h"

class KateDocument;
class KateView;
class KateSmartGroup;
class KateSmartCursorNotifier;
namespace KTextEditor { class Document; }

/**
 * Possible additional features:
 * - Notification when a cursor enters or exits a view
 * - suggest something :)
 *
 * Unresolved issues:
 * - testing, testing, testing
 * - api niceness
 */

/**
 * Internal implementation of KTextEditor::SmartCursor
 *
 * @author Hamish Rodda
 **/
class KateSmartCursor : public KTextEditor::SmartCursor
{
  public:
    KateSmartCursor(const KTextEditor::Cursor& position, KTextEditor::Document* doc, bool moveOnInsert = true);
    /// \overload
    KateSmartCursor(KTextEditor::Document* doc, bool moveOnInsert = true);
    virtual ~KateSmartCursor();

    /**
    * Debug: output the position.
    */
    operator QString();

    KateDocument* kateDocument() const;
    inline KateSmartCursor& operator= (const KTextEditor::Cursor& rhs) { setPosition(rhs); return *this; }

    // Reimplementations;
    virtual int line() const;
    virtual void setLine(int line);
    virtual void setPosition(const KTextEditor::Cursor& pos);

    virtual bool isValid() const;
    virtual bool isValid(const Cursor& position) const;
    virtual bool atEndOfLine() const;

    virtual KTextEditor::SmartCursorNotifier* notifier();
    virtual void deleteNotifier();
    virtual void setWatcher(KTextEditor::SmartCursorWatcher* watcher = 0L);

    inline bool feedbackEnabled() const { return m_feedbackEnabled; }

    inline KateSmartGroup* smartGroup() const { return m_smartGroup; }
    void migrate(KateSmartGroup* newGroup);

    bool translate(const KateEditInfo& edit);

    /**
     * This is a function solely for use by KateSuperRange.  Used to see if a
     * change in position has occurred since it was last called.
     */
    bool cursorMoved() const;

    /**
     * If the cursor is requesting feedback, this is called whenever the
     * cursor's position moves and it's not needing to be adjusted via
     * translateCursor().
     */
    void translated(const KateEditInfo & edit);

    // Called when the cursor's position has changed only (character changes not possible)
    void shifted();

  protected:
    void setLineInternal(int newLine, bool internal = true);
    void setPositionInternal(const KTextEditor::Cursor& pos, bool internal = true);
    virtual void checkFeedback();

  /* BEGIN METHODS TO CALL FROM KATE DOCUMENT TO KEEP CURSOR UP TO DATE
  public:
    void editTextInserted ( uint line, uint col, uint len);
    void editTextRemoved ( uint line, uint col, uint len);

    void editLineWrapped ( uint line, uint col, bool newLine = true );
    void editLineUnWrapped ( uint line, uint col, bool removeLine = true, uint length = 0 );

    void editLineInserted ( uint line );
    void editLineRemoved ( uint line );
  // END */

  private:
    KateSmartGroup* m_smartGroup;
    bool m_feedbackEnabled  :1;
    mutable int m_oldGroupLineStart;

    /**
     * Cursor which stores the previous position of this cursor.
     * Not guaranteed to be up to date - only up to date when the smartGroup that this cursor
     * is in receives an edit.
     */
    Cursor m_lastPosition;

    KateSmartCursorNotifier* m_notifier;
    KTextEditor::SmartCursorWatcher* m_watcher;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
