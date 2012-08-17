/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kateprojectview.h"
#include "kateprojectpluginview.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QContextMenuEvent>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <QMenu>
#include <KRun>
#include <KIcon>

KateProjectView::KateProjectView (KateProjectPluginView *pluginView, KateProject *project)
  : QTreeView ()
  , m_pluginView (pluginView)
  , m_project (project)
{
  /**
   * default style
   */
  setHeaderHidden (true);
  setEditTriggers (QAbstractItemView::NoEditTriggers);

   /**
    * attach view => project
    */
  setModel (m_project->model ());

  /**
   * connect needed signals
   */
  connect (this, SIGNAL(activated (const QModelIndex &)), this, SLOT(slotActivated (const QModelIndex &)));
  connect (m_project, SIGNAL(modelChanged ()), this, SLOT(slotModelChanged ()));

  /**
   * trigger once some slots
   */
  slotModelChanged ();
}

KateProjectView::~KateProjectView ()
{
}

void KateProjectView::selectFile (const QString &file)
{
  /**
   * get item if any
   */
  QStandardItem *item = m_project->itemForFile (file);
  if (!item)
    return;

  /**
   * select it
   */
  QModelIndex index = m_project->model()->indexFromItem (item);
  scrollTo (index, QAbstractItemView::EnsureVisible);
  selectionModel()->setCurrentIndex (index, QItemSelectionModel::Clear | QItemSelectionModel::Select);
}

void KateProjectView::slotActivated (const QModelIndex &index)
{
  /**
   * open document, if any usable user data
   */
  QString filePath = index.data (Qt::UserRole).toString();
  if (!filePath.isEmpty())
    m_pluginView->mainWindow()->openUrl (KUrl::fromPath (filePath));
}

void KateProjectView::slotModelChanged ()
{
  /**
   * model was updated
   * perhaps we need to highlight again new file
   */
  KTextEditor::View *activeView = m_pluginView->mainWindow()->activeView ();
  if (activeView && activeView->document()->url().isLocalFile())
    selectFile (activeView->document()->url().toLocalFile ());
}

void KateProjectView::contextMenuEvent (QContextMenuEvent *event)
{
  // get current file path
  QModelIndex index = selectionModel()->currentIndex();
  QString filePath = index.data (Qt::UserRole).toString();
  if (!filePath.isEmpty()) {
    // find correct mimetype to query for possible applications
    KMimeType::Ptr mimeType = KMimeType::findByPath(filePath);
    KService::List offers = KMimeTypeTrader::self()->query(mimeType->name(), "Application");

    // create context menu
    QMenu menu;
    QMenu *openWithMenu = menu.addMenu(i18n("Open With"));

    QAction *action = 0;
    // for each one, insert a menu item...
    for(KService::List::Iterator it = offers.begin(); it != offers.end(); ++it)
    {
      KService::Ptr service = *it;
      if (service->name() == "Kate") continue; // omit Kate
      action = openWithMenu->addAction(KIcon(service->icon()), service->name());
      action->setData(service->entryPath());
    }

    // launch the menu
    action = menu.exec(viewport()->mapToGlobal(event->pos()));

    // and launch the requested application
    if (action) {
      const QString openWith = action->data().toString();
      KService::Ptr app = KService::serviceByDesktopPath(openWith);
      if (app)
      {
        QList<QUrl> list;
        list << QUrl(filePath);
        KRun::run(*app, list, this);
      }
    }
  }

  event->accept();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
