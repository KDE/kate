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
  //setUniformRowHeights(true);
  setAllColumnsShowFocus(true);
  setIconSize(QSize(22,22));

  connect( this, SIGNAL(pressed(const QModelIndex &)), this, SLOT(mousePressed(const QModelIndex &)));
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

void KateFileTree::currentChanged ( const QModelIndex &current, const QModelIndex &previous )
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

void KateFileTree::mousePressed ( const QModelIndex &index )
{
  kDebug(debugArea()) << "got index" << index;
  
  KTextEditor::Document *doc = model()->data(index, KateFileTreeModel::DocumentRole).value<KTextEditor::Document *>();
  if(doc) {
    kDebug(debugArea()) << "got doc, setting prev:" << index;
    m_previouslySelected = index;
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

  if (m_previouslySelected.isValid()) {
    selectionModel()->setCurrentIndex(m_previouslySelected,QItemSelectionModel::ClearAndSelect);
  }

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
  
  event->accept();
}

void KateFileTree::slotDocumentClose() {
  m_previouslySelected = QModelIndex();
  QVariant v = m_indexContextMenu.data(KateFileTreeModel::DocumentRole);
  if (!v.isValid()) return;
  Kate::application()->documentManager()->closeDocument(v.value<KTextEditor::Document*>());
}

//END KateFileTree

// kate: space-indent on; indent-width 2; replace-tabs on;
