/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 by Victor Röder <Victor_Roeder@GMX.de>
   Copyright (C) 2002 by Roberto Raggi <roberto@kdevelop.org>

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

/******** Partly based on the ArgHintWidget of Qt3 by Trolltech AS *********/
/* Trolltech doesn't mind, if we license that piece of code as LGPL, because there isn't much
 * left from the desigener code */

#include "katecodecompletion.h"
#include "katecodecompletion.moc"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katefont.h"

#include <kdebug.h>

#include <q3vbox.h>
#include <q3listbox.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <qsizegrip.h>
#include <qfontmetrics.h>
#include <qlayout.h>
#include <qregexp.h>
#include <QDesktopWidget>
#include <QKeyEvent>


/**
 * This class is used as the codecompletion listbox. It can be resized according to its contents,
 *  therfor the needed size is provided by sizeHint();
 *@short Listbox showing codecompletion
 *@author Jonas B. Jacobi <j.jacobi@gmx.de>
 */
class KateCCListBox : public Q3ListBox
{
  public:
    /**
      @short Create a new CCListBox
      @param view The KateView, CCListBox is displayed in
    */
    KateCCListBox (QWidget* parent = 0, const char* name = 0, Qt::WFlags f = 0):Q3ListBox(parent, name, f)
    {
    }

    QSize sizeHint()  const
    {
        int count = this->count();
        int height = 20;
        int tmpwidth = 8;
        //FIXME the height is for some reasons at least 3 items heigh, even if there is only one item in the list
        if (count > 0)
            if(count < 11)
                height =  count * itemHeight(0);
            else  {
                height = 10 * itemHeight(0);
                tmpwidth += verticalScrollBar()->width();
            }

        int maxcount = 0, tmpcount = 0;
        for (int i = 0; i < count; ++i)
            if ( (tmpcount = fontMetrics().width(text(i)) ) > maxcount)
                    maxcount = tmpcount;

        if (maxcount > QApplication::desktop()->width()){
            tmpwidth = QApplication::desktop()->width() - 5;
            height += horizontalScrollBar()->height();
        } else
            tmpwidth += maxcount;
        return QSize(tmpwidth,height);

    }
};

class KateCompletionItem : public Q3ListBoxText
{
  public:
    KateCompletionItem( Q3ListBox* lb, KateCodeCompletion::CompletionItem item, Q3ListBoxItem *after )
      : Q3ListBoxText( lb,"",after )
      , m_item( item )
    {
      if( item.item().postfix() == "()" ) { // should be configurable
        setText( item.item().prefix() + " " + item.text() + item.item().postfix() );
      } else {
        setText( item.item().prefix() + " " + item.text() + " " + item.item().postfix());
      }
    }

    KateCodeCompletion::CompletionItem m_item;
};


KateCodeCompletion::KateCodeCompletion( KateView* view )
  : QObject( view, "Kate Code Completion" )
  , m_view( view )
  , m_commentLabel( 0 )
  , m_blockEvents(false)
{
  m_completionPopup = new Q3VBox( 0, 0, Qt::WType_Popup );
  m_completionPopup->setFrameStyle( Q3Frame::Box | Q3Frame::Plain );
  m_completionPopup->setLineWidth( 1 );

  m_completionListBox = new KateCCListBox( m_completionPopup );
  m_completionListBox->setFrameStyle( Q3Frame::NoFrame );
  //m_completionListBox->setCornerWidget( new QSizeGrip( m_completionListBox) );
  m_completionListBox->setFocusProxy( m_view->m_viewInternal );

  m_completionListBox->installEventFilter( this );
  m_completionPopup->resize(m_completionListBox->sizeHint() + QSize(2,2));
  m_completionPopup->installEventFilter( this );
  m_completionPopup->setFocusProxy( m_view->m_viewInternal );

  m_pArgHint = new KateArgHint( m_view );
  connect( m_pArgHint, SIGNAL(argHintHidden()),
           this, SIGNAL(argHintHidden()) );

  connect( m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*)),
           this, SLOT(slotCursorPosChanged()) );
}

bool KateCodeCompletion::codeCompletionVisible () {
  return m_completionPopup->isVisible();
}


