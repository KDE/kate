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

#ifndef _KateBookmarks_H_
#define _KateBookmarks_H_

#include <qobject.h>
#include <qptrlist.h>

class KAction;
class KActionMenu;
class KActionCollection;

namespace Kate { class View; }
namespace KTextEditor { class Mark; }

class KateBookmarks : public QObject
{
  Q_OBJECT
  
  public:
    enum Sorting { Position, Creation };
    KateBookmarks( Kate::View* parent, Sorting sort=Position );
    virtual ~KateBookmarks();
  
    void createActions( KActionCollection* );
    
    KateBookmarks::Sorting sorting() { return m_sorting; };
    void setSorting( Sorting s ) { m_sorting = s; }; 
  
  private slots:
    void toggleBookmark();
    void clearBookmarks();
    void bookmarkMenuAboutToShow();
    void bookmarkMenuAboutToHide();
    void gotoBookmark(int n);
    void goNext();
    void goPrevious();
  
  private:
    Kate::View*                  m_view;
    KAction*                     m_bookmarkToggle;
    KAction*                     m_bookmarkClear;
    KAction*                     m_goNext;
    KAction*                     m_goPrevious;
    KActionMenu*                 m_bookmarkMenu;
    QPtrList<KTextEditor::Mark>  m_marks;
    Sorting                      m_sorting;
};

#endif // _KateBookmarks_H_

// vim: noet ts=2
