/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_BOOKMARKS_H__
#define __KATE_BOOKMARKS_H__

#include <QObject>

class KateView;

class KToggleAction;
class KActionCollection;
class QMenu;
class QAction;

class KateBookmarks : public QObject
{
  Q_OBJECT

  public:
    enum Sorting { Position, Creation };
    explicit KateBookmarks( KateView* parent, Sorting sort=Position );
    virtual ~KateBookmarks();

    void createActions( KActionCollection* );

    KateBookmarks::Sorting sorting() { return m_sorting; }
    void setSorting( Sorting s ) { m_sorting = s; }

  protected:
    void insertBookmarks( QMenu& menu);

  private Q_SLOTS:
    void toggleBookmark();
    void clearBookmarks();
  
    void gotoLine();
    void gotoLine (int line);

    void bookmarkMenuAboutToShow();

    void goNext();
    void goPrevious();

    void marksChanged ();

  private:
    KateView*                    m_view;
    KToggleAction*               m_bookmarkToggle;
    QAction*                     m_bookmarkClear;
    QAction*                     m_goNext;
    QAction*                     m_goPrevious;

    Sorting                      m_sorting;
    QMenu*                       m_bookmarksMenu;

    uint _tries;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
// vim: noet ts=2
