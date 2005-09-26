/* This file is part of the KDE libraries
   Copyright (C) 2002, 2003 Anders Lund <anders.lund@lund.tdcadsl.dk>
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

#ifndef __KATE_BOOKMARKS_H__
#define __KATE_BOOKMARKS_H__

#include <QObject>

class KateView;

namespace KTextEditor { class Mark; class View; }
namespace KDocument { class View; }

class KAction;
class KToggleAction;
class KActionCollection;
class QMenu;

class KateBookmarks : public QObject
{
  Q_OBJECT

  public:
    enum Sorting { Position, Creation };
    KateBookmarks( KateView* parent, Sorting sort=Position );
    virtual ~KateBookmarks();

    void createActions( KActionCollection* );

    KateBookmarks::Sorting sorting() { return m_sorting; };
    void setSorting( Sorting s ) { m_sorting = s; };

  protected:
    void insertBookmarks( QMenu& menu);

  private slots:
    void toggleBookmark();
    void clearBookmarks();
  
    void gotoLine();
    void gotoLine (int line);
  
    void slotViewGotFocus( KDocument::View * );
    void slotViewLostFocus( KDocument::View * );

    void bookmarkMenuAboutToShow();
    void bookmarkMenuAboutToHide();

    void goNext();
    void goPrevious();

    void marksChanged ();

  private:
    KateView*                    m_view;
    KToggleAction*               m_bookmarkToggle;
    KAction*                     m_bookmarkClear;
    KAction*                     m_goNext;
    KAction*                     m_goPrevious;

    Sorting                      m_sorting;
    QMenu*                       m_bookmarksMenu;

    uint _tries;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
// vim: noet ts=2
