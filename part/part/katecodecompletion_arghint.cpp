/* This file is part of the KDE libraries

   Copyright  (C) 2001 by Joseph Wenninger <jowenn@kde.org>
   Copyright  (C) 2001 by Victor Röder <Victor_Roeder@GMX.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/

/* Trolltech doesn't mind, if we license that piece of code as LGPL, because there isn't much
 * left from the designer code */

#include <qregexp.h>
#include <klocale.h>

#include <kdebug.h>
#include <qtooltip.h>

#include "katecodecompletion_arghint.h"
#include "katecodecompletion_arghint.moc"

#include "kateview.h"
#include "katedocument.h"

static const char* left_xpm[] = {
"16 16 3 1",
"a c #000000",
"# c #c5c2c5",
". c #ffffee",
"................",
"................",
"...########.....",
"...########.....",
"...#####a##.....",
"...####aa##.....",
"...###aaa##.....",
"...##aaaa##.....",
"...##aaaa##.....",
"...###aaa##.....",
"...####aa##.....",
"...#####a##.....",
"...########.....",
"...########.....",
"................",
"................"};

static const char* right_xpm[] = {
"16 16 3 1",
"a c #000000",
"# c #c5c2c5",
". c #ffffee",
"................",
"................",
".....########...",
".....########...",
".....##a#####...",
".....##aa####...",
".....##aaa###...",
".....##aaaa##...",
".....##aaaa##...",
".....##aaa###...",
".....##aa####...",
".....##a#####...",
".....########...",
".....########...",
"................",
"................"};


ArgHintArrow::ArgHintArrow ( QWidget *parent, Dir d )
    : QButton( parent, 0, WStyle_NormalBorder )
{
	setFixedSize ( 16, 16 );

	if ( d == Left )
		pix = QPixmap ( left_xpm );
	else
		pix = QPixmap ( right_xpm );
}

void ArgHintArrow::drawButton ( QPainter *p )
{
	if ( isEnabled() )
		p->drawPixmap ( 0, 0, pix );
}


KDevArgHint::KDevArgHint ( QWidget *parent ) : QFrame ( parent, 0,  WType_Popup ),ESC_accel(0)
{
	setFrameStyle ( QFrame::Box | QFrame::Plain );
	setLineWidth ( 1 );
			//	setBackgroundColor ( QColor ( 255, 255, 238 ) );

	setPalette(QToolTip::palette());

	QHBoxLayout* hbox = new QHBoxLayout ( this );
	hbox->setMargin ( 1 );

	hbox->addWidget ( ( m_pPrev = new ArgHintArrow ( this , ArgHintArrow::Left ) ) );
	hbox->addWidget ( ( m_pStateLabel = new QLabel ( this ) ) );
	hbox->addWidget ( ( m_pNext = new ArgHintArrow ( this, ArgHintArrow::Right ) ) );
	hbox->addWidget ( ( m_pFuncLabel = new QLabel ( this ) ) );


	setFocusPolicy ( StrongFocus );
	setFocusProxy ( parent );

				//	m_pStateLabel->setBackgroundColor ( QColor ( 255, 255, 238 ) );
	m_pStateLabel->setPalette(QToolTip::palette());
	m_pStateLabel->setAlignment ( AlignCenter );
	m_pStateLabel->setFont ( QToolTip::font() );
				//	m_pFuncLabel->setBackgroundColor ( QColor ( 255, 255, 238 ) );
	m_pFuncLabel->setPalette(QToolTip::palette());
	m_pFuncLabel->setAlignment ( AlignCenter);
	m_pFuncLabel->setFont ( QToolTip::font() );

	m_pPrev->setFixedSize ( 16, 16 );
	m_pStateLabel->setFixedSize ( 36, 16 );
	m_pNext->setFixedSize ( 16, 16 );

	connect ( m_pPrev, SIGNAL ( clicked() ), this, SLOT ( gotoPrev() ) );
	connect ( m_pNext, SIGNAL ( clicked() ), this, SLOT ( gotoNext() ) );

	m_nNumFunc = m_nCurFunc = m_nCurLine = 0;

	m_nCurArg = 1;

	m_bMarkingEnabled = false;

	updateState();
}

KDevArgHint::~KDevArgHint()
{
	delete m_pPrev;
	delete m_pNext;
	delete m_pStateLabel;
	delete m_pFuncLabel;
}

/*
void KDevArgHint::setFont ( const QFont& font )
{
	m_pFuncLabel->setFont ( font );
	m_pStateLabel->setFont ( font );
}*/

/** No descriptions */
void KDevArgHint::gotoPrev()
{
	if ( m_nCurFunc > 0 )
		m_nCurFunc--;
	else
		m_nCurFunc = m_nNumFunc - 1;
  
	updateState();
}

/** No descriptions */
void KDevArgHint::gotoNext()
{
	if ( m_nCurFunc < m_nNumFunc - 1 )
		m_nCurFunc++;
	else
		m_nCurFunc = 0;

	updateState();
}

/** No descriptions */
void KDevArgHint::updateState()
{
	QString strState;
        strState = (i18n (  "%1 of %2" )).arg( m_nCurFunc + 1).arg(m_nNumFunc );

	m_pStateLabel->setText ( strState );

	m_pFuncLabel->setText ( markCurArg() );

	if ( m_nNumFunc <= 1 )
	{
		m_pPrev->hide();
		m_pNext->hide();
		m_pStateLabel->hide();
	}
	else
	{
		m_pPrev->show();
		m_pNext->show();
		m_pStateLabel->show();
	}

	m_pPrev->adjustSize();
	m_pStateLabel->adjustSize();
	m_pNext->adjustSize();
	m_pFuncLabel->adjustSize();
	adjustSize();
}

