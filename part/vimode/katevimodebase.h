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

#ifndef KATE_VI_MODE_BASE_INCLUDED
#define KATE_VI_MODE_BASE_INCLUDED

#include <ktexteditor/cursor.h>
#include "kateview.h"
#include "katevirange.h"
#include "kateviewinternal.h"

#include <QList>

using KTextEditor::Cursor;
using KTextEditor::Range;

class QKeyEvent;
class QString;
class QRegExp;
class QTimer;
class KateDocument;
class KateViVisualMode;
class KateViNormalMode;
class KateViInputModeManager;

class KateViModeBase : public QObject
{
  Q_OBJECT

  public:
    KateViModeBase() : QObject() {};
    virtual ~KateViModeBase() {};

    /**
     * @return normal mode command accumulated so far
     */
    QString getVerbatimKeys() const;

    virtual void addMapping( const QString &from, const QString &to ) = 0;
    virtual const QString getMapping( const QString &from ) const = 0;
    virtual const QStringList getMappings() const = 0;

  protected:
    // helper methods
    bool deleteRange( KateViRange &r, bool linewise = true, bool addToRegister = true );
    const QString getRange( KateViRange &r, bool linewise = true ) const;
    const QString getLine( int lineNumber = -1 ) const;
    const QChar getCharUnderCursor() const;
    const QString getWordUnderCursor() const;
    KateViRange findPattern( const QString &pattern, bool backwards = false, int count = 1 ) const;
    Cursor findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    KateViRange findSurrounding( const QChar &c1, const QChar &c2, bool inner = false ) const;
    KateViRange findSurrounding( const QRegExp &c1, const QRegExp &c2, bool inner = false ) const;
    int findLineStartingWitchChar( const QChar &c, unsigned int count, bool forward = true ) const;
    void updateCursor( const Cursor &c ) const;
    const QChar getCharAtVirtualColumn( QString &line, int virtualColumn, int tabWidht ) const;

    void addToNumberUnderCursor( int count );

    KateViRange goLineUp();
    KateViRange goLineDown();
    KateViRange goLineUpDown( int lines);

    unsigned int linesDisplayed() { return m_viewInternal->linesDisplayed(); }
    void scrollViewLines( int l ) { m_viewInternal->scrollViewLines( l ); }

    unsigned int getCount() const { return ( m_count > 0 ) ? m_count : 1; }

    bool startNormalMode();
    bool startInsertMode();
    bool startVisualMode();
    bool startVisualLineMode();
    bool startVisualBlockMode();
    bool startReplaceMode();

    void error( const QString &errorMsg ) const;
    void message( const QString &msg ) const;

    QChar getChosenRegister( const QChar &defaultReg ) const;
    QString getRegisterContent( const QChar &reg ) const;
    void fillRegister( const QChar &reg, const QString &text);

    QChar m_register;

    KateViRange m_commandRange;
    unsigned int m_count;

    QString m_extraWordCharacters;
    QString m_keysVerbatim;

    int m_stickyColumn;

    inline KateDocument* doc() const { return m_view->doc(); };

    // key mappings
    int m_timeoutlen; // time to wait for the next keypress of a multi-key mapping (default: 1000 ms)
    QTimer *m_mappingTimer;

    KateView *m_view;
    KateViewInternal *m_viewInternal;
    KateViInputModeManager* m_viInputModeManager;
};

#endif
