/***************************************************************************
                          katecodecompletion_arghint.cpp  -  description
                             -------------------
	begin		: Sun Nov 18 20:00 CET 2001
	copyright	: (C) 2001 by Joseph Wenninger
	email		: jowenn@kde.org
	taken from KDEVELOP:
	begin		: Sam Jul 14 18:20:00 CEST 2001
	copyright	: (C) 2001 by Victor Röder
	email		: Victor_Roeder@GMX.de
 ***************************************************************************/

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kateview.h"
#include "kateviewinternal.h"
#include <qsizegrip.h>
#include <qapp.h>

#include "katecodecompletion_arghint.h"
#include "katedocument.h"
#include "katecodecompletion_iface_impl.h"
#include "katecodecompletion_iface_impl.moc"
#include <kdebug.h>
#include <iostream>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qtooltip.h>

class CompletionItem : public QListBoxText
{
public:
  CompletionItem( QListBox *lb,KTextEditor::CompletionEntry entry )
    : QListBoxText( lb ) {
    if(entry.postfix=="()"){ // should be configurable
      setText( entry.prefix + " " + entry.text + entry.postfix);
    }
    else{
      setText( entry.prefix + " " + entry.text + " " + entry.postfix);
    }
    m_entry = entry;
    }
  KTextEditor::CompletionEntry m_entry;
};


CodeCompletion_Impl::CodeCompletion_Impl(KateView *view):QObject(view),m_view(view),m_commentLabel(0)
{
  m_completionPopup = new QVBox( 0, 0, WType_Popup );
  m_completionPopup->setFrameStyle( QFrame::Box | QFrame::Plain );
  m_completionPopup->setLineWidth( 1 );

  m_completionListBox = new QListBox( m_completionPopup );
  m_completionListBox->setFrameStyle( QFrame::NoFrame );
  m_completionListBox->installEventFilter( this );

  m_completionPopup->installEventFilter( this );
  m_completionPopup->setFocusProxy( m_completionListBox );

  //QFont font = m_view->doc()->getFont(); //Is this correct ???
  //m_completionListBox->setFont(QFont(font.family(),font.pointSize()));

  m_pArgHint = new KDevArgHint ( m_view );
  connect(m_pArgHint,SIGNAL(argHintHidden()),SIGNAL(argHintHidden()));

  connect(m_view, SIGNAL ( cursorPositionChanged() ), this, SLOT ( slotCursorPosChanged () ) );
}

void CodeCompletion_Impl::showCompletionBox(QValueList<KTextEditor::CompletionEntry> complList,int offset,bool casesensitive){
  kdDebug(13000) << "showCompletionBox " << endl;

  m_caseSensitive=casesensitive;
  m_complList = complList;
  // align the prefix (begin)
  QValueList<KTextEditor::CompletionEntry>::Iterator it;
  uint maxLen =0;
  for( it = m_complList.begin(); it != m_complList.end(); ++it ){
    if(maxLen < (*it).prefix.length()){
      maxLen = (*it).prefix.length();
    }
  }
  for( it = m_complList.begin(); it != m_complList.end(); ++it ){
    QString fillStr;
    fillStr.fill(QChar(' '),maxLen - (*it).prefix.length()); // add some spaces
    (*it).prefix.append(fillStr);
  }
  // alignt the prefix (end)

  m_offset = offset;
  m_view->cursorPositionReal(&m_lineCursor, &m_colCursor);
  m_colCursor = m_colCursor - offset; // calculate the real start of the code completion text
  updateBox(true);

}

bool CodeCompletion_Impl::eventFilter( QObject *o, QEvent *e ){

  if ( o == m_completionPopup || o == m_completionListBox || o == m_completionListBox->viewport() ) {
    if ( e->type() == QEvent::KeyPress ) {
      QKeyEvent *ke = (QKeyEvent*)e;
      if ( (ke->key() == Key_Left) || (ke->key() == Key_Right) ||
	 (ke->key() == Key_Up) || (ke->key() == Key_Down ) ||
	   (ke->key() == Key_Home )|| (ke->key() == Key_End) ||
	   (ke->key() == Key_Prior) || (ke->key() == Key_Next )) {
	QTimer::singleShot(0,this,SLOT(showComment()));
	return FALSE;
      }
      if (ke->key() == Key_Enter || ke->key() == Key_Return) { // return
	CompletionItem* item = static_cast<CompletionItem*> (m_completionListBox->item(m_completionListBox->currentItem()));
	if(item !=0){
	  QString text = item->m_entry.text;
	  QString currentLine = m_view->currentTextLine();
	  int len = m_view->cursorColumnReal() - m_colCursor;
	  QString currentComplText = currentLine.mid(m_colCursor,len);
	  QString add = text.mid(currentComplText.length());
	  if(item->m_entry.postfix == "()") add=add+"(";

   	  emit filterInsertString(&(item->m_entry),&add);      
          m_view->insertText(add);

	  m_completionPopup->hide();
	  deleteCommentLabel();
	  m_view->setFocus();

	  emit  completionDone((item->m_entry));
	  emit completionDone();
	}
	return FALSE;
      }

      if(ke->key() == Key_Escape){ // abort
	m_completionPopup->hide();
	deleteCommentLabel();
	m_view->setFocus();
	emit completionAborted();
	return FALSE;
      }

      QApplication::sendEvent(m_view->m_viewInternal, e ); // redirect the event to the editor
      if(m_colCursor+m_offset > m_view->cursorColumnReal()){ // the cursor is to far left
	kdDebug(13000)<< "Aborting Codecompletion after sendEvent"<<endl;
	kdDebug(13000)<<QString("%1").arg(m_view->cursorColumnReal())<<endl;
	m_completionPopup->hide();
	deleteCommentLabel();
	m_view->setFocus();
	emit completionAborted();
	return TRUE; //was false
      }
      updateBox();
      return TRUE;
    }

    if(e->type() == QEvent::FocusOut){
      m_completionPopup->hide();
      deleteCommentLabel();
      emit completionAborted();
    }
  }
  return FALSE;
}

