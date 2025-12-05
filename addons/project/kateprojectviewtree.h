/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QTreeView>

class KateProjectPluginView;
class KateProject;

namespace KTextEditor
{
class MainWindow;
}

/**
 * A tree like view of project content.
 */
class KateProjectViewTree : public QTreeView
{
    friend class KateProjectTreeDelegate;

public:
    /**
     * construct project view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectViewTree(KateProjectPluginView *pluginView, KateProject *project);

    /**
     * deconstruct project
     */
    ~KateProjectViewTree() override;

    /**
     * our project.
     * @return project
     */
    KateProject *project() const
    {
        return m_project;
    }

    /**
     * Select given file in the view.
     * @param file select this file in the view, will be shown if invisible
     */
    void selectFile(const QString &file);

    /**
     * Open the selected document, if any.
     */
    void openSelectedDocument();

    /**
     * Add a new file
     */
    void addFile(const QModelIndex &idx, const QString &fileName);

    /**
     * Add a new directory
     */
    void addDirectory(const QModelIndex &idx, const QString &name);

    /**
     * Remove a path, the function doesn't close documents before removing
     */
    void removePath(const QModelIndex &idx, const QString &fullPath);

    /**
     * Open project terminal at location dirPath
     */
    void openTerminal(const QString &dirPath);

    KTextEditor::MainWindow *mainWindow();

private:
    /**
     * item got clicked, do stuff, like open document
     * @param index model index of clicked item
     */
    void slotClicked(const QModelIndex &index);

    /**
     * Triggered on model changes.
     * This includes the files list, itemForFile mapping!
     */
    void slotModelChanged();

    void flattenPath(const QModelIndex &index);

    /**
     * For the given index, unflatten the tree and then return the
     * item for the part of the path that was clicked
     *
     * e.g., idx = /path/child1/child2
     * it will turn idx into a tree:
     * - path
     *   - child1
     *     - child2
     * then return child1 item
     *
     * This function invokes a Queued call to flattenPath. This reflattens
     * the path once the operation is done. Used by addFile/addFolder
     */
    class QStandardItem *unflattenTreeAndReturnClickedItem(const QModelIndex &idx);

protected:
    /**
     * Create matching context menu.
     * @param event context menu event
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

    void mouseMoveEvent(QMouseEvent *e) override;

private:
    /**
     * our plugin view
     */
    KateProjectPluginView *m_pluginView;

    /**
     * our project
     */
    KateProject *m_project;

    /**
     * List of nodes that are expanded. This is used to restore expansion state after project reload
     */
    QSet<QString> m_expandedNodes;

    /**
     * saved scroll position for restore later
     */
    int m_verticalScrollPosition = 0;

    QPoint m_lastMousePosition;
};
