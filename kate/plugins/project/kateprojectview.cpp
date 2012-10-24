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
#include <QSortFilterProxyModel>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <QMenu>
#include <KRun>
#include <KIcon>
#include <KFilterProxySearchLine>
#include <KLineEdit>

KateProjectView::KateProjectView (KateProjectPluginView *pluginView, KateProject *project)
  : QWidget ()
  , m_pluginView (pluginView)
  , m_project (project)
  , m_treeView (new KateProjectViewTree(pluginView, project))
  , m_filter (new KFilterProxySearchLine ())
{
  /**
   * layout tree view and co.
   */
  QVBoxLayout *layout = new QVBoxLayout ();
  layout->setSpacing (0);
  layout->addWidget (m_treeView);
  layout->addWidget (m_filter);
  setLayout (layout);
  
  /**
   * allow filtering
   * tree view always has a sortfilterproxy model
   */
  m_filter->setProxy (static_cast<QSortFilterProxyModel *>(m_treeView->model ()));
  
  /**
   * do some stuff if line edit is changed
   */
  connect (m_filter->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(filterTextChanged(QString)));
}

KateProjectView::~KateProjectView ()
{
}

void KateProjectView::selectFile (const QString &file)
{
  m_treeView->selectFile (file);
}

void KateProjectView::openSelectedDocument ()
{
  m_treeView->openSelectedDocument ();
}

void KateProjectView::filterTextChanged (QString filterText)
{
  if (!filterText.isEmpty())
    m_treeView->expandAll ();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
