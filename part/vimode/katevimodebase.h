/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *  Copyright (C) 2011 Svyatoslav Kuzmich <svatoslav1@gmail.com>
 *  Copyright (C) 2012 - 2013 Simon St James <kdedevel@etotheipiplusone.com>
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
#include "katepartprivate_export.h"
#include "katedocument.h"
#include "katelayoutcache.h"

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

enum OperationMode {
    CharWise = 0,
    LineWise,
    Block
};

enum Direction {
    Up,
    Down,
    Left,
    Right,
    Next,
    Prev
};

class KATEPART_TESTS_EXPORT KateViModeBase : public QObject
{
  Q_OBJECT

  public:
    KateViModeBase()
      : QObject(),
        m_count(0),
        m_oneTimeCountOverride(-1),
        m_iscounted(false),
        m_stickyColumn(-1)
    {
    }
    virtual ~KateViModeBase() {}

    /**
     * @return normal mode command accumulated so far
     */
    QString getVerbatimKeys() const;
    virtual bool handleKeypress( const QKeyEvent *e ) = 0;

    void setCount(unsigned int count) { m_count = count; }
    void setRegister(QChar reg) {m_register =  reg;}

    void error( const QString &errorMsg );
    void message( const QString &msg );

    Range findPattern( const QString& pattern, bool backwards, bool caseSensitive, const Cursor& startFrom, int count = -1 /* i.e use getCount() */ ) const;
  protected:
    // helper methods
    void yankToClipBoard(QChar chosen_register, QString text);
    bool deleteRange( KateViRange &r, OperationMode mode = LineWise, bool addToRegister = true );
    const QString getRange( KateViRange &r, OperationMode mode = LineWise ) const;
    const QString getLine(int line = -1) const;
    const QChar getCharUnderCursor() const;
    const QString getWordUnderCursor() const;
    const Range getWordRangeUnderCursor() const;
    KateViRange findPatternForMotion( const QString& pattern, bool backwards, bool caseSensitive, const Cursor& startFrom, int count = 1 ) const;
    Cursor findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findPrevWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;
    Cursor findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine = false ) const;

    KateViRange findSurroundingBrackets( const QChar &c1, const QChar &c2, bool inner,
                                        const QChar &nested1 , const QChar &nested2  ) const;
    KateViRange findSurrounding( const QRegExp &c1, const QRegExp &c2, bool inner = false ) const;
    KateViRange findSurroundingQuotes( const QChar &c, bool inner = false ) const;

    int findLineStartingWitchChar( const QChar &c, unsigned int count, bool forward = true ) const;
    void updateCursor( const Cursor &c ) const;
    const QChar getCharAtVirtualColumn( QString &line, int virtualColumn, int tabWidht ) const;

    void addToNumberUnderCursor( int count );

    KateViRange goLineUp();
    KateViRange goLineDown();
    KateViRange goLineUpDown( int lines);
    KateViRange goVisualLineUpDown(int lines);


    unsigned int linesDisplayed() { return m_viewInternal->linesDisplayed(); }
    void scrollViewLines( int l ) { m_viewInternal->scrollViewLines( l ); }

    unsigned int getCount() const {
      if (m_oneTimeCountOverride != -1)
      {
        return m_oneTimeCountOverride;
      }
      return ( m_count > 0 ) ? m_count : 1;
    }
    bool isCounted() { return m_iscounted; }

    bool startNormalMode();
    bool startInsertMode();
    bool startVisualMode();
    bool startVisualLineMode();
    bool startVisualBlockMode();
    bool startReplaceMode();

    QChar getChosenRegister( const QChar &defaultReg ) const;
    QString getRegisterContent( const QChar &reg );
    OperationMode getRegisterFlag( const QChar &reg ) const;
    void fillRegister( const QChar &reg, const QString &text, OperationMode flag = CharWise);

    void switchView(Direction direction = Next);

    QChar m_register;

    void addJump(KTextEditor::Cursor cursor);
    KTextEditor::Cursor getNextJump(KTextEditor::Cursor);
    KTextEditor::Cursor getPrevJump(KTextEditor::Cursor);

    KateViRange m_commandRange;
    unsigned int m_count;
    int m_oneTimeCountOverride;
    bool m_iscounted;

    QString m_extraWordCharacters;
    QString m_keysVerbatim;

    int m_stickyColumn;
    bool m_lastMotionWasVisualLineUpOrDown;
    bool m_currentMotionWasVisualLineUpOrDown;

    inline KateDocument* doc() const { return m_view->doc(); };


    KateView *m_view;
    KateViewInternal *m_viewInternal;
    KateViInputModeManager* m_viInputModeManager;

    // info message of vi mode
    QPointer<KTextEditor::Message> m_infoMessage;
};

#endif
