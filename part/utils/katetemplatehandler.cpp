/* This file is part of the KDE libraries
  Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "katetemplatehandler.h"
#include "katetemplatehandler.moc"
#include "katedocument.h"
#include "katesmartcursor.h"
#include "kateview.h"
#include "kateconfig.h"
#include "katerenderer.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/smartcursor.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/range.h>
#include <ktexteditor/attribute.h>

#include <QtCore/QRegExp>
#include <kdebug.h>

KateTemplateHandler::KateTemplateHandler(
  KateDocument *doc,
  const KTextEditor::Cursor& position,
  const QString &templateString,
  const QMap<QString, QString> &initialValues )
    : QObject( doc )
    , m_doc( doc )
    , m_currentTabStop( -1 )
    , m_currentRange( 0 )
    , m_initOk( false )
    , m_recursion( false )
    , m_templateRange(0)
{
  QList<KateTemplateHandlerPlaceHolderInfo> buildList;
  QRegExp rx( "([$%])\\{([^}\\s]+)\\}" );
  rx.setMinimal( true );
  int oldPos = 0;
  QString insertString = templateString;

  while ( true )
  {
    const int pos = rx.indexIn( insertString, oldPos );

    if ( pos < 0 )
      break; // leave loop

      if ( pos > oldPos && insertString[ pos - 1 ] == '\\' )
      {
          insertString.remove( pos - 1, 1 );
          oldPos = pos;
          continue;
      }

      const QString placeholder = rx.cap( 2 );
      const QString value = initialValues[ placeholder ];

      // don't add %{MACRO} to the tab navigation, unless there was not value
      if ( rx.cap( 1 ) != "%" || placeholder == value )
        buildList.append( KateTemplateHandlerPlaceHolderInfo( pos, value.length(), placeholder ) );

      insertString.replace( pos, rx.matchedLength(), value );
      oldPos = pos + value.length();
  }

  doc->editStart();

  if ( !doc->insertText( position, insertString ) )
  {
    deleteLater();
    doc->editEnd();
    return ;
  }

  if ( buildList.isEmpty() )
  {
    m_initOk = true;
    deleteLater();
    doc->editEnd();
    return ;
  }

  doc->undoSafePoint();
  doc->editEnd();
  generateRangeTable( position, insertString, buildList );

  connect( doc, SIGNAL( textInserted(KTextEditor::Document*, const KTextEditor::Range& ) ), this, SLOT( slotTextInserted(KTextEditor::Document*, const KTextEditor::Range& ) ) );
  connect( doc, SIGNAL( aboutToRemoveText( const KTextEditor::Range& ) ), this, SLOT( slotAboutToRemoveText( const KTextEditor::Range& ) ) );
  connect( doc, SIGNAL( textRemoved() ), this, SLOT( slotTextRemoved() ) );

  ( *this ) ( Qt::Key_Tab );
}

KateTemplateHandler::~KateTemplateHandler()
{
  delete m_templateRange;
#ifdef __GNUC__
  #warning delete placeholder infos here
#endif
}

void KateTemplateHandler::slotRangeDeleted(KTextEditor::SmartRange* range) {
  if (range==m_templateRange) m_templateRange=0;
}

void KateTemplateHandler::generateRangeTable( const KTextEditor::Cursor& insertPosition, const QString& insertString, const QList<KateTemplateHandlerPlaceHolderInfo> &buildList )
{
  KateRendererConfig *config=m_doc->activeKateView()->renderer()->config();
  kDebug(13020) << config->templateEditablePlaceholderColor()
                << config->templateBackgroundColor()
                << config->templateFocusedEditablePlaceholderColor()
                << config->templateNotEditablePlaceholderColor();

  QColor color;

  // editable placeholder
  color=config->templateEditablePlaceholderColor();
  color.setAlpha(0x88);
  KTextEditor::Attribute::Ptr attributeEditableElement(new KTextEditor::Attribute());
  attributeEditableElement->setBackground(QBrush(color));

  // focused editable placeholder
  color=config->templateFocusedEditablePlaceholderColor();
  color.setAlpha(0x88);
  KTextEditor::Attribute::Ptr attributeEditableElementFocus(new KTextEditor::Attribute());
  attributeEditableElementFocus->setBackground(QBrush(color));
  attributeEditableElement->setDynamicAttribute(KTextEditor::Attribute::ActivateCaretIn,attributeEditableElementFocus);

  // not editable/slave placeholder
  color=config->templateNotEditablePlaceholderColor();
  color.setAlpha(0x88);
  KTextEditor::Attribute::Ptr attributeNotEditableElement(new KTextEditor::Attribute());
  attributeNotEditableElement->setBackground(QBrush(color));

  // template background
  color=config->templateBackgroundColor();
  color.setAlpha(0x88);
  KTextEditor::Attribute::Ptr attributeTemplateBackground(new KTextEditor::Attribute());
  attributeTemplateBackground->setBackground(QBrush(color));

  KTextEditor::SmartCursor *endC= m_doc->newSmartCursor(insertPosition);
  endC->advance(insertString.length());
  m_templateRange=m_doc->newSmartRange(KTextEditor::Range(insertPosition,*endC));
  connect(m_templateRange->primaryNotifier(),SIGNAL(rangeDeleted(KTextEditor::SmartRange*)),this,SLOT(slotRangeDeleted(KTextEditor::SmartRange*)));
  kDebug(13020) << insertPosition << "--" << *endC << "++++" << *m_templateRange;
  delete endC;
  m_templateRange->setAttribute(attributeTemplateBackground);

  uint line = insertPosition.line();
  uint col = insertPosition.column();
  uint colInText = 0;

  foreach (const KateTemplateHandlerPlaceHolderInfo& info, buildList)
  {
    bool firstOccurrence=false;
    KateTemplatePlaceHolder *ph = m_dict[ info.placeholder ];

    if ( !ph )
    {
      firstOccurrence=true;
      ph = new KateTemplatePlaceHolder(( info.placeholder == "cursor" ),true,false);

      m_dict.insert( info.placeholder, ph );

      if ( !ph->isCursor ) m_tabOrder.append( ph );
    }

    // FIXME handle space/tab replacement correctly make it use of the indenter
    while ( colInText < info.begin )
    {
      ++col;

      if ( insertString.at( colInText ) == '\n' )
      {
        col = 0;
        line++;
      }

      ++colInText;
    }

    KTextEditor::SmartCursor *tmpC=m_doc->newSmartCursor(KTextEditor::Cursor(line,col));;
    tmpC->advance(info.len);
    KTextEditor::SmartRange *hlr=m_doc->newSmartRange(KTextEditor::Range(KTextEditor::Cursor(line,col),*tmpC),m_templateRange,KTextEditor::SmartRange::ExpandRight);
    hlr->setAttribute(firstOccurrence?attributeEditableElement:attributeNotEditableElement);
    hlr->setParentRange(m_templateRange);
    delete tmpC;
    ph->ranges.append(hlr);

    colInText += info.len;
    col += info.len;
      //hlr->allowZeroLength();
    //hlr->setBehavior(KateSmartRange::ExpandRight);
  }

  KateTemplatePlaceHolder *cursor = m_dict[ "cursor" ];

  if ( cursor ) m_tabOrder.append( cursor );
  m_doc->addHighlightToDocument(m_templateRange,true);
}

void KateTemplateHandler::slotTextInserted(KTextEditor::Document*, const KTextEditor::Range& range)
{
  if (m_doc->isEditRunning() && (!m_doc->isWithUndo())) return;

#ifdef __GNUC__
#warning FIXME undo/redo detection
#endif
  kDebug(13020) << "*****";
  if ( m_recursion ) return ;
  kDebug(13020) << "no recurssion";

  KTextEditor::Cursor cur=range.start();
  KTextEditor::Cursor curE=range.end();

  kDebug(13020) << cur << "---" << *m_currentRange;

  kDebug(13020) << m_doc->text(range);

  if ( ( !m_currentRange ) ||
       ( ( !m_currentRange->contains( cur ) ) && ( ! ( ( m_currentRange->start() == m_currentRange->end() ) && ( (m_currentRange->end() == cur) || 
(m_currentRange->start()==curE) ) ) )
       ) ) locateRange( cur,curE );

  if ( !m_currentRange ) return ;


  bool expandedLeft=false;

  if (m_currentRange->start()==curE) {
    expandedLeft=true;
    m_currentRange->setRange(KTextEditor::Range(cur,m_currentRange->end()));
  }

  kDebug(13020) << "m_currentRange is not null";

  KateTemplatePlaceHolder *ph = m_tabOrder.at( m_currentTabStop );

  m_recursion = true;
  m_doc->editStart( /*false*/ );

  QString sourceText = m_doc->text ( *m_currentRange );
  kDebug(13020) << ph->isReplacableSpace << "--->" << sourceText << "<---";
  if ( (sourceText.length()==0) || (ph->isReplacableSpace && (sourceText==" ")) ) {
    ph->isReplacableSpace = true;
    sourceText=QString(" ");
    KTextEditor::Cursor start = m_currentRange->start();
    m_doc->insertText( m_currentRange->start(), sourceText );
    m_currentRange->setRange(KTextEditor::Range(start,m_currentRange->end()));
    m_doc->activeView()->setSelection( *m_currentRange );
    kDebug() << "inserted a replaceable space:" << *m_currentRange;
  }
  else {
   if (ph->isReplacableSpace && sourceText.startsWith(' ')) {
    m_doc->removeText( KTextEditor::Range(m_currentRange->start(),1));
    sourceText=sourceText.right(sourceText.length()-1);
   } else if (ph->isReplacableSpace && expandedLeft) {
    m_doc->removeText( KTextEditor::Range(KTextEditor::Cursor(m_currentRange->end().line(),m_currentRange->end().column()-1),1) );
    sourceText=sourceText.left(sourceText.length()-1);
   }
   ph->isReplacableSpace = false;
  }
  ph->isInitialValue = false;

  bool undoDontMerge = m_doc->undoDontMerge();
  //Q_ASSERT( !m_doc->isEditRunning() );


    foreach ( KTextEditor::SmartRange* range, ph->ranges )
  {
    if ( range == m_currentRange ) continue;
    kDebug(13020) << "updating a range:" << *range;
    KTextEditor::Cursor start = range->start();
    KTextEditor::Cursor end = range->end();
    //m_doc->removeText( start.line(), start.column(), end.line(), end.column(), false );
    //m_doc->insertText( start.line(), start.column(), sourceText );
    m_doc->removeText( *range, false );
    kDebug(13020) << "updating a range(2):" << *range;
    m_doc->insertText( start, sourceText );
    range->setRange(KTextEditor::Range(start,range->end()));
    kDebug(13020) << "updating a range(3):" << *range;
  }

  m_doc->setUndoDontMerge(false);
  m_doc->setUndoAllowComplexMerge(true);
  m_doc->undoSafePoint();
  m_doc->editEnd();
  m_doc->setUndoDontMerge(undoDontMerge);
  m_recursion = false;

  if ( ph->isCursor ) deleteLater();
}

