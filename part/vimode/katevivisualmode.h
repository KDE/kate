/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
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

#ifndef KATE_VI_VISUAL_MODE_INCLUDED
#define KATE_VI_VISUAL_MODE_INCLUDED

#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>
#include "katevinormalmode.h"

class KateViRange;
class KateViInputModeManager;

class KateViVisualMode : public KateViNormalMode {

    Q_OBJECT

  public:
    KateViVisualMode( KateViInputModeManager *viInputModeManager, KateView *view, KateViewInternal *viewInternal );
    ~KateViVisualMode();

    void init();

    void setVisualLine( bool l );
    void setVisualBlock( bool l );
    bool isVisualLine() const { return m_mode == VisualLineMode; }
    bool isVisualBlock() const { return m_mode == VisualBlockMode; }
    void switchStartEnd();
    void reset();
    void setVisualModeType( ViMode mode );
    void saveRangeMarks();
    void setStart( const Cursor& c ) { m_start = c; }
    Cursor getStart() { return m_start; }

    void goToPos( const Cursor& c);

    ViMode getLastVisualMode() const { return m_lastVisualMode; }
    Cursor getStart() const { return m_start; }

    // Selects all lines in range;
    void SelectLines(Range range);

    // Selects range between c1 and c2, but includes the end cursor position.
    void SelectInclusive(Cursor c1, Cursor c2);

    // Select block between c1 and c2.
    void SelectBlockInclusive(Cursor c1, Cursor c2);

public Q_SLOTS:
    /**
     * Updates the visual mode's range to reflect a new cursor position. This
     * needs to be called if modifying the range from outside the vi mode, e.g.
     * via mouse selection.
     */
    void updateSelection();

  private:
    void initializeCommands();

    /**
     * Called when a motion/text object is used. Updates the cursor position
     * and modifies the range. A motion will only modify the end of the range
     * (i.e. move the cursor) while a text object may modify both the start and
     * the end.
     */
    void goToPos( const KateViRange &r );
    ViMode m_mode;
    Cursor m_start;
    ViMode m_lastVisualMode; // used when reselecting a visual selection
    bool m_selection_is_changed_inside_ViMode;
};

#endif
