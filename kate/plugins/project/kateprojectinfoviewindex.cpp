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
}

KateProjectInfoViewIndex::~KateProjectInfoViewIndex ()
{
}

void KateProjectInfoViewIndex::slotTextChanged (const QString &text)
{
  /**
   * clear results and trigger search
   */
  m_model->clear();
  if (m_project->projectIndex())
    m_project->projectIndex()->findMatches (*m_model, text, KateProjectIndex::FindMatches);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
