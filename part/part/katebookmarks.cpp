/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003, 2004 Anders Lund <anders.lund@lund.tdcadsl.dk>
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

#include "katebookmarks.h"
#include "katebookmarks.moc"

#include "katedocument.h"
#include "kateview.h"

#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kstringhandler.h>
#include <kxmlguiclient.h>
#include <kxmlguifactory.h>

#include <qregexp.h>
#include <qmemarray.h>
#include <qevent.h>

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

KateBookmarks::KateBookmarks( KateView* view, Sorting sort )
  : QObject( view, "kate bookmarks" )
  , m_view( view )
  , m_sorting( sort )
{
  connect (view->getDoc(), SIGNAL(marksChanged()), this, SLOT(marksChanged()));
  _tries=0;
  m_bookmarksMenu = 0L;
}

KateBookmarks::~KateBookmarks()
{
}

void KateBookmarks::createActions( KActionCollection* ac )
{
  m_bookmarkToggle = new KToggleAction(
    i18n("Set &Bookmark"), "bookmark", CTRL+Key_B,
    this, SLOT(toggleBookmark()),
    ac, "bookmarks_toggle" );
  m_bookmarkToggle->setWhatsThis(i18n("If a line has no bookmark then add one, otherwise remove it."));
  m_bookmarkToggle->setCheckedState( i18n("Clear &Bookmark") );

  m_bookmarkClear = new KAction(
    i18n("Clear &All Bookmarks"), 0,
    this, SLOT(clearBookmarks()),
    ac, "bookmarks_clear");
  m_bookmarkClear->setWhatsThis(i18n("Remove all bookmarks of the current document."));

  m_goNext = new KAction(
    i18n("Next Bookmark"), "next", ALT + Key_PageDown,
    this, SLOT(goNext()),
    ac, "bookmarks_next");
  m_goNext->setWhatsThis(i18n("Go to the next bookmark."));

  m_goPrevious = new KAction(
    i18n("Previous Bookmark"), "previous", ALT + Key_PageUp,
    this, SLOT(goPrevious()),
    ac, "bookmarks_previous");
  m_goPrevious->setWhatsThis(i18n("Go to the previous bookmark."));

  m_bookmarksMenu = (new KActionMenu(i18n("&Bookmarks"), ac, "bookmarks"))->popupMenu();

  //connect the aboutToShow() and aboutToHide() signals with
  //the bookmarkMenuAboutToShow() and bookmarkMenuAboutToHide() slots
  connect( m_bookmarksMenu, SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuAboutToShow()));
  connect( m_bookmarksMenu, SIGNAL(aboutToHide()), this, SLOT(bookmarkMenuAboutToHide()) );

  marksChanged ();
  bookmarkMenuAboutToHide();

  connect( m_view, SIGNAL( gotFocus( Kate::View * ) ), this, SLOT( slotViewGotFocus( Kate::View * ) ) );
  connect( m_view, SIGNAL( lostFocus( Kate::View * ) ), this, SLOT( slotViewLostFocus( Kate::View * ) ) );
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

  QPtrList<KTextEditor::Mark> m = m_view->getDoc()->marks();
  for (uint i=0; i < m.count(); i++)
    m_view->getDoc()->removeMark( m.at(i)->line, KTextEditor::MarkInterface::markType01 );

  // just to be sure ;)
  marksChanged ();
}

void KateBookmarks::slotViewGotFocus( Kate::View *v )
{
  if ( v == (Kate::View*)m_view )
    bookmarkMenuAboutToHide();
}

void KateBookmarks::slotViewLostFocus( Kate::View *v )
{
  if ( v == (Kate::View*)m_view )
    m_bookmarksMenu->clear();
}

