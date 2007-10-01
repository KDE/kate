/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
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

//BEGIN Includes
#include "katefilelist.h"
#include "katefilelist.moc"
#include "kateviewdocumentproxymodel.h"

#include <kstandardaction.h>
#include <KLocale>
//END Includes


//BEGIN KateFileList

KateFileList::KateFileList(QWidget *parent, KActionCollection *actionCollection): QListView(parent)
{
  m_windowNext = actionCollection->addAction(KStandardAction::Back, this, SLOT(slotPrevDocument()));
  m_windowPrev = actionCollection->addAction(KStandardAction::Forward, this, SLOT(slotNextDocument()));

  m_sortAction = new KSelectAction( i18n("Sort &By"), this );
  actionCollection->addAction( "filelist_sortby", m_sortAction );

  QStringList l;
  l << i18n("Opening Order") << i18n("Document Name") << i18n("URL") << i18n("Custom");
  m_sortAction->setItems( l );

  connect( m_sortAction, SIGNAL(triggered(int)), this, SLOT(setSortType(int)) );

  m_sortType = SortOpening;
  QPalette p = palette();
  p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
  p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
  setPalette(p);
}

KateFileList::~KateFileList()
{}

void KateFileList::setSortType(int sortType)
{
  m_sortType = sortType;
}

void KateFileList::slotNextDocument()
{
  QModelIndex idx = selectionModel()->currentIndex();
  if (idx.isValid())
  {
    QModelIndex newIdx = model()->index(idx.row() + 1, idx.column(), idx.parent());
    if (!newIdx.isValid())
      newIdx = model()->index(0, idx.column(), idx.parent());
    if (newIdx.isValid())
    {
//      selectionModel()->select(newIdx,QItemSelectionModel::SelectCurrent);
//      selectionModel()->setCurrentIndex(newIdx,QItemSelectionModel::SelectCurrent);
      emit activated(newIdx);
    }
  }
}

void KateFileList::slotPrevDocument()
{
  QModelIndex idx = selectionModel()->currentIndex();
  if (idx.isValid())
  {
    int row = idx.row() - 1;
    if (row < 0) row = model()->rowCount(idx.parent()) - 1;
    QModelIndex newIdx = model()->index(row, idx.column(), idx.parent());
    if (newIdx.isValid())
    {
//      selectionModel()->select(newIdx,QItemSelectionModel::SelectCurrent);
//      selectionModel()->setCurrentIndex(newIdx,QItemSelectionModel::SelectCurrent);
      emit activated(newIdx);
    }
  }
}
//END KateFileList

// kate: space-indent on; indent-width 2; replace-tabs on;
