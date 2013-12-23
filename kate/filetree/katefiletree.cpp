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

#include <KMimeTypeTrader>
#include <KOpenWithDialog>
#include <KRun>
#include <KMessageBox>
#include <KLocalizedString>

#include <QMimeDatabase>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QApplication>
#include <QMenu>
//END Includes


//BEGIN KateFileTree

KateFileTree::KateFileTree(QWidget *parent): QTreeView(parent)
{
  setAcceptDrops(false);
  setIndentation(12);
  setAllColumnsShowFocus(true);
  
  connect( this, SIGNAL(activated(QModelIndex)), this, SLOT(mouseClicked(QModelIndex)));

  m_filelistCloseDocument = new QAction( QIcon::fromTheme("window-close"), i18n( "Close" ), this );
  connect( m_filelistCloseDocument, SIGNAL(triggered()), this, SLOT(slotDocumentClose()) );
  m_filelistCloseDocument->setWhatsThis(i18n("Close the current document."));

  m_filelistCopyFilename = new QAction( QIcon::fromTheme("edit-copy"), i18n( "Copy Filename" ), this );
  connect( m_filelistCopyFilename, SIGNAL(triggered()), this, SLOT(slotCopyFilename()) );
  m_filelistCopyFilename->setWhatsThis(i18n("Copy the filename of the file."));

  QActionGroup *modeGroup = new QActionGroup(this);

  m_treeModeAction = setupOption(modeGroup, QIcon::fromTheme("view-list-tree"), i18n("Tree Mode"),
                                 i18n("Set view style to Tree Mode"),
                                 SLOT(slotTreeMode()), true);

  m_listModeAction = setupOption(modeGroup, QIcon::fromTheme("view-list-text"), i18n("List Mode"),
                                 i18n("Set view style to List Mode"),
                                 SLOT(slotListMode()), false);

  QActionGroup *sortGroup = new QActionGroup(this);

  m_sortByFile = setupOption(sortGroup, QIcon(), i18n("Document Name"),
                             i18n("Sort by Document Name"),
                             SLOT(slotSortName()), true);

  
  m_sortByPath = setupOption(sortGroup, QIcon(), i18n("Document Path"),
                             i18n("Sort by Document Path"),
                             SLOT(slotSortPath()), false);
  
  m_sortByOpeningOrder =  setupOption(sortGroup, QIcon(), i18n("Opening Order"),
                             i18n("Sort by Opening Order"),
                             SLOT(slotSortOpeningOrder()), false);
  
  QPalette p = palette();
  p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
  p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
  setPalette(p);
}

KateFileTree::~KateFileTree()
{}

