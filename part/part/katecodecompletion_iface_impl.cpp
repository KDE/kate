// basic idea from qtdesigner by TrollTech
// Taken from kdevelop
//#include "kwrite/kwview.h"
//#include "kwrite/kwdoc.h"
#include "kateview.h"
#include <qsizegrip.h>
#include <qapp.h>

#include "katecodecompletion_arghint.h"
#include "katedocument.h"
#include "katecodecompletion_iface_impl.h"
#include "katecodecompletion_iface_impl.moc"
#include <kdebug.h>
#include <iostream.h>


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

CodeCompletion_Impl::CodeCompletion_Impl(KateView *view):QObject(view),m_view(view)
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
  connect(m_pArgHint,SIGNAL(argHintHided()),SIGNAL(argHintHided()));

  connect(m_view, SIGNAL ( cursorPositionChanged() ), this, SLOT ( slotCursorPosChanged () ) );
}

void CodeCompletion_Impl::showCompletionBox(QValueList<KTextEditor::CompletionEntry> complList,int offset){
  kdDebug() << "showCompletionBox " << endl;

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
      if ( ke->key() == Key_Left || ke->key() == Key_Right ||
	   ke->key() == Key_Up || ke->key() == Key_Down ||
	   ke->key() == Key_Home || ke->key() == Key_End ||
	   ke->key() == Key_Prior || ke->key() == Key_Next ) {
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
	  if(item->m_entry.postfix == "()"){ // add (
	    m_view->insertText(add + "(");
	    //	    VConfig c;
	    //	    m_edit->view()->getVConfig(c);
	    //	    m_edit->view()->cursorLeft(c);
	  }
	  else{
	    m_view->insertText(add);
	  }
	  m_completionPopup->hide();
	  m_view->setFocus();
	  emit completionDone();
	}
	return FALSE;
      }

      if(ke->key() == Key_Escape){ // abort
	m_completionPopup->hide();
	m_view->setFocus();
	emit completionAborted();
	return FALSE;
      }

      QApplication::sendEvent(m_view, e ); // redirect the event to the editor
      if(m_colCursor+m_offset > m_view->cursorColumnReal()){ // the cursor is to far left
	m_completionPopup->hide();
	m_view->setFocus();
	emit completionAborted();
	return FALSE;
      }
      updateBox();
      return TRUE;
    }

    if(e->type() == QEvent::FocusOut){
      m_completionPopup->hide();
      emit completionAborted();
    }
  }
  return FALSE;
}

void CodeCompletion_Impl::updateBox(bool newCoordinate){
  m_completionListBox->clear();
  QString currentLine = m_view->currentTextLine();
  kdDebug() << "Column:" << m_colCursor<<endl;
  kdDebug() <<"Line:" << currentLine<<endl;
  kdDebug() << "CurrentColumn:" << m_view->cursorColumnReal()<<endl;
  int len = m_view->cursorColumnReal() - m_colCursor;
  kdDebug()<< "Len:" << len<<endl;
  QString currentComplText = currentLine.mid(m_colCursor,len);
  kdDebug() << "TEXT:" << currentComplText<<endl;
  QValueList<KTextEditor::CompletionEntry>::Iterator it;
  kdDebug() << "Count:" << m_complList.count()<<endl;
  for( it = m_complList.begin(); it != m_complList.end(); ++it ){
    kdDebug()<< "insert "<<endl;
    if((*it).text.startsWith(currentComplText)){
      new CompletionItem(m_completionListBox,*it);
    }
  }
  if(m_completionListBox->count()==0){
    m_completionPopup->hide();
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
    m_view->myViewInternal->paintCursor();
    m_completionPopup->move(m_view->mapToGlobal(m_view->cursorCoordinates()));
  }
  m_completionPopup->show();
}

void CodeCompletion_Impl::showArgHint ( QStringList functionList, const QString& strWrapping, const QString& strDelimiter )
{
	m_pArgHint->reset();

	m_pArgHint->setArgMarkInfos ( strWrapping, strDelimiter );

	QStringList::Iterator it;

	int nNum = 0;

	for( it = functionList.begin(); it != functionList.end(); it++ )
	{
		kdDebug() << "Insert function text: " << *it << endl;

		m_pArgHint->setFunctionText ( nNum, ( *it ) );

		nNum++;
	}
	m_view->myViewInternal->paintCursor();
	m_pArgHint->move(m_view->mapToGlobal(m_view->cursorCoordinates()));
	m_pArgHint->show();

}

void CodeCompletion_Impl::slotCursorPosChanged()
{
	m_pArgHint->cursorPositionChanged ( m_view, m_view->cursorLine(), m_view->cursorColumnReal() );
}
