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
    KateBookmarks( Kate::View* parent );
    virtual ~KateBookmarks();
  
    void createActions( KActionCollection* );
  
  private slots:
    void toggleBookmark();
    void clearBookmarks();
    void bookmarkMenuAboutToShow();
    void gotoBookmark(int n);
  
  private:
    Kate::View*                  m_view;
    KAction*                     m_bookmarkToggle;
    KAction*                     m_bookmarkClear;
    KActionMenu*                 m_bookmarkMenu;
    QPtrList<KTextEditor::Mark>  m_marks;
};

#endif // _KateBookmarks_H_

// vim: noet ts=2