void KDevArgHint::reset()
{
	m_funcList.clear();

	m_nNumFunc = m_nCurFunc = m_nCurLine = 0;
	m_nCurArg = 1;

	updateState();
	ESC_accel=new QAccel((QWidget*)parent());
	ESC_accel->insertItem(Key_Escape,1);
	ESC_accel->setEnabled(true);
	connect(ESC_accel,SIGNAL(activated(int)),this,SLOT(slotDone(int)));
}

void KDevArgHint::cursorPositionChanged (KateView *view, int nLine, int nCol )
{
	if ( m_nCurLine == 0 )
		m_nCurLine = nLine;

	if ( m_nCurLine > 0 && m_nCurLine != nLine)
	{
		slotDone(0);
		return;
	}

	if ( view->getDoc()->hasSelection() )
	{
		slotDone(0);
		return;
	}

	
	
 	QString strCurLine="";
	if (view->doc()->kateTextLine(nLine)) strCurLine = view->doc()->kateTextLine ( nLine )->string();
	strCurLine.replace(QRegExp("\t"),"        "); // hack which asume that TAB is 8 char big #fixme
	//strCurLine = strCurLine.left ( nCol );
	QString strLineToCursor = strCurLine.left ( nCol );
	QString strLineAfterCursor = strCurLine.mid ( nCol, ( strCurLine.length() - nCol ) );

	// only for testing
	//strCurLine = strLineToCursor;

	int nBegin = strLineToCursor.findRev ( m_strArgWrapping[0] );

	if ( nBegin == -1 || // the first wrap was not found
	nBegin != -1 && strLineToCursor.findRev ( m_strArgWrapping[1] ) != -1 ) // || // the first and the second wrap were found before the cursor
	//nBegin != -1 && strLineAfterCursor.findRev ( m_strArgWrapping[1] ) != -1 ) // the first wrap was found before the cursor and the second beyond
	{
		slotDone(0);
		//m_nCurLine = 0; // reset m_nCurLine so that ArgHint is finished
	}

	int nCountDelimiter = 0;

	while ( nBegin != -1 )
	{
	  nBegin = strLineToCursor.find ( m_strArgDelimiter, nBegin + 1 );

		if ( nBegin != -1 )
			nCountDelimiter++;
	}

	setCurArg ( nCountDelimiter + 1 );
}

void KDevArgHint::setFunctionText ( int nFunc, const QString& strText )
{
	m_funcList.replace ( nFunc, strText );

	m_nNumFunc++;

	if ( nFunc == m_nCurFunc )
		m_pFuncLabel->clear();

	updateState();
}

void KDevArgHint::setArgMarkInfos ( const QString& strWrapping, const QString& strDelimiter )
{
	if ( strWrapping.isEmpty() || strDelimiter.isEmpty() )
		return;

	m_strArgWrapping = strWrapping;
	m_strArgDelimiter = strDelimiter;
	m_bMarkingEnabled = true;
}

void KDevArgHint::nextArg()
{
	m_nCurArg++;

	updateState();
}

void KDevArgHint::prevArg()
{
	m_nCurArg--;

	updateState();
}

void KDevArgHint::setCurArg ( int nCurArg )
{
	if ( m_nCurArg == nCurArg )
		return;

	m_nCurArg = nCurArg;

	updateState();
}

QString KDevArgHint::markCurArg()
{
	QString strFuncText = m_funcList [ m_nCurFunc ];

	if ( !m_bMarkingEnabled )
		return strFuncText;

	if ( strFuncText.isEmpty() )
		return "\0";


	int nBegin = strFuncText.find ( m_strArgWrapping[0] ) + 1;
	int nEnd = nBegin;

	for ( int i = 0; i <= m_nCurArg; i++ )
	{
		if ( i > 1 )
			nBegin = nEnd + 1;

		if ( strFuncText.find ( m_strArgDelimiter , nBegin ) == -1 )
		{
			nEnd = strFuncText.find ( m_strArgWrapping[1], nBegin );

			break;
		}
		else
			nEnd = strFuncText.find ( m_strArgDelimiter, nBegin );
	}

	strFuncText = strFuncText.insert ( nBegin, "<b>" );
	strFuncText = strFuncText.insert ( ( nEnd + 3 ), "</b>" );

	while ( strFuncText.find ( ' ', 0 ) != -1 ) // replace ' ' with "&ndsp;" so that there's no wrap
		strFuncText = strFuncText.replace ( ( strFuncText.find ( ' ', 0 ) ), 1, "&nbsp;" );

	strFuncText = strFuncText.prepend ( "<qt>&nbsp;" );
	strFuncText = strFuncText.append ( "</qt>" );

	kdDebug ( 12001 ) << strFuncText <<endl;

	return strFuncText;
}

void KDevArgHint::slotDone(int /* id */)
{
	// kdDebug(13000)<<QString("Slot done %1").arg(id)<<endl;
	hide();
	if (ESC_accel) {delete ESC_accel; ESC_accel=0;}
	emit argHintHidden();
}
