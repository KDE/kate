/* This file is part of the KDE libraries
   copyright	: (C) 2001 by Joseph Wenninger <jowenn@kde.org>
   copyright	: (C) 2001 by Victor Röder <Victor_Roeder@GMX.de>

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
 * left from the desigener code */

#ifndef __CODECOMPLETION_ARGHINT_H__
#define __CODECOMPLETION_ARGHINT_H__

#include <qframe.h>
#include <qmap.h>
#include <qbutton.h>
#include <qpixmap.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qlayout.h>
#include <qfont.h>
#include <qaccel.h>

class KateView;

class ArgHintArrow : public QButton
{
	Q_OBJECT

	public:
		enum Dir { Left, Right };

		ArgHintArrow ( QWidget *parent, Dir d );
		void drawButton ( QPainter *p );

	private:
		QPixmap pix;
};



/** KDevArgHint is the base class of the porject */
class KDevArgHint : public QFrame
{
	Q_OBJECT

	public:
		/** construtor */
		KDevArgHint ( QWidget* parent );
		/** destructor */
		virtual ~KDevArgHint();

//  		bool eventFilter( QObject *o, QEvent *e );


	public:
		void setFunctionText ( int nFunc, const QString& strText );
		void setArgMarkInfos ( const QString& strWrapping, const QString& strDelimiter );
		void reset();
		void nextArg();
		void prevArg();
		void setCurArg ( int nCurArg );
//		virtual void setFont ( const QFont & );

	private:
		QAccel *ESC_accel;
		QMap<int, QString> m_funcList;
		QLabel* m_pFuncLabel;
		QLabel* m_pStateLabel;

		ArgHintArrow* m_pPrev;
		ArgHintArrow* m_pNext;

		int m_nCurFunc;
		int m_nNumFunc;
		int m_nCurArg;
		int m_nCurLine;

		bool m_bMarkingEnabled;

		QString m_strArgWrapping;
		QString m_strArgDelimiter;

	public slots:
		void cursorPositionChanged (KateView *view, int nLine, int nCol );

	private slots:
		/** No descriptions */
		void gotoNext();

		 /** No descriptions */
		void gotoPrev();

	protected:
		  /** No descriptions */
		void updateState();
		QString markCurArg();

 signals:
		void argHintHidden();

 public slots:
		void slotDone(int);
};

#endif
