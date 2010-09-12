/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

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

#ifndef KATE_FILETREE_H
#define KATE_FILETREE_H

#include <QTreeView>
#include <KUrl>
#include <KIcon>

namespace KTextEditor {
  class Document;
}

class QActionGroup;

class KateFileTree: public QTreeView
{
    Q_OBJECT

  public:
    
    KateFileTree(QWidget * parent);
    virtual ~KateFileTree();

  private:
    QAction *m_filelistCloseDocument;

    QAction *m_treeModeAction;
    QAction *m_listModeAction;

    QAction *m_sortByFile;
    QAction *m_sortByPath;
    QAction *m_sortByOpeningOrder;
    
    QPersistentModelIndex m_previouslySelected;
    QPersistentModelIndex m_indexContextMenu;

  public Q_SLOTS:
    void slotDocumentClose();
    void currentChanged( const QModelIndex &current, const QModelIndex &previous );
    
  protected:
    virtual void contextMenuEvent ( QContextMenuEvent * event );

  Q_SIGNALS:
    void closeDocument(KTextEditor::Document*);
    void activateDocument(KTextEditor::Document*);
    
    void openDocument(KUrl);

    void viewModeChanged(bool treeMode);
    void sortRoleChanged(int);
    
  private Q_SLOTS:
    void mouseClicked(const QModelIndex &index);
    void mousePressed(const QModelIndex &index);

    void slotTreeMode();
    void slotListMode();

    void slotSortName();
    void slotSortPath();
    void slotSortOpeningOrder();
    
  private:
    QAction *setupOption(QActionGroup *group, const KIcon &, const QString &, const QString &, const char *slot, bool checked=false);
};

#endif // KATE_FILETREE_H

// kate: space-indent on; indent-width 2; replace-tabs on;
