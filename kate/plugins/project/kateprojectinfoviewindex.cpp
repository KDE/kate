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

#include "kateprojectinfoviewindex.h"
#include "kateprojectpluginview.h"

#include <QVBoxLayout>

KateProjectInfoViewIndex::KateProjectInfoViewIndex (KateProjectPluginView *pluginView, KateProject *project)
  : QWidget ()
  , m_pluginView (pluginView)
  , m_project (project)
  , m_lineEdit (new QLineEdit())
  , m_treeView (new QTreeView())
  , m_model (new QStandardItemModel (m_treeView))
{
  /**
   * default style
   */
  m_treeView->setEditTriggers (QAbstractItemView::NoEditTriggers);
  
  /**
   * attach model
   * kill selection model
   */
  QItemSelectionModel *m = m_treeView->selectionModel();
  m_treeView->setModel (m_model);
  delete m;
  
  /**
   * layout widget
   */
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing (0);
  layout->addWidget (m_lineEdit);
  layout->addWidget (m_treeView);
  setLayout (layout);
  
  /**
   * connect needed signals
   */
  connect (m_lineEdit, SIGNAL(textChanged (const QString &)), this, SLOT(slotTextChanged (const QString &)));
  
  /**
   * trigger once search with nothing
   */
  slotTextChanged (QString());
}

KateProjectInfoViewIndex::~KateProjectInfoViewIndex ()
{
}

void KateProjectInfoViewIndex::slotTextChanged (const QString &text)
{
  /**
   * setup model
   */
  m_model->clear();
  m_model->setHorizontalHeaderLabels (QStringList () << "Name" << "Kind" << "File" << "Line");
  
  /**
   * get results
   */
  if (m_project->projectIndex() && !text.isEmpty())
    m_project->projectIndex()->findMatches (*m_model, text, KateProjectIndex::FindMatches);
  
  /**
   * resize
   */
  m_treeView->resizeColumnToContents (2);
  m_treeView->resizeColumnToContents (1);
  m_treeView->resizeColumnToContents (0);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
