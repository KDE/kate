 /***************************************************************************
                          KateBookmarks.cpp  -  description
                             -------------------
    begin                : Mon Apr 1 2002
    copyright            : (C) 2002 by John Firebaugh
    email                : jfirebaugh@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
                     
 // $Id$
 
#include "katebookmarks.h"
#include "katebookmarks.moc"

#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kdebug.h>

#include "../interfaces/document.h"
#include "../interfaces/view.h"

KateBookmarks::KateBookmarks( Kate::View* view )
  : QObject( view, "kate bookmarks" )
  , m_view( view )
{
}

KateBookmarks::~KateBookmarks()
{
}

void KateBookmarks::createActions( KActionCollection* ac )
{
  m_bookmarkMenu = new KActionMenu(
    i18n("&Bookmarks"), ac, "bookmarks" );
  
  // setup bookmark menu
  m_bookmarkToggle = new KAction(
    i18n("Toggle &Bookmark"), CTRL+Key_B,
    this, SLOT(toggleBookmark()),
    ac, "bookmarks_toggle" );
  m_bookmarkClear = new KAction(
    i18n("Clear Bookmarks"), 0,
    this, SLOT(clearBookmarks()),
    ac, "bookmarks_clear");
  
  // connect bookmarks menu aboutToshow
  connect( m_bookmarkMenu->popupMenu(), SIGNAL(aboutToShow()),
           this, SLOT(bookmarkMenuAboutToShow()));
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
  
  m_marks = m_view->getDoc()->marks();
  QPtrListIterator<KTextEditor::Mark> it( m_marks );
  for( int i = 0; *it; ++it, ++i ) {
    if( (*it)->type & KTextEditor::MarkInterface::markType01 ) {
      QString bText = m_view->getDoc()->textLine( (*it)->line );
      bText.truncate(32);
      bText.append ("...");
      m_bookmarkMenu->popupMenu()->insertItem(
        QString("%1 - \"%2\"").arg( (*it)->line).arg( bText ),
        this, SLOT (gotoBookmark(int)), 0, i );
    }
  }
}

void KateBookmarks::gotoBookmark( int n )
{
  m_view->setCursorPosition( m_marks.at(n)->line, 0 );
}

// vim: noet ts=2