void KateCodeCompletion::buildItemList() {
  kdDebug(13034)<<"buildItemList"<<endl;
  m_items.clear();
  foreach (const KTextEditor::CompletionData& data,m_data) {
//     //kdDebug(13034)<<"buildItemList:1"<<endl;
    const QList<KTextEditor::CompletionItem>&  list=data.items();
    for(int i=0;i<list.count();i++) {
      //kdDebug(13034)<<"buildItemList:2"<<endl;
      m_items.append(CompletionItem(&data,i));
    }
  }
  qSort(m_items);
#if 0
  kdDebug(13035)<<"------------"<<endl;
  foreach (const CompletionItem& item,m_items)
    kdDebug(13035)<<item.text()<<endl;
  kdDebug(13035)<<"------------"<<endl;
#endif
}


void KateCodeCompletion::showCompletion(const KTextEditor::Cursor &position,const QLinkedList<KTextEditor::CompletionData> &data) {
  kdDebug(13034)<<"KateCodeCompletion::showCompletion"<<endl;
  kdDebug(13034)<<"data.size()=="<<data.size()<<endl;
  if (data.isEmpty() && m_data.isEmpty()) return;
  else if (m_data.isEmpty()) { // new completion
    m_data=data;
    kdDebug(13035)<<"m_data was empty"<<endl;
    buildItemList();
    updateBox();
  } else if (data.isEmpty()) {  // abort completion, no providers anymore
    m_data.clear();
    kdDebug(13035)<<"data is empty"<<endl;
    buildItemList();
    updateBox();
    return;
    //do abort here
  } else { //update completion
    if (data.size()!=m_data.size()) { // different provider count
      m_data=data;
      kdDebug(13035)<<"different size"<<endl;
      buildItemList();
      updateBox();
    } else {
      bool equal=true;
      for (QLinkedList<KTextEditor::CompletionData>::const_iterator it1=data.constBegin(),
          it2=m_data.constBegin();it1!=data.constEnd();++it1,++it2) {
          if (!((*it1)==(*it2))) {equal=false; kdDebug(13035)<<(*it1).id()<<" "<<(*it2).id()<<endl; break;}
      }
      if (equal) return;
      kdDebug(13035)<<"not equal"<<endl;
      m_data=data;
      buildItemList();
      updateBox();
    }
  }
}

#if 0
void KateCodeCompletion::showCompletionBox(
    QList<KTextEditor::CompletionItem> complList, int offset, bool casesensitive )
{
  kdDebug(13035) << "showCompletionBox " << endl;

  //if ( codeCompletionVisible() ) return;

  m_caseSensitive = casesensitive;
  //m_complList = complList;
  m_offset = offset;
  m_view->cursorPosition().position( m_lineCursor, m_colCursor );
  m_colCursor -= offset;

  updateBox( true );
}
#endif 

bool KateCodeCompletion::eventFilter( QObject *o, QEvent *e )
{
  kdDebug(13035)<<"KateCodeCompletion::eventFilter"<<endl;
  if ( o != m_completionPopup &&
       o != m_completionListBox &&
       o != m_completionListBox->viewport()
       #if 0 
       && o != m_view /*TEST*/ &&
       o != m_view->m_viewInternal /*TEST*/
 #endif
    )
    return false;

/* Is this really needed? abortCompletion will hide this thing, 
   aborting here again will send abort signal even on successfull completion */
/* JOWENN: Yes it is, for mouse click triggered aborting of code completion*/
   if( (e->type() == QEvent::Hide) && (m_completionPopup==o) )
   {
     if (!m_blockEvents)
      abortCompletion();
     return false;
   }


   if ( e->type() == QEvent::MouseButtonDblClick  ) {
    doComplete();
    return false;
   }

   if ( e->type() == QEvent::MouseButtonPress ) {
    QTimer::singleShot(0, this, SLOT(showComment()));
    return false;
   }

  if ((e->type()==QEvent::KeyPress) || (e->type()==QEvent::KeyRelease)) 
  {
    QApplication::sendEvent(m_view->m_viewInternal,e);
    if (!e->isAccepted()) QApplication::sendEvent(m_view->window(),e);
  }
  if ((e->type()==QEvent::Shortcut) || (e->type()==QEvent::ShortcutOverride) ||
	(e->type()==QEvent::Accel) )  
  {
    QApplication::sendEvent(m_view->window(),e);
  }

  kdDebug(13035)<<"e->type()=="<<e->type()<<endl;
  return false;
}

