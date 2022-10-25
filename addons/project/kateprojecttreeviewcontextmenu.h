/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2013 Dominik Haumann <dhaumann.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KATE_PROJECT_TREE_VIEW_CONTEXT_MENU_H
#define KATE_PROJECT_TREE_VIEW_CONTEXT_MENU_H

#include <QPoint>
#include <QString>

class QModelIndex;
class KateProjectViewTree;

class KateProjectTreeViewContextMenu
{
public:
    /**
     * our project.
     * @return project
     */
    static void exec(const QString &filename, const QModelIndex &index, const QPoint &pos, KateProjectViewTree *parent);
};

#endif