void KateBookmarks::insertBookmarks( QPopupMenu& menu )
{
  uint line = m_view->cursorLine();
  const QRegExp re("&(?!&)");
  int idx( -1 );
  int old_menu_count = menu.count();
  KTextEditor::Mark *next = 0;
  KTextEditor::Mark *prev = 0;

  QPtrList<KTextEditor::Mark> m = m_view->getDoc()->marks();
  QMemArray<uint> sortArray( m.count() );
  QPtrListIterator<KTextEditor::Mark> it( m );

  if ( it.count() > 0 )
    menu.insertSeparator();

  for( int i = 0; *it; ++it, ++i )
  {
    if( (*it)->type & KTextEditor::MarkInterface::markType01 )
    {
      QString bText = KStringHandler::rEmSqueeze
                      ( m_view->getDoc()->textLine( (*it)->line ),
                        menu.fontMetrics(), 32 );
      bText.replace(re, "&&"); // kill undesired accellerators!
      bText.replace('\t', ' '); // kill tabs, as they are interpreted as shortcuts

      if ( m_sorting == Position )
      {
        sortArray[i] = (*it)->line;
        ssort( sortArray, i );
        idx = sortArray.find( (*it)->line ) + 3;
      }

      menu.insertItem(
          QString("%1 - \"%2\"").arg( (*it)->line+1 ).arg( bText ),
          m_view, SLOT(gotoLineNumber(int)), 0, (*it)->line, idx );

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

  idx = ++old_menu_count;
  if ( next )
  {
    m_goNext->setText( i18n("&Next: %1 - \"%2\"").arg( next->line + 1 )
        .arg( KStringHandler::rsqueeze( m_view->getDoc()->textLine( next->line ), 24 ) ) );
    m_goNext->plug( &menu, idx );
    idx++;
  }
  if ( prev )
  {
    m_goPrevious->setText( i18n("&Previous: %1 - \"%2\"").arg(prev->line + 1 )
        .arg( KStringHandler::rsqueeze( m_view->getDoc()->textLine( prev->line ), 24 ) ) );
    m_goPrevious->plug( &menu, idx );
    idx++;
  }
  if ( next || prev )
    menu.insertSeparator( idx );

}

void KateBookmarks::bookmarkMenuAboutToShow()
{

  QPtrList<KTextEditor::Mark> m = m_view->getDoc()->marks();

  m_bookmarksMenu->clear();
  m_bookmarkToggle->setChecked( m_view->getDoc()->mark( m_view->cursorLine() )
                                & KTextEditor::MarkInterface::markType01 );
  m_bookmarkToggle->plug( m_bookmarksMenu );
  m_bookmarkClear->plug( m_bookmarksMenu );


  insertBookmarks(*m_bookmarksMenu);
}

/*
   Make sure next/prev actions are plugged, and have a clean text
*/
void KateBookmarks::bookmarkMenuAboutToHide()
{
  m_bookmarkToggle->plug( m_bookmarksMenu );
  m_bookmarkClear->plug( m_bookmarksMenu );
  m_goNext->setText( i18n("Next Bookmark") );
  m_goNext->plug( m_bookmarksMenu );
  m_goPrevious->setText( i18n("Previous Bookmark") );
  m_goPrevious->plug( m_bookmarksMenu );
}

void KateBookmarks::goNext()
{
  QPtrList<KTextEditor::Mark> m = m_view->getDoc()->marks();
  if (m.isEmpty())
    return;

  uint line = m_view->cursorLine();
  int found = -1;

  for (uint z=0; z < m.count(); z++)
    if ( (m.at(z)->line > line) && ((found == -1) || (uint(found) > m.at(z)->line)) )
      found = m.at(z)->line;

  if (found != -1)
    m_view->gotoLineNumber ( found );
}

void KateBookmarks::goPrevious()
{
  QPtrList<KTextEditor::Mark> m = m_view->getDoc()->marks();
  if (m.isEmpty())
    return;

  uint line = m_view->cursorLine();
  int found = -1;

  for (uint z=0; z < m.count(); z++)
    if ((m.at(z)->line < line) && ((found == -1) || (uint(found) < m.at(z)->line)))
      found = m.at(z)->line;

  if (found != -1)
    m_view->gotoLineNumber ( found );
}

void KateBookmarks::marksChanged ()
{
  m_bookmarkClear->setEnabled( !m_view->getDoc()->marks().isEmpty() );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