void KateCodeCompletion::handleKey (QKeyEvent *e)
{
  kdDebug(13035)<<"KateCodeCompletion::handleKey"<<endl;
  // close completion if you move out of range
  if ((e->key() == Qt::Key_Up) && (m_completionListBox->currentItem() == 0))
  {
    abortCompletion();
    m_view->setFocus();
    return;
  }

  // keyboard movement
  if( (e->key() == Qt::Key_Up)    || (e->key() == Qt::Key_Down ) ||
        (e->key() == Qt::Key_Home ) || (e->key() == Qt::Key_End)   ||
        (e->key() == Qt::Key_PageUp) || (e->key() == Qt::Key_PageDown ))
  {
    QTimer::singleShot(0,this,SLOT(showComment()));
    QApplication::sendEvent( m_completionListBox, (QEvent*)e );
    return;
  }

  // update the box
  updateBox();
}

void KateCodeCompletion::doComplete()
{
#if 0
  foreach (const KTextEditor::CompletionData& data,m_data) {
    kdDebug(13035)<<"datalist="<<&data<<endl;
  }
  kdDebug(13035)<<"doComplete------------"<<endl;
  foreach (const CompletionItem& item,m_items)
    kdDebug(13035)<<item.text()<<endl;
  kdDebug(13035)<<"doComplete------------"<<endl;
#endif

  KateCompletionItem* item = static_cast<KateCompletionItem*>(
     m_completionListBox->item(m_completionListBox->currentItem()));

  if( item == 0 )
    return;

  if (item->m_item.item().provider()) {
      m_view->completingInProgress(true);
      item->m_item.item().provider()->doComplete(m_view,*(item->m_item.data),item->m_item.item());
      m_view->completingInProgress(false);
  } else {
    QString text = item->m_item.text();
    QString currentLine = m_view->currentTextLine();
    int alreadyThere = m_view->cursorPosition().column() - item->m_item.data->matchStart().column();
    //QString currentComplText = currentLine.mid(m_colCursor,len);
    QString add = text.mid(alreadyThere);
    if( item->m_item.item().postfix() == "()" )
      add += "(";

    m_view->insertText(add);
  }
  complete( item->m_item.item() );
  m_view->setFocus();
}

void KateCodeCompletion::abortCompletion()
{
  m_completionPopup->hide();
  delete m_commentLabel;
  m_commentLabel = 0;
  m_items.clear();
  m_data.clear();
  m_view->completionAborted();
/*  emit completionAborted();*/
}

void KateCodeCompletion::complete( KTextEditor::CompletionItem entry )
{
  kdDebug(13035)<<"KateCodeCompletion::completion=============about to close completion box"<<endl;
  m_blockEvents=true;
  m_completionPopup->hide();
  delete m_commentLabel;
  m_commentLabel = 0;
  m_items.clear();
  m_data.clear();
  m_view->completionDone();;

/*
  emit completionDone( entry );
  emit completionDone();*/
}

