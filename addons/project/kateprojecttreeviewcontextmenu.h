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

class QWidget;

class KateProjectTreeViewContextMenu
{
public:
    /**
     * construct project view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectTreeViewContextMenu();

    /**
     * deconstruct project
     */
    ~KateProjectTreeViewContextMenu();

    /**
     * our project.
     * @return project
     */
    void exec(const QString &filename, const QPoint &pos, QWidget *parent);

protected:
};

#endif
