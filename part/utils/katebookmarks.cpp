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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katebookmarks.h"
#include "katebookmarks.moc"

#include "katedocument.h"
#include "kateview.h"

#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kguiitem.h>
#include <kicon.h>
#include <kmenu.h>
#include <kstringhandler.h>
#include <kxmlguiclient.h>
#include <kxmlguifactory.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>

#include <QtCore/QRegExp>
#include <QtCore/QEvent>
#include <QtCore/QVector>

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
  : QObject( view )
  , m_view( view )
  , m_sorting( sort )
{
  setObjectName( "kate bookmarks" );
  connect (view->doc(), SIGNAL( marksChanged( KTextEditor::Document* ) ), this, SLOT( marksChanged() ));
  _tries=0;
  m_bookmarksMenu = 0L;
}

KateBookmarks::~KateBookmarks()
{
}

void KateBookmarks::createActions( KActionCollection* ac )
{
    m_bookmarkToggle = new KToggleAction( i18n("Set &Bookmark"), this );
    ac->addAction( "bookmarks_toggle", m_bookmarkToggle );
    m_bookmarkToggle->setIcon( KIcon( "bookmark-new" ) );
    m_bookmarkToggle->setShortcut( Qt::CTRL+Qt::Key_B );
    m_bookmarkToggle->setWhatsThis(i18n("If a line has no bookmark then add one, otherwise remove it."));
    connect( m_bookmarkToggle, SIGNAL( triggered() ), this, SLOT(toggleBookmark()) );

    m_bookmarkClear = new KAction( i18n("Clear &All Bookmarks"), this );
    ac->addAction("bookmarks_clear", m_bookmarkClear);
    m_bookmarkClear->setWhatsThis(i18n("Remove all bookmarks of the current document."));
    connect( m_bookmarkClear, SIGNAL( triggered() ), this, SLOT(clearBookmarks()) );

    m_goNext = new KAction( i18n("Next Bookmark"), this);
    ac->addAction("bookmarks_next", m_goNext);
    m_goNext->setIcon( KIcon( "find-next" ) );
    m_goNext->setShortcut( Qt::ALT + Qt::Key_PageDown );
    m_goNext->setWhatsThis(i18n("Go to the next bookmark."));
    connect( m_goNext, SIGNAL( triggered() ), this, SLOT(goNext()) );

    m_goPrevious = new KAction( i18n("Previous Bookmark"), this);
    ac->addAction("bookmarks_previous", m_goPrevious);
    m_goPrevious->setIcon( KIcon( "find-previous" ) );
    m_goPrevious->setShortcut( Qt::ALT + Qt::Key_PageUp );
    m_goPrevious->setWhatsThis(i18n("Go to the previous bookmark."));
    connect( m_goPrevious, SIGNAL( triggered() ), this, SLOT(goPrevious()) );

    KActionMenu *actionMenu = new KActionMenu(i18n("&Bookmarks"), this);
    ac->addAction("bookmarks", actionMenu);
    m_bookmarksMenu = actionMenu->menu();

    connect( m_bookmarksMenu, SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuAboutToShow()));

    marksChanged ();

    // Always want the actions with shortcuts plugged into something so their shortcuts can work
    m_view->addAction(m_bookmarkToggle);
    m_view->addAction(m_bookmarkClear);
    m_view->addAction(m_goNext);
    m_view->addAction(m_goPrevious);
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

void KateBookmarks::insertBookmarks( QMenu& menu )
{
  int line = m_view->cursorPosition().line();
  const QRegExp re("&(?!&)");
  int idx( -1 );
  KTextEditor::Mark *next = 0;
  KTextEditor::Mark *prev = 0;

  const QHash<int, KTextEditor::Mark*> &m = m_view->doc()->marks();
  QVector<uint> sortArray( m.size() );

  if ( m.isEmpty() )
    return;

  int i = 0;
  QAction* firstNewAction = menu.addSeparator();
  for (QHash<int, KTextEditor::Mark*>::const_iterator it = m.constBegin(); it != m.constEnd(); ++it, ++i)
  {
    if( it.value()->type & KTextEditor::MarkInterface::markType01 )
    {
      QString bText = menu.fontMetrics().elidedText
                      ( m_view->doc()->line( it.value()->line ),
                        Qt::ElideRight,
                        menu.fontMetrics().maxWidth() * 32 );
      bText.replace(re, "&&"); // kill undesired accellerators!
      bText.replace('\t', ' '); // kill tabs, as they are interpreted as shortcuts

      QAction *before=0;
      if ( m_sorting == Position )
      {
        sortArray[i] = it.value()->line;
        ssort( sortArray, i );

        for (int i=0; i < sortArray.size(); ++i)
        {
          if ((int)sortArray[i] == it.value()->line)
          {
            idx = i + 3;
            if (idx>=menu.actions().size()) before=0;
            else before=menu.actions()[idx];
            break;
          }
        }
      }

      if (before) {
        QAction *a=new QAction(QString("%1 - \"%2\"").arg( it.value()->line+1 ).arg( bText ),&menu);
        menu.insertAction(before,a);
        connect(a,SIGNAL(activated()),this,SLOT(gotoLine()));
        a->setData(it.value()->line);
        if (!firstNewAction) firstNewAction = a;

      } else {
        QAction* a = menu.addAction(QString("%1 - \"%2\"").arg( it.value()->line+1 ).arg( bText ), this, SLOT(gotoLine()));
        a->setData(it.value()->line);
      }

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

  if ( next )
  {
    m_goNext->setText( i18n("&Next: %1 - \"%2\"",  next->line + 1 ,
          KStringHandler::rsqueeze( m_view->doc()->line( next->line ), 24 ) ) );
    menu.insertAction(firstNewAction, m_goNext);
    firstNewAction = m_goNext;
  }
  if ( prev )
  {
    m_goPrevious->setText( i18n("&Previous: %1 - \"%2\"", prev->line + 1 ,
          KStringHandler::rsqueeze( m_view->doc()->line( prev->line ), 24 ) ) );
    menu.insertAction(firstNewAction, m_goPrevious);
    firstNewAction = m_goPrevious;
  }

  if ( next || prev )
    menu.insertSeparator(firstNewAction);
}

void KateBookmarks::gotoLine()
{
  if (!sender()) return;
  gotoLine(((QAction*)(sender()))->data().toInt());
}

void KateBookmarks::gotoLine (int line)
{
  m_view->setCursorPosition(KTextEditor::Cursor(line, 0));
}

void KateBookmarks::bookmarkMenuAboutToShow()
{
  m_bookmarksMenu->clear();
  m_bookmarkToggle->setChecked( m_view->doc()->mark( m_view->cursorPosition().line() )
                                & KTextEditor::MarkInterface::markType01 );
  m_bookmarksMenu->addAction(m_bookmarkToggle);
  m_bookmarksMenu->addAction(m_bookmarkClear);

  m_goNext->setText( i18n("Next Bookmark") );
  m_goPrevious->setText( i18n("Previous Bookmark") );

  insertBookmarks(*m_bookmarksMenu);
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