void KateCodeCompletion::updateBox( bool )
{
  m_blockEvents=false;
#if 0
  if( m_colCursor > m_view->cursorPosition().column() ) {
    // the cursor is too far left
    kdDebug(13035) << "Aborting Codecompletion after sendEvent" << endl;
    kdDebug(13035) << m_view->cursorPosition().column() << endl;
    abortCompletion();
    m_view->setFocus();
    return;
  }
#endif 
  m_completionListBox->clear();
  kdDebug(13035)<<"m_items.size():"<<m_items.size()<<endl;;
  if (m_items.size()==0)
  {
    if (codeCompletionVisible())
    {
      abortCompletion();
      m_view->setFocus();
    }
    return;
  }

  QString currentLine = m_view->currentTextLine();
/*  int len = m_view->cursorPosition().column() - m_colCursor;
  QString currentComplText = currentLine.mid(m_colCursor,len); */
  QList<CompletionItem>::Iterator it;

  int currentCol=m_view->cursorPosition().column();
  int len=-1;
  QString currComp;
  Q3ListBoxItem *afteritem=0;
  if( m_caseSensitive ) {
    for( it = m_items.begin(); it != m_items.end(); ++it ) {
      if ((len<0) || ((currentCol-(it->data->matchStart().column()))!=len)) {
        int tmp=(currentCol-(it->data->matchStart().column()));
        if (tmp<0) continue;
        len=tmp;
        currComp=currentLine.mid(it->data->matchStart().column(),len);
      }
      if( (*it).text().startsWith(currComp) ) {
        afteritem=new KateCompletionItem(m_completionListBox,*it,afteritem);
      }
    }
  } else {
    for( it = m_items.begin(); it != m_items.end(); ++it ) {
      if ((len<0) || ((currentCol-(it->data->matchStart().column()))!=len)) {
        int tmp=(currentCol-(it->data->matchStart().column()));
        if (tmp<0) continue;
        len=tmp;
        currComp=currentLine.mid(it->data->matchStart().column(),len).upper();
      }
      if( (*it).text().upper().startsWith(currComp) ) {
        afteritem=new KateCompletionItem(m_completionListBox,*it,afteritem);
      }
    }
  }

  if( m_completionListBox->count() == 0 ) 
#warning fixme
/*||

      ( m_completionListBox->count() == 1 && // abort if we equaled the last item
        currentComplText == m_completionListBox->text(0).trimmed() ) ) */{
    abortCompletion();
    m_view->setFocus();
    return;
  }
    kdDebug(13035)<<"KateCodeCompletion::updateBox: Resizing widget"<<endl;
        m_completionPopup->resize(m_completionListBox->sizeHint() + QSize(2,2));
    QPoint p = m_view->mapToGlobal( m_view->cursorPositionCoordinates() );
        int x = p.x();
        int y = p.y() ;
        if ( y + m_completionPopup->height() + m_view->renderer()->config()->fontMetrics( )->height() > QApplication::desktop()->height() )
                y -= (m_completionPopup->height() );
        else
                y += m_view->renderer()->config()->fontMetrics( )->height();

        if (x + m_completionPopup->width() > QApplication::desktop()->width())
                x = QApplication::desktop()->width() - m_completionPopup->width();

        m_completionPopup->move( QPoint(x,y) );

  m_completionListBox->setCurrentItem( 0 );
  m_completionListBox->setSelected( 0, true );
  m_completionListBox->setFocus();
  m_completionPopup->show();

  QTimer::singleShot(0,this,SLOT(showComment()));
}

void KateCodeCompletion::showArgHint ( QStringList functionList, const QString& strWrapping, const QString& strDelimiter )
{
  int line, col;
  m_view->cursorPosition().position( line, col );
  m_pArgHint->reset( line, col );
  m_pArgHint->setArgMarkInfos( strWrapping, strDelimiter );

  int nNum = 0;
  QStringList::Iterator end(functionList.end());
  for( QStringList::Iterator it = functionList.begin(); it != end; ++it )
  {
    kdDebug(13035) << "Insert function text: " << *it << endl;

    m_pArgHint->addFunction( nNum, ( *it ) );

    nNum++;
  }

  m_pArgHint->move(m_view->mapToGlobal(m_view->cursorPositionCoordinates() + QPoint(0,m_view->renderer()->config()->fontMetrics( )->height())) );
  m_pArgHint->show();
}

void KateCodeCompletion::slotCursorPosChanged()
{
  m_pArgHint->cursorPositionChanged ( m_view, m_view->cursorPosition().line(), m_view->cursorPosition().column() );
}