void KateTemplateHandler::locateRange( const KTextEditor::Cursor& cursor, const KTextEditor::Cursor& cursor2 )
{
  /* if (m_currentRange) {
    m_doc->tagLines(m_currentRange->start().line(),m_currentRange->end().line());

   }*/

  for ( int i = 0;i < m_tabOrder.count();i++ )
  {
    KateTemplatePlaceHolder *ph = m_tabOrder.at( i );

    foreach ( KTextEditor::SmartRange* range, ph->ranges)
    {
      kDebug(13020) << "CURSOR:" << cursor << "RANGE:" << *range;
      if ( range->contains( cursor ) )
      {
        m_currentTabStop = i;
        m_currentRange = range;
        //m_doc->tagLines(m_currentRange->start().line(),m_currentRange->end().line());
        return ;
      }
    }

  }

  for ( int i = 0;i < m_tabOrder.count();i++ )
  {
    KateTemplatePlaceHolder *ph = m_tabOrder.at( i );

    foreach ( KTextEditor::SmartRange* range, ph->ranges)
    {
      kDebug(13020) << "CURSOR:" << cursor << "RANGE:" << *range;
      if ( range->contains( cursor2 ) )
      {
        m_currentTabStop = i;
        m_currentRange = range;
        //m_doc->tagLines(m_currentRange->start().line(),m_currentRange->end().line());
        return ;
      }
    }

  }

  m_currentRange = 0;
  /*while (m_ranges->count()>0)
   delete (m_ranges->take(0));
  disconnect(m_ranges,0,0,0);
  delete m_ranges;*/
  KateTemplatePlaceHolder *cur = m_dict[ "cursor" ];
  if (cur) {
    if (cur->isInitialValue) { //this check should never be important
      m_doc->removeText(*(cur->ranges[0]));
    }
  }
  deleteLater();
}


