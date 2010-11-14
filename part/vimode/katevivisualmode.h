/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
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
#include "katevinormalmode.h"

class KateViRange;
class KateViInputModeManager;

class KateViVisualMode : public KateViNormalMode {
  public:
    KateViVisualMode( KateViInputModeManager *viInputModeManager, KateView *view, KateViewInternal *viewInternal );
    ~KateViVisualMode();

    void init();

    void setVisualLine( bool l );
    void setVisualBlock( bool l );
    bool isVisualLine() const { return m_mode == VisualLineMode; }
    bool isVisualBlock() const { return m_mode == VisualBlockMode; }
    void switchStartEnd();
    void setVisualModeType( ViMode mode );
    KTextEditor::Cursor getStart() const { return m_start; }
    KTextEditor::Range getVisualRange() const;

  private:
    void initializeCommands();
    void updateDirty( bool entireView = false ) const;
    void goToPos( const KateViRange &r );
    void reset();
    ViMode m_mode;
    KTextEditor::Cursor m_start;
    KTextEditor::Cursor m_previous; // previous position, used when deciding which lines to redraw
};

#endif