QAction *KateFileTree::setupOption(
  QActionGroup *group,
  const QIcon &icon,
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
  connect( new_action, SIGNAL(triggered()), this, slot );
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
  qCDebug(FILETREE) << "current:" << current << "previous:" << previous;

  if(!current.isValid())
    return;
  
  KTextEditor::Document *doc = model()->data(current, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if(doc) {
    qCDebug(FILETREE) << "got doc, setting prev:" << current;
    m_previouslySelected = current;
  }
}

void KateFileTree::mouseClicked ( const QModelIndex &index )
{
  qCDebug(FILETREE) << "got index" << index;

  KTextEditor::Document *doc = model()->data(index, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if(doc) {
    qCDebug(FILETREE) << "got doc" << index << "setting prev:" << QModelIndex();
    emit activateDocument(doc);
  }
  else {
    setExpanded(index, !isExpanded(index));
  }
  
}

void KateFileTree::contextMenuEvent ( QContextMenuEvent * event )
{
  m_indexContextMenu = selectionModel()->currentIndex();

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

  const bool isFile = (0 != m_indexContextMenu.data(KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>());

  QMenu menu;
  menu.addAction(m_filelistCloseDocument);
  if (isFile) {
    menu.addAction(m_filelistCopyFilename);
    QMenu *openWithMenu=menu.addMenu(i18n("Open With"));
    connect(openWithMenu, SIGNAL(aboutToShow()), this, SLOT(slotFixOpenWithMenu()));
    connect(openWithMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotOpenWithMenuAction(QAction*)));
  }
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


void KateFileTree::slotFixOpenWithMenu()
{
  QMenu *menu = (QMenu*)sender();
  menu->clear();
  
   KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if (!doc) return;

  // get a list of appropriate services.
  QMimeDatabase db;
  QMimeType mime = db.mimeTypeForName( doc->mimeType() );

  QAction *a = 0;
  KService::List offers = KMimeTypeTrader::self()->query(mime.name(), "Application");
  // for each one, insert a menu item...
  for(KService::List::Iterator it = offers.begin(); it != offers.end(); ++it)
  {
    KService::Ptr service = *it;
    if (service->name() == "Kate") continue;
    a = menu->addAction(QIcon::fromTheme(service->icon()), service->name());
    a->setData(service->entryPath());
  }
  // append "Other..." to call the KDE "open with" dialog.
  a = menu->addAction(i18n("&Other..."));
  a->setData(QString());
}

void KateFileTree::slotOpenWithMenuAction(QAction* a)
{
  QList<QUrl> list;
  
  KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if (!doc) return;

  
  list.append( doc->url() );

  const QString openWith = a->data().toString();
  if (openWith.isEmpty())
  {
    // display "open with" dialog
    KOpenWithDialog dlg(list);
    if (dlg.exec())
      KRun::run(*dlg.service(), list, this);
    return;
  }

  KService::Ptr app = KService::serviceByDesktopPath(openWith);
  if (app)
  {
    KRun::run(*app, list, this);
  }
  else
  {
    KMessageBox::error(this, i18n("Application '%1' not found.", openWith), i18n("Application not found"));
  }
}


#include "metatype_qlist_ktexteditor_document_pointer.h"

void KateFileTree::slotDocumentClose()
{
  m_previouslySelected = QModelIndex();
  QVariant v = m_indexContextMenu.data(KateFileTreeModel::DocumentTreeRole);
  if (!v.isValid()) return;
  QList<KTextEditor::Document*> closingDocuments = v.value<QList<KTextEditor::Document*> >();
  Kate::application()->documentManager()->closeDocumentList(closingDocuments);
}

void KateFileTree::slotCopyFilename()
{
  KTextEditor::Document *doc = model()->data(m_indexContextMenu, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if (doc) {
    QApplication::clipboard()->setText(doc->url().url());
  }
}

void KateFileTree::switchDocument( const QString &doc )
{
  if (doc.isEmpty()) {
    // no argument: switch to the previous document
    slotDocumentPrev();
  } else if (doc.toInt() > 0 &&
             doc.toInt() <= model()->rowCount( model()->parent(currentIndex()) )) {
    // numerical argument: switch to the nth document
    int i = doc.toInt() - 1;
    KTextEditor::Document *doc =
      model()->data(model()->index(i - 1, 0),
                    KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    if (doc) {
      emit activateDocument(doc);
    }
  } else {
    // string argument: switch to the given file
    QModelIndexList matches =
      model()->match(model()->index(0, 0), Qt::DisplayRole, QVariant(doc), 1, Qt::MatchContains);
    if (!matches.isEmpty()) {
      KTextEditor::Document *doc =
        model()->data(matches.takeFirst(),
                      KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
      if (doc) {
        emit activateDocument(doc);
      }
    }
  }
}

void KateFileTree::slotDocumentFirst()
{
  KTextEditor::Document *doc =
    model()->data(model()->index(0, 0),
                  KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if (doc) {
    emit activateDocument(doc);
  }
}

void KateFileTree::slotDocumentLast()
{
  int count = model()->rowCount( model()->parent(currentIndex()) );
  KTextEditor::Document *doc =
    model()->data(model()->index(count - 1, 0),
                  KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if (doc) {
    emit activateDocument(doc);
  }
}

void KateFileTree::slotDocumentPrev()
{
  qCDebug(FILETREE) << "BEGIN";
  KateFileTreeProxyModel *ftpm = static_cast<KateFileTreeProxyModel*>(model());


  
  QModelIndex current_index = currentIndex();
  QModelIndex prev;
  
  // scan up the tree skipping any dir nodes
  
  //qCDebug(FILETREE) << "cur" << ftpm->data(current_index, Qt::DisplayRole);
  while(current_index.isValid()) {
    if(current_index.row() > 0) {
      current_index = ftpm->sibling(current_index.row()-1, current_index.column(), current_index);
      //qCDebug(FILETREE) << "get prev" << ftpm->data(current_index, Qt::DisplayRole);
      if(!current_index.isValid()) {
        //qCDebug(FILETREE) << "somehow getting prev index from sibling didn't work :(";
        break;
      }
      
      if(ftpm->isDir(current_index)) {
        // try and select the last child in this parent
        //qCDebug(FILETREE) << "is a dir";
        int children = ftpm->rowCount(current_index);
        current_index = ftpm->index(children-1, 0, current_index);
        //qCDebug(FILETREE) << "child" << ftpm->data(current_index, Qt::DisplayRole);
        if(ftpm->isDir(current_index)) {
          // since we're a dir, keep going
          //qCDebug(FILETREE) << "child is a dir";
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
          //qCDebug(FILETREE) << "got doc 1";
          prev = current_index;
          break;
        }
      } else { // found document item
        //qCDebug(FILETREE) << "got doc 2";
        prev = current_index;
        break;
      }
    }
    else {
      //qCDebug(FILETREE) << "get parent";
      // just select the parent, the logic above will handle the rest
      current_index = ftpm->parent(current_index);
      //qCDebug(FILETREE) << "got parent" << ftpm->data(current_index, Qt::DisplayRole);
      if(!current_index.isValid()) {
        // paste the root node here, try and wrap around
        //qCDebug(FILETREE) << "parent invalid";
        
        int children = ftpm->rowCount(current_index);
        QModelIndex last_index = ftpm->index(children-1, 0, current_index);
        //qCDebug(FILETREE) << "last" << ftpm->data(last_index, Qt::DisplayRole);
        if(!last_index.isValid())
          break;
        
        if(ftpm->isDir(last_index)) {
          // last node is a dir, select last child row
          //qCDebug(FILETREE) << "last root is a dir, select child";
          int last_children = ftpm->rowCount(last_index);
          prev = ftpm->index(last_children-1, 0, last_index);
          //qCDebug(FILETREE) << "last child" << ftpm->data(current_index, Qt::DisplayRole);
          // bug here?
          break;
        }
        else {
          // got last file node
          //qCDebug(FILETREE) << "got doc";
          prev = last_index;
          break;
        }
      }
    }
  }
  
  if(prev.isValid()) {
    //qCDebug(FILETREE) << "got prev node:" << prev;
    //qCDebug(FILETREE) << "doc:" << ftpm->data(prev, Qt::DisplayRole).value<QString>();

    KTextEditor::Document *doc = model()->data(prev, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    emit activateDocument(doc);
  }
  else {
    qCDebug(FILETREE) << "didn't get prev node :(";
  }
  
  qCDebug(FILETREE) << "END";
}

/*
plan:

default: select next sibling
if cur is a dir, select it

*/

void KateFileTree::slotDocumentNext()
{
  qCDebug(FILETREE) << "BEGIN";
  
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
        // paste the root node here, try and wrap around
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
    //qCDebug(FILETREE) << "got next node:" << next;
    //qCDebug(FILETREE) << "doc:" << ftpm->data(next, Qt::DisplayRole).value<QString>();

    KTextEditor::Document *doc = model()->data(next, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
    emit activateDocument(doc);
  }
  else {
    qCDebug(FILETREE) << "didn't get next node :(";
  }
  
  qCDebug(FILETREE) << "END";
}
//END KateFileTree

// kate: space-indent on; indent-width 2; replace-tabs on;
