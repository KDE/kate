/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>

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

 // $Id$
 
#include "katebookmarks.h"
#include "katebookmarks.moc"

#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kstringhandler.h>
#include <kdebug.h>

#include <qregexp.h>
#include <qmemarray.h>

#include "../interfaces/document.h"
#include "../interfaces/view.h"

/**
   Utility: selection sort
   sort a QMemArray<uint> in ascending order.
   max it the largest (zerobased) index to sort. 
   To sort the entire array: ssort( *array, array.size() -1 );
   This is only efficient if ran only once.
*/
static void ssort( QMemArray<uint> &a, int max )
{
  uint tmp, j, maxpos;
  for ( uint h = max; h >= 1; h-- )
  {
    maxpos = 0;
    for ( j = 0; j <= h; j++ )
      maxpos = a[j] > a[maxpos] ? j : maxpos;
    tmp = a[maxpos];
    a[maxpos] = a[h];
    a[h] = tmp; 
  }
}

// TODO add a insort() or bubble_sort - more efficient for aboutToShow() ?

KateBookmarks::KateBookmarks( Kate::View* view, Sorting sort )
  : QObject( view, "kate bookmarks" )
  , m_view( view )
  , m_sorting( sort )
{
}

KateBookmarks::~KateBookmarks()
{
}

void KateBookmarks::createActions( KActionCollection* ac )
{
  m_bookmarkMenu = new KActionMenu(
    i18n("&Bookmarks"), ac, "bookmarks" );
  KPopupMenu *m = m_bookmarkMenu->popupMenu();
  
  // setup bookmark menu
  m_bookmarkToggle = new KAction(
    i18n("Toggle &Bookmark"), CTRL+Key_B,
    this, SLOT(toggleBookmark()),
    ac, "bookmarks_toggle" );
  m_bookmarkToggle->plug( m ); // make available
  m_bookmarkClear = new KAction(
    i18n("Clear Bookmarks"), 0,
    this, SLOT(clearBookmarks()),
    ac, "bookmarks_clear");
  m_bookmarkClear->plug( m );  // make available
  m_goNext = new KAction(
    "Next Bookmark", ALT + Key_PageDown,
    this, SLOT(goNext()),
    ac, "bookmarks_next");
  m_goNext->plug( m );
  m_goPrevious = new KAction(
    "Previous Bookmark", ALT + Key_PageUp,
    this, SLOT(goPrevious()),
    ac, "bookmarks_pevious");
  m_goPrevious->plug( m );
  
  // connect bookmarks menu aboutToshow
  connect( m, SIGNAL(aboutToShow()),
           this, SLOT(bookmarkMenuAboutToShow()));
  // anders: I ensure the next/prev actions are available
  // and reset their texts (for edit shortcuts dialog, call me picky!).
  // TODO - come up with a better solution, please anyone?
  connect( m, SIGNAL(aboutToHide()),
           this, SLOT(bookmarkMenuAboutToHide()) );
}

void KateBookmarks::toggleBookmark ()
{
  uint mark = m_view->getDoc()->mark( m_view->cursorLine() );
  if( mark & KTextEditor::MarkInterface::markType01 )
    m_view->getDoc()->removeMark( m_view->cursorLine(),
        KTextEditor::MarkInterface::markType01 );
  else
    m_view->getDoc()->addMark( m_view->cursorLine(),
        KTextEditor::MarkInterface::markType01 );
}

void KateBookmarks::clearBookmarks ()
{
  m_marks = m_view->getDoc()->marks();
  QPtrListIterator<KTextEditor::Mark> it( m_marks );
  for( ; *it; ++it ) {
    m_view->getDoc()->removeMark( (*it)->line, KTextEditor::MarkInterface::markType01 );
  }
}