void CodeCompletion_Impl::updateBox(bool newCoordinate){
  m_completionListBox->clear();
  QString currentLine = m_view->currentTextLine();
  kdDebug(13000) << "Column:" << m_colCursor<<endl;
  kdDebug(13000) <<"Line:" << currentLine<<endl;
  kdDebug(13000) << "CurrentColumn:" << m_view->cursorColumnReal()<<endl;
  int len = m_view->cursorColumnReal() - m_colCursor;
  kdDebug(13000)<< "Len:" << len<<endl;
  QString currentComplText = currentLine.mid(m_colCursor,len);
  kdDebug(13000) << "TEXT:" << currentComplText<<endl;
  QValueList<KTextEditor::CompletionEntry>::Iterator it;
  kdDebug(13000) << "Count:" << m_complList.count()<<endl;

  if (m_caseSensitive)
  for( it = m_complList.begin(); it != m_complList.end(); ++it ){
    kdDebug(13000)<< "insert "<<endl;
    if((*it).text.startsWith(currentComplText)){
      new CompletionItem(m_completionListBox,*it);
    }
  }
  else
  {
    currentComplText=currentComplText.upper();
    for( it = m_complList.begin(); it != m_complList.end(); ++it ){
      kdDebug(13000)<< "insert "<<endl;
      if((*it).text.upper().startsWith(currentComplText)){
        new CompletionItem(m_completionListBox,*it);
      }
    }

  }

  if(m_completionListBox->count()==0){
    m_completionPopup->hide();
    deleteCommentLabel();
    m_view->setFocus();
    emit completionAborted();
    return;
  }
  m_completionListBox->setCurrentItem( 0 );
  m_completionListBox->setSelected( 0,true );
  m_completionListBox->setFocus();
  if(newCoordinate){
    m_completionPopup->resize( m_completionListBox->sizeHint() +
			       QSize( m_completionListBox->verticalScrollBar()->width() + 4,
				      m_completionListBox->horizontalScrollBar()->height() + 4 ) );
    m_view->m_viewInternal->paintCursor();
    m_completionPopup->move(m_view->mapToGlobal(m_view->cursorCoordinates()));
  }
  m_completionPopup->show();
  QTimer::singleShot(0,this,SLOT(showComment()));
  
}

void CodeCompletion_Impl::showArgHint ( QStringList functionList, const QString& strWrapping, const QString& strDelimiter )
{
	m_pArgHint->reset();

	m_pArgHint->setArgMarkInfos ( strWrapping, strDelimiter );

	QStringList::Iterator it;

	int nNum = 0;

	for( it = functionList.begin(); it != functionList.end(); it++ )
	{
		kdDebug(13000) << "Insert function text: " << *it << endl;

		m_pArgHint->setFunctionText ( nNum, ( *it ) );

		nNum++;
	}
	m_view->m_viewInternal->paintCursor();
	m_pArgHint->move(m_view->mapToGlobal(m_view->cursorCoordinates()));
	m_pArgHint->show();

}

void CodeCompletion_Impl::slotCursorPosChanged()
{
	m_pArgHint->cursorPositionChanged ( m_view, m_view->cursorLine(), m_view->cursorColumnReal() );
}


void CodeCompletion_Impl::deleteCommentLabel()
{
	if (m_commentLabel)
	{	
		delete m_commentLabel;
		m_commentLabel=0;
	}
}

void CodeCompletion_Impl::showComment()
{
	CompletionItem* item = static_cast<CompletionItem*> (m_completionListBox->item(m_completionListBox->currentItem()));
	if (item)
	{
                if (m_commentLabel) delete m_commentLabel;
                if (!item->m_entry.comment.isEmpty())
                {
                        m_commentLabel=new KateCodeCompletionCommentLabel(0,item->m_entry.comment);
			m_commentLabel->setFont(QToolTip::font());
			m_commentLabel->setPalette(QToolTip::palette());
			QPoint rightPoint=m_completionPopup->mapToGlobal(QPoint(m_completionPopup->width(),0));
			QPoint leftPoint=m_completionPopup->mapToGlobal(QPoint(0,0));
			QPoint finalPoint;

			QRect screen = QApplication::desktop()->screenGeometry( m_commentLabel->x11Screen() );

			if (rightPoint.x()+m_commentLabel->width() > screen.x() + screen.width())
				finalPoint.setX(leftPoint.x()-m_commentLabel->width());
			else
				finalPoint.setX(rightPoint.x());

			m_completionListBox->ensureCurrentVisible();

			finalPoint.setY(
				m_completionListBox->viewport()->mapToGlobal(m_completionListBox->itemRect(
					m_completionListBox->item(m_completionListBox->currentItem())).topLeft()).y());

			m_commentLabel->move(finalPoint);
                        m_commentLabel->show();
                }

	}
}