bool KateTemplateHandler::operator() ( int key )
{
  if ( key==Qt::Key_Tab )
  {
    m_currentTabStop++;

    if ( m_currentTabStop >= ( int ) m_tabOrder.count() )
      m_currentTabStop = 0;
  }
  else
  {
    m_currentTabStop--;

    if ( m_currentTabStop < 0 ) m_currentTabStop = m_tabOrder.count() - 1;
  }

  m_currentRange = m_tabOrder.at( m_currentTabStop )->ranges[0];

  KateTemplatePlaceHolder *ph=m_tabOrder.at( m_currentTabStop );
  if (  ph->isInitialValue || ph->isReplacableSpace)
  {
    m_doc->activeView()->setSelection( *m_currentRange );
  }
  else m_doc->activeView()->setSelection( KTextEditor::Range(m_currentRange->end(), m_currentRange->end()) );

  KTextEditor::Cursor curpos=m_currentRange->end();

  m_doc->activeView()->setCursorPosition( curpos );
  m_doc->activeKateView()->tagLine( m_currentRange->end() );
  return true;
}

void KateTemplateHandler::slotAboutToRemoveText( const KTextEditor::Range& range )
{
  if ( m_recursion ) return ;

  kDebug(13020) << "(remove):" << range;

  if (range.start()==range.end()) return;

  if (m_currentRange) {
    KTextEditor::Cursor cur=range.start();
    kDebug(13020) << cur << "---" << *m_currentRange;
  }
  if ( m_currentRange && ( !m_currentRange->contains( range.start() ) ) ) {
    kDebug(13020) << "about to locate range";
    locateRange( range.start(), KTextEditor::Cursor(-1,-1) );
  }

  if ( m_currentRange != 0 )
  {
    if ( range.end() <= m_currentRange->end() ) return ;
  }

  kDebug(13020) << "disconnect & leave";
  if ( m_doc )
  {
    disconnect( m_doc, SIGNAL( textInserted(KTextEditor::Document*, const KTextEditor::Range& ) ), this, SLOT( slotTextInserted(KTextEditor::Document*, const KTextEditor::Range& ) ) );
    disconnect( m_doc, SIGNAL( aboutToRemoveText( const KTextEditor::Range& ) ), this, SLOT( slotAboutToRemoveText( const KTextEditor::Range& ) ) );
    disconnect( m_doc, SIGNAL( textRemoved() ), this, SLOT( slotTextRemoved() ) );
  }

  deleteLater();
}

void KateTemplateHandler::slotTextRemoved()
{
  if ( m_recursion ) return ;
  if ( !m_currentRange ) return ;

  slotTextInserted( m_doc,*m_currentRange);
}