void KateBookmarks::bookmarkMenuAboutToShow()
{
  m_bookmarkMenu->popupMenu()->clear();
  m_bookmarkToggle->plug( m_bookmarkMenu->popupMenu() );
  m_bookmarkClear->plug( m_bookmarkMenu->popupMenu() );
  m_bookmarkMenu->popupMenu()->insertSeparator();
  
  KTextEditor::Mark *next = 0;
  KTextEditor::Mark *prev = 0;
  uint line = m_view->cursorLine();
  
  const QRegExp re("&(?!&)");
  m_marks = m_view->getDoc()->marks();
  int idx( -1 );
  QMemArray<uint> sortArray( m_marks.count() );
  QPtrListIterator<KTextEditor::Mark> it( m_marks );
  for( int i = 0; *it; ++it, ++i ) {
    if( (*it)->type & KTextEditor::MarkInterface::markType01 ) {
      QString bText = KStringHandler::rsqueeze( m_view->getDoc()->textLine( (*it)->line ), 32 );
      bText.replace(re, "&&"); // kill undesired accellerators!
      if ( m_sorting == Position )
      {
        sortArray[i] = (*it)->line;
        ssort( sortArray, i );
        idx = sortArray.find( (*it)->line ) + 3;
      }
        m_bookmarkMenu->popupMenu()->insertItem(
        QString("%1 - \"%2\"").arg( (*it)->line+1 ).arg( bText ),
        this, SLOT (gotoBookmark(int)), 0, i, idx );
      if ( (*it)->line < line )
      {
        if ( ! prev || prev->line < (*it)->line )
          prev = (*it);
      }
      else if ( (*it)->line > line )
      {
        if ( ! next || next->line > (*it)->line )
          next = (*it);
      }
    }
  }
  
  idx = 3;
  if ( next )
  {
    m_goNext->setText( i18n("&Next: %1 - \"%2\"").arg( next->line + 1 )
        .arg( KStringHandler::rsqueeze( m_view->getDoc()->textLine( next->line ), 24 ) ) );
    m_goNext->plug( m_bookmarkMenu->popupMenu(), idx );
    idx++;
  }
  if ( prev )
  {
    m_goPrevious->setText( i18n("&Previous: %1 - \"%2\"").arg(prev->line + 1 )
        .arg( KStringHandler::rsqueeze( m_view->getDoc()->textLine( prev->line ), 24 ) ) );
    m_goPrevious->plug( m_bookmarkMenu->popupMenu(), idx );
    idx++;
  }
  if ( next || prev ) 
    m_bookmarkMenu->popupMenu()->insertSeparator( idx );
}

/*
   Make sure next/prev actions are plugged, and have a clean text
*/
void KateBookmarks::bookmarkMenuAboutToHide()
{
  m_goNext->setText( i18n("Next Bookmark") );
  m_goNext->plug( m_bookmarkMenu->popupMenu() );
  m_goPrevious->setText( i18n("Previous Bookmark") );
  m_goPrevious->plug( m_bookmarkMenu->popupMenu() );
}

void KateBookmarks::gotoBookmark( int n )
{
  m_view->setCursorPosition( m_marks.at(n)->line, 0 );
}

void KateBookmarks::goNext()
{
  m_marks = m_view->getDoc()->marks();
  if ( ! m_marks.count() ) 
    return;
    
  uint line = m_view->cursorLine();
  QMemArray<uint> a( m_marks.count() );
  QPtrListIterator<KTextEditor::Mark> it( m_marks );
  for ( int i=0; *it; ++i, ++it )
    a[i] = (*it)->line;
  ssort( a, m_marks.count()-1 );
  for ( uint j=0; j < m_marks.count() ; j++ )
  {
    if ( a[j] > line ) 
    {
      m_view->setCursorPosition( a[j], 0 );
      return;
    }
  }       
}

void KateBookmarks::goPrevious()
{
  m_marks = m_view->getDoc()->marks();
  if ( ! m_marks.count() ) 
    return;
  
  uint line = m_view->cursorLine();
  QMemArray<uint> a( m_marks.count() );
  QPtrListIterator<KTextEditor::Mark> it( m_marks );
  for ( int i=0; *it; ++i, ++it )
    a[i] = (*it)->line;
  ssort( a, m_marks.count()-1 );
  for ( int j = m_marks.count() - 1; j >= 0 ; j-- )
  {
    if ( a[j] < line )
    {
      m_view->setCursorPosition( a[j], 0 );
      return;
    }
  }       
}

// vim: noet ts=2
