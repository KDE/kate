/*
   Copyright (C) 2002 by Roberto Raggi <roberto@kdevelop.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   version 2, License as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katecodecompletion_arghint.h"
#include "katecodecompletion_arghint.moc"

#include "kateview.h"
#include "katedocument.h"

#include <kdebug.h>

#include <qlabel.h>
#include <qintdict.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qapplication.h>

struct KDevArgHintData
{
    KateView* editorView;
    QIntDict<QLabel> labelDict;
    QLayout* layout;
};


using namespace std;

KDevArgHint::KDevArgHint( KateView* parent, const char* name )
    : QFrame( parent, name, WType_Popup )
{
    setBackgroundColor( black );

    d = new KDevArgHintData();
    d->labelDict.setAutoDelete( true );
    d->layout = new QVBoxLayout( this, 1, 2 );
    d->layout->setAutoAdd( true );
    d->editorView = parent;

    m_markCurrentFunction = true;

    setFocusPolicy( StrongFocus );
    setFocusProxy( parent );

    reset( -1, -1 );
}

KDevArgHint::~KDevArgHint()
{
    delete( d );
    d = 0;
}

void KDevArgHint::setArgMarkInfos( const QString& wrapping, const QString& delimiter )
{
    m_wrapping = wrapping;
    m_delimiter = delimiter;
    m_markCurrentFunction = true;
}

void KDevArgHint::reset( int line, int col )
{
    m_functionMap.clear();
    m_currentFunction = -1;
    d->labelDict.clear();

    m_currentLine = line;
    m_currentCol = col - 1;
}

void KDevArgHint::slotDone(bool completed)
{
    hide();

    m_currentLine = m_currentCol = -1;

    emit argHintHidden();
    if (completed)
        emit argHintCompleted();
    else
        emit argHintAborted();
}

void KDevArgHint::cursorPositionChanged( KateView* view, int line, int col )
{
    if( m_currentCol == -1 || m_currentLine == -1 ){
        slotDone(false);
        return;
    }

    int nCountDelimiter = 0;
    int count = 0;

    QString currentTextLine = view->doc()->textLine( line );
    QString text = currentTextLine.mid( m_currentCol, col - m_currentCol );
    QRegExp strconst_rx( "\"[^\"]*\"" );
    QRegExp chrconst_rx( "'[^']*'" );

    text = text
        .replace( strconst_rx, "\"\"" )
        .replace( chrconst_rx, "''" );

    int index = 0;
    while( index < (int)text.length() ){
        if( text[index] == m_wrapping[0] ){
            ++count;
        } else if( text[index] == m_wrapping[1] ){
            --count;
        } else if( count > 0 && text[index] == m_delimiter[0] ){
            ++nCountDelimiter;
        }
        ++index;
    }

    if( (m_currentLine > 0 && m_currentLine != line) || (m_currentLine < col) || (count == 0) ){
        slotDone(count == 0);
        return;
    }

    // setCurArg ( nCountDelimiter + 1 );

}

void KDevArgHint::addFunction( int id, const QString& prot )
{
    m_functionMap[ id ] = prot;
    QLabel* label = new QLabel( prot.stripWhiteSpace().simplifyWhiteSpace(), this );
    label->setBackgroundColor( QColor(255, 255, 238) );
    label->show();
    d->labelDict.insert( id, label );

    if( m_currentFunction < 0 )
        setCurrentFunction( id );
}

void KDevArgHint::setCurrentFunction( int currentFunction )
{
    if( m_currentFunction != currentFunction ){

        if( currentFunction < 0 )
            currentFunction = (int)m_functionMap.size() - 1;

        if( currentFunction > (int)m_functionMap.size()-1 )
            currentFunction = 0;

        if( m_markCurrentFunction && m_currentFunction >= 0 ){
            QLabel* label = d->labelDict[ m_currentFunction ];
            label->setFont( font() );
        }

        m_currentFunction = currentFunction;

        if( m_markCurrentFunction ){
            QLabel* label = d->labelDict[ currentFunction ];
            QFont fnt( font() );
            fnt.setBold( true );
            label->setFont( fnt );
        }

        adjustSize();
    }
}

void KDevArgHint::show()
{
    QFrame::show();
    adjustSize();
}

bool KDevArgHint::eventFilter( QObject*, QEvent* e )
{
    if( isVisible() && e->type() == QEvent::KeyPress ){
        QKeyEvent* ke = static_cast<QKeyEvent*>( e );
        if( (ke->state() & ControlButton) && ke->key() == Key_Left ){
            setCurrentFunction( currentFunction() - 1 );
            ke->accept();
            return true;
        } else if( ke->key() == Key_Escape ){
            slotDone(false);
            return false;
        } else if( (ke->state() & ControlButton) && ke->key() == Key_Right ){
            setCurrentFunction( currentFunction() + 1 );
            ke->accept();
            return true;
        }
    }

    return false;
}

void KDevArgHint::adjustSize( )
{
    QRect screen = QApplication::desktop()->screenGeometry( pos() );

    QFrame::adjustSize();
    if( width() > screen.width() )
        resize( screen.width(), height() );

    if( x() + width() > screen.width() )
        move( screen.width() - width(), y() );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
