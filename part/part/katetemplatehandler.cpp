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
#include "katerangetype.h"

#include <ktexteditor/attribute.h>

#include <qregexp.h>
#include <kdebug.h>
#include <q3valuelist.h>

KateTemplateHandler::KateTemplateHandler(
  KateDocument *doc,
  const KTextEditor::Cursor& position,
  const QString &templateString,
  const QMap<QString, QString> &initialValues )
    : QObject( doc )
    , KateKeyInterceptorFunctor()
    , m_doc( doc )
    , m_currentTabStop( -1 )
    , m_currentRange( 0 )
    , m_initOk( false )
    , m_recursion( false )
{
  connect( m_doc, SIGNAL( destroyed() ), this, SLOT( slotDocumentDestroyed() ) );
    //m_ranges = new KateRangeList( false, this ); //false/*,this*/);

  if ( !m_doc->setTabInterceptor( this ) )
  {
    deleteLater();
    return ;
  }

  /*KateArbitraryHighlight *kah = doc->arbitraryHL();
  KateArbitraryHighlightRange *hlr=new KateArbitraryHighlightRange(doc,KTextEditor::Cursor(line,column),
   KTextEditor::Cursor(line,column+3));
  hlr->setUnderline(true);
  hlr->setOverline(true);
  l->append(hlr);*/
  Q3ValueList<KateTemplateHandlerPlaceHolderInfo> buildList;
  QRegExp rx( "([$%])\\{([^}\\s]+)\\}" );
  rx.setMinimal( true );
  int pos = 0;
  int opos = 0;
  QString insertString = templateString;

  while ( pos >= 0 )
  {
    pos = rx.search( insertString, pos );

    if ( pos > -1 )
    {
      if ( ( pos - opos ) > 0 )
      {
        if ( insertString[ pos - 1 ] == '\\' )
        {
          insertString.remove( pos - 1, 1 );
          opos = pos;
          continue;
        }
      }

      QString placeholder = rx.cap( 2 );
      QString value = initialValues[ placeholder ];

      // don't add %{MACRO} to the tab navigation, unless there was not value
      if ( rx.cap( 1 ) != "%" || placeholder == value )
        buildList.append( KateTemplateHandlerPlaceHolderInfo( pos, value.length(), placeholder ) );

      insertString.replace( pos, rx.matchedLength(), value );
      pos += value.length();
      opos = pos;
    }
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
  //kah->addHighlightToDocument( m_ranges );

    //m_ranges->tagAll();

  /* connect(doc,SIGNAL(charactersInteractivelyInserted(int ,int ,const QString&)),this,
   SLOT(slotCharactersInteractivlyInserted(int,int,const QString&)));
   connect(doc,SIGNAL(charactersSemiInteractivelyInserted(int ,int ,const QString&)),this,
   SLOT(slotCharactersInteractivlyInserted(int,int,const QString&)));*/
  connect( doc, SIGNAL( textInserted( int, int ) ), this, SLOT( slotTextInserted( int, int ) ) );
  connect( doc, SIGNAL( aboutToRemoveText( const KTextEditor::Range& ) ), this, SLOT( slotAboutToRemoveText( const KTextEditor::Range& ) ) );
  connect( doc, SIGNAL( textRemoved() ), this, SLOT( slotTextRemoved() ) );

  ( *this ) ( Qt::Key_Tab );
}

KateTemplateHandler::~KateTemplateHandler()
{
  if ( m_doc )
  {
    m_doc->removeTabInterceptor( this );
  }
}

void KateTemplateHandler::slotDocumentDestroyed() {m_doc = 0;}

void KateTemplateHandler::generateRangeTable( const KTextEditor::Cursor& insertPosition, const QString& insertString, const Q3ValueList<KateTemplateHandlerPlaceHolderInfo> &buildList )
{
  uint line = insertPosition.line();
  uint col = insertPosition.column();
  uint colInText = 0;

  for ( Q3ValueList<KateTemplateHandlerPlaceHolderInfo>::const_iterator it = buildList.begin();it != buildList.end();++it )
  {
    KateTemplatePlaceHolder *ph = m_dict[ ( *it ).placeholder ];

    if ( !ph )
    {
      ph = new KateTemplatePlaceHolder(m_doc);
      KTextEditor::Attribute* attrib = new KTextEditor::Attribute();
      attrib->setUnderline( true );
      attrib->setOverline( true );
      KateRangeType* rt = new KateRangeType();
      rt->addAttribute(attrib, KateRangeType::activateNone, true);
        //ph->ranges.setRangeType(rt);

      ph->isInitialValue = true;
      ph->isCursor = ( ( *it ).placeholder == "cursor" );
      m_dict.insert( ( *it ).placeholder, ph );

      if ( !ph->isCursor ) m_tabOrder.append( ph );
    }

    // FIXME handle space/tab replacement correctly make it use of the indenter
    while ( colInText < ( *it ).begin )
    {
      ++col;

      if ( insertString.at( colInText ) == '\n' )
      {
        col = 0;
        line++;
      }

      ++colInText;
    }

      //KateSmartRange *hlr = new KateSmartRange( m_doc, KTextEditor::Cursor( line, col ),
      //KTextEditor::Cursor( line, ( *it ).len + col ), ph->ranges.topRange() );
    colInText += ( *it ).len;
    col += ( *it ).len;
      //hlr->allowZeroLength();
    //hlr->setBehaviour(KateSmartRange::ExpandRight);
  }

  KateTemplatePlaceHolder *cursor = m_dict[ "cursor" ];

  if ( cursor ) m_tabOrder.append( cursor );
}

void KateTemplateHandler::slotTextInserted( int line, int col )
{
#ifdef __GNUC__
#warning FIXME undo/redo detection
#endif

  if ( m_recursion ) return ;

  //if (m_editSessionNumber!=0) return; // assume that this is due an udno/redo operation right now
  KTextEditor::Cursor cur( line, col );

  if ( ( !m_currentRange ) ||
       ( ( !m_currentRange->contains( cur ) ) && ( ! ( ( m_currentRange->start() == m_currentRange->end() ) && m_currentRange->end() == cur ) )
       ) ) locateRange( cur );

  if ( !m_currentRange ) return ;

  KateTemplatePlaceHolder *ph = m_tabOrder.at( m_currentTabStop );

  QString sourceText = m_doc->text ( *m_currentRange );

  ph->isInitialValue = false;
  bool undoDontMerge = m_doc->m_undoDontMerge;
  Q_ASSERT( m_doc->editSessionNumber == 0 );
  m_recursion = true;

  m_doc->editStart( /*false*/ );

    /*foreach ( KateSmartRange* range, ph->ranges.topRange()->childRanges() )
  {
    if ( range == m_currentRange ) continue;

    KTextEditor::Cursor start = range->start();
    KTextEditor::Cursor end = range->end();
    m_doc->removeText( start.line(), start.column(), end.line(), end.column(), false );
    m_doc->insertText( start.line(), start.column(), sourceText );
  }*/

  m_doc->m_undoDontMerge = false;
  m_doc->m_undoComplexMerge = true;
  m_doc->undoSafePoint();
  m_doc->editEnd();
  m_doc->m_undoDontMerge = undoDontMerge;
  m_recursion = false;

  if ( ph->isCursor ) deleteLater();
}

void KateTemplateHandler::locateRange( const KTextEditor::Cursor& cursor )
{
  /* if (m_currentRange) {
    m_doc->tagLines(m_currentRange->start().line(),m_currentRange->end().line());

   }*/

  for ( uint i = 0;i < m_tabOrder.count();i++ )
  {
    KateTemplatePlaceHolder *ph = m_tabOrder.at( i );

      /*foreach ( KateSmartRange* range, ph->ranges.topRange()->childRanges() )
    {
      if ( range->includes( cursor ) )
      {
        m_currentTabStop = i;
        m_currentRange = range;
        //m_doc->tagLines(m_currentRange->start().line(),m_currentRange->end().line());
        return ;
      }
    }*/

  }

  m_currentRange = 0;
  /*while (m_ranges->count()>0)
   delete (m_ranges->take(0));
  disconnect(m_ranges,0,0,0);
  delete m_ranges;*/
  deleteLater();
}


bool KateTemplateHandler::operator() ( KKey key )
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

    //m_currentRange = m_tabOrder.at( m_currentTabStop )->ranges.topRange()->firstChildRange();

  if ( m_tabOrder.at( m_currentTabStop ) ->isInitialValue )
  {
    m_doc->activeView()->setSelection( *m_currentRange );
  }
  else m_doc->activeView()->setSelection( KTextEditor::Range(m_currentRange->end(), m_currentRange->end()) );

  m_doc->activeView()->setCursorPosition( m_currentRange->end() );
  m_doc->activeKateView()->tagLine( m_currentRange->end() );

  return true;
}

void KateTemplateHandler::slotAboutToRemoveText( const KTextEditor::Range& range )
{
  if ( m_recursion ) return ;

  if ( m_currentRange && ( !m_currentRange->contains( range.start() ) ) ) locateRange( range.start() );

  if ( m_currentRange != 0 )
  {
    if ( m_currentRange->end() <= range.end() ) return ;
  }

  if ( m_doc )
  {
    disconnect( m_doc, SIGNAL( textInserted( int, int ) ), this, SLOT( slotTextInserted( int, int ) ) );
    disconnect( m_doc, SIGNAL( aboutToRemoveText( const KTextEditor::Range& ) ), this, SLOT( slotAboutToRemoveText( const KTextEditor::Range& ) ) );
    disconnect( m_doc, SIGNAL( textRemoved() ), this, SLOT( slotTextRemoved() ) );
  }

  deleteLater();
}

void KateTemplateHandler::slotTextRemoved()
{
  if ( m_recursion ) return ;
  if ( !m_currentRange ) return ;

  slotTextInserted( m_currentRange->start().line(), m_currentRange->start().column() );
}

