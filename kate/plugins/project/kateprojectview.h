/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_PROJECT_VIEW_H
#define KATE_PROJECT_VIEW_H

#include "kateproject.h"

#include <QTreeView>

/**
 * Class representing a view of a project.
 * A tree like view of project content.
 */
class KateProjectView : public QTreeView
{
  Q_OBJECT

  public:
    /**
     * construct project view for given project
     * @param project project this view is for
     */
    KateProjectView (KateProject *project);
    
    /**
     * deconstruct project
     */
    ~KateProjectView ();
    
  private:
    /**
     * our project
     */
    KateProject *m_project;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
