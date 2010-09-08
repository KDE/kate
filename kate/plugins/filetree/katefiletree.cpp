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

  QPalette p = palette();
  p.setColor(QPalette::Inactive, QPalette::Highlight, p.color(QPalette::Active, QPalette::Highlight));
  p.setColor(QPalette::Inactive, QPalette::HighlightedText, p.color(QPalette::Active, QPalette::HighlightedText));
  setPalette(p);
}

KateFileTree::~KateFileTree()
{}

void KateFileTree::currentChanged ( const QModelIndex &current, const QModelIndex &previous )
{
  kDebug(debugArea()) << "current:" << current << "previous:" << previous;

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
  
  QMenu menu;
  menu.addAction(m_filelistCloseDocument);
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
