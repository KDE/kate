 /***************************************************************************
                          KateBookmarks.h  -  description
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
