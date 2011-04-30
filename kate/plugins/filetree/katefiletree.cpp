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

//BEGIN Includes
#include "katefiletree.h"
#include "katefiletree.moc"
#include "katefiletreemodel.h"
#include "katefiletreeproxymodel.h"
#include "katefiletreedebug.h"

#include <ktexteditor/document.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <KLocale>

#include <QtGui/QMenu>
#include <QtGui/QContextMenuEvent>
#include <kicon.h>
#include <kdebug.h>
//END Includes


//BEGIN KateFileTree

KateFileTree::KateFileTree(QWidget *parent): QTreeView(parent)
{
  setAcceptDrops(false);
  setIndentation(12);
  setAllColumnsShowFocus(true);

  setTextElideMode(Qt::ElideLeft);
  
  connect( this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(mouseClicked(const QModelIndex &)));

  m_filelistCloseDocument = new QAction( KIcon("window-close"), i18n( "Close" ), this );
  connect( m_filelistCloseDocument, SIGNAL( triggered() ), this, SLOT( slotDocumentClose() ) );
  m_filelistCloseDocument->setWhatsThis(i18n("Close the current document."));

  QActionGroup *modeGroup = new QActionGroup(this);

  m_treeModeAction = setupOption(modeGroup, KIcon("view-list-tree"), i18n("Tree Mode"),
                                 i18n("Set view style to Tree Mode"),
                                 SLOT( slotTreeMode() ), true);

  m_listModeAction = setupOption(modeGroup, KIcon("view-list-text"), i18n("List Mode"),
                                 i18n("Set view style to List Mode"),
                                 SLOT( slotListMode() ), false);

  QActionGroup *sortGroup = new QActionGroup(this);

  m_sortByFile = setupOption(sortGroup, KIcon(), i18n("Document Name"),
                             i18n("Sort by Document Name"),
                             SLOT( slotSortName() ), true);

  
  m_sortByPath = setupOption(sortGroup, KIcon(), i18n("Document Path"),
                             i18n("Sort by Document Path"),
                             SLOT( slotSortPath() ), false);
  
  m_sortByOpeningOrder =  setupOption(sortGroup, KIcon(), i18n("Opening Order"),
                             i18n("Sort by Opening Order"),
                             SLOT( slotSortOpeningOrder() ), false);
  
  QPalette p = palette();
  p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
  p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
  setPalette(p);
}

KateFileTree::~KateFileTree()
{}

QAction *KateFileTree::setupOption(
  QActionGroup *group,
  const KIcon &icon,
  const QString &label,
  const QString &whatsThis,
  const char *slot,
  bool checked
)
{
  QAction *new_action = new QAction( icon, label, this );
  new_action->setWhatsThis(whatsThis);
  new_action->setActionGroup(group);
  new_action->setCheckable(true);
  new_action->setChecked(checked);
  connect( new_action, SIGNAL( triggered() ), this, slot );
  return new_action;
}

void KateFileTree::slotListMode()
{
  emit viewModeChanged(true);
}

void KateFileTree::slotTreeMode()
{
  emit viewModeChanged(false);
}

void KateFileTree::slotSortName()
{
  emit sortRoleChanged(Qt::DisplayRole);
}

void KateFileTree::slotSortPath()
{
  emit sortRoleChanged(KateFileTreeModel::PathRole);
}

void KateFileTree::slotSortOpeningOrder()
{
  emit sortRoleChanged(KateFileTreeModel::OpeningOrderRole);
}

