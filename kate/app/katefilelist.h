/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001,2006 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_FILELIST_H__
#define __KATE_FILELIST_H__

#include <KSelectAction>
#include <KActionCollection>
#include <QListView>

class KateFileList: public QListView
{
    Q_OBJECT
  public:
    KateFileList(QWidget * parent, KActionCollection *actionCollection);
    virtual ~KateFileList();

  private:
    QAction* m_windowNext;
    QAction* m_windowPrev;
    KSelectAction* m_sortAction;
    enum SortType {SortOpening = 0, SortName = 1, SortUrl = 2, SortCustom = 3};
    int m_sortType;

  public Q_SLOTS:
    void slotNextDocument();
    void slotPrevDocument();
    void setSortType(int sortType);

};

#endif //__KATE_FILELIST_H__

// kate: space-indent on; indent-width 2; replace-tabs on;
