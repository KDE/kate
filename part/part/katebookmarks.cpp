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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
#include <qevent.h>
#include <QVector>

namespace KTextEditor{ class Document; }

/**
   Utility: selection sort
   sort a QMemArray<uint> in ascending order.
   max it the largest (zerobased) index to sort.
   To sort the entire array: ssort( *array, array.size() -1 );
   This is only efficient if ran only once.
*/
static void ssort( QVector<uint> &a, int max )
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
  connect (view->doc(), SIGNAL( marksChanged( KTextEditor::Document* ) ), this, SLOT( marksChanged() ));
  _tries=0;
  m_bookmarksMenu = 0L;
}

KateBookmarks::~KateBookmarks()
{
}

void KateBookmarks::createActions( KActionCollection* ac )
{
  m_bookmarkToggle = new KToggleAction(
    i18n("Set &Bookmark"), "bookmark", Qt::CTRL+Qt::Key_B,
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
    i18n("Next Bookmark"), "next", Qt::ALT + Qt::Key_PageDown,
    this, SLOT(goNext()),
    ac, "bookmarks_next");
  m_goNext->setWhatsThis(i18n("Go to the next bookmark."));

  m_goPrevious = new KAction(
    i18n("Previous Bookmark"), "previous", Qt::ALT + Qt::Key_PageUp,
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

  connect( m_view, SIGNAL( focusIn( KDocument::View * ) ), this, SLOT( slotViewGotFocus( KDocument::View * ) ) );
  connect( m_view, SIGNAL( focusOut( KDocument::View * ) ), this, SLOT( slotViewLostFocus( KDocument::View * ) ) );
}

void KateBookmarks::toggleBookmark ()
{
  uint mark = m_view->doc()->mark( m_view->cursorPosition().line() );
  if( mark & KTextEditor::MarkInterface::markType01 )
    m_view->doc()->removeMark( m_view->cursorPosition().line(),
        KTextEditor::MarkInterface::markType01 );
  else
    m_view->doc()->addMark( m_view->cursorPosition().line(),
        KTextEditor::MarkInterface::markType01 );
}

void KateBookmarks::clearBookmarks ()
{
  QHash<int, KTextEditor::Mark*> m = m_view->doc()->marks();
  for (QHash<int, KTextEditor::Mark*>::const_iterator i = m.constBegin(); i != m.constEnd(); ++i)
    m_view->doc()->removeMark( i.value()->line, KTextEditor::MarkInterface::markType01 );

  // just to be sure ;)
  // dominik: the following line can be deleted afaics, as Document::removeMark emits this signal.
  marksChanged ();
}

void KateBookmarks::slotViewGotFocus( KDocument::View *v )
{
  if ( v == m_view )
    bookmarkMenuAboutToHide();
}

void KateBookmarks::slotViewLostFocus( KDocument::View *v )
{
  if ( v == m_view )
    m_bookmarksMenu->clear();
}

void KateBookmarks::insertBookmarks( Q3PopupMenu& menu )
{
  int line = m_view->cursorPosition().line();
  const QRegExp re("&(?!&)");
  int idx( -1 );
  int old_menu_count = menu.count();
  KTextEditor::Mark *next = 0;
  KTextEditor::Mark *prev = 0;

  const QHash<int, KTextEditor::Mark*> &m = m_view->doc()->marks();
  QVector<uint> sortArray( m.size() );

  if ( !m.isEmpty() )
    menu.insertSeparator();
  
  int i = 0;
  for (QHash<int, KTextEditor::Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it, ++i)
  {
    if( it.value()->type & KTextEditor::MarkInterface::markType01 )
    {
      QString bText = KStringHandler::rEmSqueeze
                      ( m_view->doc()->line( it.value()->line ),
                        menu.fontMetrics(), 32 );
      bText.replace(re, "&&"); // kill undesired accellerators!
      bText.replace('\t', ' '); // kill tabs, as they are interpreted as shortcuts

      if ( m_sorting == Position )
      {
        sortArray[i] = it.value()->line;
        ssort( sortArray, i );

        for (int i=0; i < sortArray.size(); ++i)
        {
          if (sortArray[i] == it.value()->line)
          {
            idx = i + 3;
            break;
          }
        }
      }

      menu.insertItem(
          QString("%1 - \"%2\"").arg( it.value()->line+1 ).arg( bText ),
          this, SLOT(gotoLine(int)), 0, it.value()->line, idx );

      if ( it.value()->line < line )
      {
        if ( ! prev || prev->line < it.value()->line )
          prev = (*it);
      }

      else if ( it.value()->line > line )
      {
        if ( ! next || next->line > it.value()->line )
          next = it.value();
      }
    }
  }

  idx = ++old_menu_count;
  if ( next )
  {
    m_goNext->setText( i18n("&Next: %1 - \"%2\"").arg( next->line + 1 )
        .arg( KStringHandler::rsqueeze( m_view->doc()->line( next->line ), 24 ) ) );
    m_goNext->plug( &menu, idx );
    idx++;
  }
  if ( prev )
  {
    m_goPrevious->setText( i18n("&Previous: %1 - \"%2\"").arg(prev->line + 1 )
        .arg( KStringHandler::rsqueeze( m_view->doc()->line( prev->line ), 24 ) ) );
    m_goPrevious->plug( &menu, idx );
    idx++;
  }
  if ( next || prev )
    menu.insertSeparator( idx );

}

void KateBookmarks::gotoLine (int line)
{
  m_view->setCursorPositionReal (line, 0);
}

void KateBookmarks::bookmarkMenuAboutToShow()
{
  m_bookmarksMenu->clear();
  m_bookmarkToggle->setChecked( m_view->doc()->mark( m_view->cursorPosition().line() )
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
  const QHash<int, KTextEditor::Mark*> &m = m_view->doc()->marks();
  if (m.isEmpty())
    return;

  int line = m_view->cursorPosition().line();
  int found = -1;

  for (QHash<int, KTextEditor::Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it)
  {
    if ( (it.value()->line > line) && ((found == -1) || (found > it.value()->line)) )
      found = it.value()->line;
  }

  if (found != -1)
    gotoLine ( found );
}

void KateBookmarks::goPrevious()
{
  const QHash<int, KTextEditor::Mark*> &m = m_view->doc()->marks();
  if (m.isEmpty())
    return;

  int line = m_view->cursorPosition().line();
  int found = -1;

  for (QHash<int, KTextEditor::Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it)
  {
    if ((it.value()->line < line) && ((found == -1) || (found < it.value()->line)))
      found = it.value()->line;
  }

  if (found != -1)
    gotoLine ( found );
}

void KateBookmarks::marksChanged ()
{
  m_bookmarkClear->setEnabled( !m_view->doc()->marks().isEmpty() );
}

// kate: space-indent on; indent-width 2; replace-tabs on;