void KateCodeCompletion::showComment()
{
  if (!m_completionPopup->isVisible())
    return;

  KateCompletionItem* item = static_cast<KateCompletionItem*>(m_completionListBox->item(m_completionListBox->currentItem()));

  if( !item )
    return;

  if( item->m_item.item().comment().isEmpty() )
    return;

  delete m_commentLabel;
  m_commentLabel = new KateCodeCompletionCommentLabel( 0, item->m_item.item().comment() );
 // m_commentLabel->setFont(QToolTip::font());
  m_commentLabel->setPalette(QToolTip::palette());

  QPoint rightPoint = m_completionPopup->mapToGlobal(QPoint(m_completionPopup->width(),0));
  QPoint leftPoint = m_completionPopup->mapToGlobal(QPoint(0,0));
  QRect screen = QApplication::desktop()->screenGeometry ( m_commentLabel );
  QPoint finalPoint;
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

KateArgHint::KateArgHint( KateView* parent, const char* name )
    : Q3Frame( parent, name, Qt::WType_Popup )
{
    setBackgroundColor( Qt::black );
    setPaletteForegroundColor( Qt::black );

    labelDict.setAutoDelete( true );
    layout = new QVBoxLayout( this, 1, 2 );
    layout->setAutoAdd( true );
    editorView = parent;

    m_markCurrentFunction = true;

    setFocusPolicy( Qt::StrongFocus );
    setFocusProxy( parent );

    reset( -1, -1 );
}

KateArgHint::~KateArgHint()
{
}

void KateArgHint::setArgMarkInfos( const QString& wrapping, const QString& delimiter )
{
    m_wrapping = wrapping;
    m_delimiter = delimiter;
    m_markCurrentFunction = true;
}

void KateArgHint::reset( int line, int col )
{
    m_functionMap.clear();
    m_currentFunction = -1;
    labelDict.clear();

    m_currentLine = line;
    m_currentCol = col - 1;
}

void KateArgHint::slotDone(bool completed)
{
    hide();

    m_currentLine = m_currentCol = -1;

    emit argHintHidden();
    if (completed)
        emit argHintCompleted();
    else
        emit argHintAborted();
}

void KateArgHint::cursorPositionChanged( KateView* view, int line, int col )
{
    if( m_currentCol == -1 || m_currentLine == -1 ){
        slotDone(false);
        return;
    }

    int nCountDelimiter = 0;
    int count = 0;

    QString currentTextLine = view->doc()->line( line );
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

void KateArgHint::addFunction( int id, const QString& prot )
{
    m_functionMap[ id ] = prot;
    QLabel* label = new QLabel( prot.trimmed().simplifyWhiteSpace(), this );
    label->setBackgroundColor( QColor(255, 255, 238) );
    label->show();
    labelDict.insert( id, label );

    if( m_currentFunction < 0 )
        setCurrentFunction( id );
}

void KateArgHint::setCurrentFunction( int currentFunction )
{
    if( m_currentFunction != currentFunction ){

        if( currentFunction < 0 )
            currentFunction = (int)m_functionMap.size() - 1;

        if( currentFunction > (int)m_functionMap.size()-1 )
            currentFunction = 0;

        if( m_markCurrentFunction && m_currentFunction >= 0 ){
            QLabel* label = labelDict[ m_currentFunction ];
            label->setFont( font() );
        }

        m_currentFunction = currentFunction;

        if( m_markCurrentFunction ){
            QLabel* label = labelDict[ currentFunction ];
            QFont fnt( font() );
            fnt.setBold( true );
            label->setFont( fnt );
        }

        adjustSize();
    }
}

void KateArgHint::show()
{
    Q3Frame::show();
    adjustSize();
}

bool KateArgHint::eventFilter( QObject*, QEvent* e )
{
    if( isVisible() && e->type() == QEvent::KeyPress ){
        QKeyEvent* ke = static_cast<QKeyEvent*>( e );
        if( (ke->state() & Qt::ControlModifier) && ke->key() == Qt::Key_Left ){
            setCurrentFunction( currentFunction() - 1 );
            ke->accept();
            return true;
        } else if( ke->key() == Qt::Key_Escape ){
            slotDone(false);
            return false;
        } else if( (ke->state() & Qt::ControlModifier) && ke->key() == Qt::Key_Right ){
            setCurrentFunction( currentFunction() + 1 );
            ke->accept();
            return true;
        }
    }

    return false;
}

void KateArgHint::adjustSize( )
{
    QRect screen = QApplication::desktop()->screenGeometry( pos() );

    Q3Frame::adjustSize();
    if( width() > screen.width() )
        resize( screen.width(), height() );

    if( x() + width() > screen.width() )
        move( screen.width() - width(), y() );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
