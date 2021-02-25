/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2013 Dominik Haumann <dhaumann.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KATE_PROJECT_TREE_VIEW_CONTEXT_MENU_H
#define KATE_PROJECT_TREE_VIEW_CONTEXT_MENU_H

#include <QObject>
#include <QPoint>
#include <QString>

class QWidget;
class QModelIndex;

class KateProjectTreeViewContextMenu : public QObject
{
    Q_OBJECT
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
    void exec(const QString &filename, const QModelIndex &index, const QPoint &pos, QWidget *parent);

    /**
     * emits on clicking Menu->Show File History
     */
    Q_SIGNAL void showFileHistory(const QString &file);

protected:
};

#endif