void KateFileTree::slotCurrentChanged ( const QModelIndex &current, const QModelIndex &previous )
{
  kDebug(debugArea()) << "current:" << current << "previous:" << previous;

  if(!current.isValid())
    return;
  
  KTextEditor::Document *doc = model()->data(current, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if(doc) {
    kDebug(debugArea()) << "got doc, setting prev:" << current;
    m_previouslySelected = current;
  }
}

void KateFileTree::mouseClicked ( const QModelIndex &index )
{
  kDebug(debugArea()) << "got index" << index;

  KTextEditor::Document *doc = model()->data(index, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if(doc) {
    kDebug(debugArea()) << "got doc" << index << "setting prev:" << QModelIndex();
    emit activateDocument(doc);
    //m_previouslySelected = QModelIndex();
  }
  else {
    kDebug(debugArea()) << "selecting previous item" << m_previouslySelected;

    selectionModel()->setCurrentIndex(m_previouslySelected,QItemSelectionModel::ClearAndSelect);
  }
  
}

void KateFileTree::contextMenuEvent ( QContextMenuEvent * event ) {
  m_indexContextMenu=selectionModel()->currentIndex();

  selectionModel()->setCurrentIndex(m_indexContextMenu, QItemSelectionModel::ClearAndSelect);

  KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel*>(model());
  KateFileTreeModel *ftm = static_cast<KateFileTreeModel*>(ftpm->sourceModel());

  bool listMode = ftm->listMode();
  m_treeModeAction->setChecked(!listMode);
  m_listModeAction->setChecked(listMode);

  int sortRole = ftpm->sortRole();
  m_sortByFile->setChecked(sortRole == Qt::DisplayRole);
  m_sortByPath->setChecked(sortRole == KateFileTreeModel::PathRole);
  m_sortByOpeningOrder->setChecked(sortRole == KateFileTreeModel::OpeningOrderRole);
  
  QMenu menu;
  menu.addAction(m_filelistCloseDocument);
  menu.addSeparator();
  QMenu *view_menu = menu.addMenu(i18n("View Mode"));
  view_menu->addAction(m_treeModeAction);
  view_menu->addAction(m_listModeAction);

  QMenu *sort_menu = menu.addMenu(i18n("Sort By"));
  sort_menu->addAction(m_sortByFile);
  sort_menu->addAction(m_sortByPath);
  sort_menu->addAction(m_sortByOpeningOrder);
  
  menu.exec(viewport()->mapToGlobal(event->pos()));

  if (m_previouslySelected.isValid()) {
    selectionModel()->setCurrentIndex(m_previouslySelected,QItemSelectionModel::ClearAndSelect);
  }

  event->accept();
}

Q_DECLARE_METATYPE(QList<KTextEditor::Document*>)

void KateFileTree::slotDocumentClose() {
  m_previouslySelected = QModelIndex();
  QVariant v = m_indexContextMenu.data(KateFileTreeModel::DocumentTreeRole);
  if (!v.isValid()) return;
  QList<KTextEditor::Document*> closingDocuments = v.value<QList<KTextEditor::Document*> >();
  Kate::application()->documentManager()->closeDocumentList(closingDocuments);
}

void KateFileTree::slotDocumentPrev()
{
  kDebug(debugArea()) << "BEGIN";
  KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel*>(model());
  
  QModelIndex current_index = currentIndex();
  QModelIndex prev;
  
  // scan up the tree skipping any dir nodes
  
  //kDebug(debugArea()) << "cur" << ftpm->data(current_index, Qt::DisplayRole);
  while(current_index.isValid()) {
    if(current_index.row() > 0) {
      current_index = ftpm->sibling(current_index.row()-1, current_index.column(), current_index);
      //kDebug(debugArea()) << "get prev" << ftpm->data(current_index, Qt::DisplayRole);
      if(!current_index.isValid()) {
        //kDebug(debugArea()) << "somehow getting prev index from sibling didn't work :(";
        break;
      }
      
      if(ftpm->isDir(current_index)) {
        // try and select the last child in this parent
        //kDebug(debugArea()) << "is a dir";
        int children = ftpm->rowCount(current_index);
        current_index = ftpm->index(children-1, 0, current_index);
        //kDebug(debugArea()) << "child" << ftpm->data(current_index, Qt::DisplayRole);
        if(ftpm->isDir(current_index)) {
          // since we're a dir, keep going
          //kDebug(debugArea()) << "child is a dir";
          while(ftpm->isDir(current_index)) {
            children = ftpm->rowCount(current_index);
            current_index = ftpm->index(children-1, 0, current_index);
          }
          
          if(!ftpm->isDir(current_index)) {
            prev = current_index;
            break;
          }
          
          continue;
        } else {
          // we're the previous file, set prev
          //kDebug(debugArea()) << "got doc 1";
          prev = current_index;
          break;
        }
      } else { // found document item
        //kDebug(debugArea()) << "got doc 2";
        prev = current_index;
        break;
      }
    }
    else {
      //kDebug(debugArea()) << "get parent";
      // just select the parent, the logic above will handle the rest
      current_index = ftpm->parent(current_index);
      //kDebug(debugArea()) << "got parent" << ftpm->data(current_index, Qt::DisplayRole);
      if(!current_index.isValid()) {
        // past the root node here, try and wrap arround
        //kDebug(debugArea()) << "parent invalid";
        
        int children = ftpm->rowCount(current_index);
        QModelIndex last_index = ftpm->index(children-1, 0, current_index);
        //kDebug(debugArea()) << "last" << ftpm->data(last_index, Qt::DisplayRole);
        if(!last_index.isValid())
          break;
        
        if(ftpm->isDir(last_index)) {
          // last node is a dir, select last child row
          //kDebug(debugArea()) << "last root is a dir, select child";
          int last_children = ftpm->rowCount(last_index);
          prev = ftpm->index(last_children-1, 0, last_index);
          //kDebug(debugArea()) << "last child" << ftpm->data(current_index, Qt::DisplayRole);
          // bug here?
          break;
        }
        else {
          // got last file node
          //kDebug(debugArea()) << "got doc";
          prev = last_index;
          break;
        }
      }
    }
  }
  
  if(prev.isValid()) {
    //kDebug(debugArea()) << "got prev node:" << prev;
    //kDebug(debugArea()) << "doc:" << ftpm->data(prev, Qt::DisplayRole).value<QString>();

    KTextEditor::Document *doc = model()->data(prev, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    emit activateDocument(doc);
  }
  else {
    kDebug(debugArea()) << "didn't get prev node :(";
  }
  
  kDebug(debugArea()) << "END";
}

/*
plan:

default: select next sibling
if cur is a dir, select it

*/

void KateFileTree::slotDocumentNext()
{
  kDebug(debugArea()) << "BEGIN";
  
  KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel*>(model());
  
  QModelIndex current_index = currentIndex();
  int parent_row_count = ftpm->rowCount( ftpm->parent(current_index) );
  QModelIndex next;
  
  // scan down the tree skipping any dir nodes
  while(current_index.isValid()) {
    if(current_index.row() < parent_row_count-1) {
      current_index = ftpm->sibling(current_index.row()+1, current_index.column(), current_index);
      if(!current_index.isValid()) {
        break;
      }
      
      if(ftpm->isDir(current_index)) {
        // we have a dir node
        while(ftpm->isDir(current_index)) {
          current_index = ftpm->index(0, 0, current_index);
        }
        
        parent_row_count = ftpm->rowCount( ftpm->parent(current_index) );
        
        if(!ftpm->isDir(current_index)) {
          next = current_index;
          break;
        }
      } else { // found document item
        next = current_index;
        break;
      }
    }
    else {
      // select the parent's next sibling
      QModelIndex parent_index = ftpm->parent(current_index);
      int grandparent_row_count = ftpm->rowCount( ftpm->parent(parent_index) );
      
      current_index = parent_index;
      parent_row_count = grandparent_row_count;
      
      // at least if we're not past the last node
      if(!current_index.isValid()) {
        // past the root node here, try and wrap arround
        QModelIndex last_index = ftpm->index(0, 0, QModelIndex());
        if(!last_index.isValid()) {
          break;
        }
        
        if(ftpm->isDir(last_index)) {
          // last node is a dir, select first child row
          while(ftpm->isDir(last_index)) {
            if(ftpm->rowCount(last_index)) {
              // has children, select first
              last_index = ftpm->index(0, 0, last_index);
            }
          }
          
          next = last_index;
          break;
        }
        else {
          // got first file node
          next = last_index;
          break;
        }
      }
    }
  }
  
  if(next.isValid()) {
    //kDebug(debugArea()) << "got next node:" << next;
    //kDebug(debugArea()) << "doc:" << ftpm->data(next, Qt::DisplayRole).value<QString>();

    KTextEditor::Document *doc = model()->data(next, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    emit activateDocument(doc);
  }
  else {
    kDebug(debugArea()) << "didn't get next node :(";
  }
  
  kDebug(debugArea()) << "END";
}
//END KateFileTree

// kate: space-indent on; indent-width 2; replace-tabs on;
