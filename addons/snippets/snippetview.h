/*  This file is part of the Kate project.
 *  Based on the snippet plugin from KDevelop 4.
 *
 *  Copyright (C) 2007 Robert Gruber <rgruber@users.sourceforge.net> 
 *  Copyright (C) 2010 Milian Wolff <mail@milianw.de>
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2014 Sven Brauch <svenbrauch@gmail.com>
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

#ifndef SNIPPETVIEW_H
#define SNIPPETVIEW_H

#include "ui_snippetview.h"

class QStandardItem;
class KateSnippetGlobal;
class QAction;
class QSortFilterProxyModel;

namespace KTextEditor {
}

/**
 * This class gets embedded into the right tool view by the KateSnippetGlobal.
 * @author Robert Gruber <rgruber@users.sourceforge.net>
 * @author Milian Wolff <mail@milianw.de>
 */
class SnippetView : public QWidget, public Ui::SnippetViewBase
{
    Q_OBJECT

public:
    explicit SnippetView(KateSnippetGlobal* plugin, QWidget* parent = nullptr);

public:
    void setupActionsForWindow(QWidget* widget);

private Q_SLOTS:
    /**
     * Opens the "Add Repository" dialog.
     */
    void slotAddRepo();

    /**
     * Opens the "Edit repository" dialog.
     */
    void slotEditRepo();

    /**
     * Removes the selected repository from the disk.
     */
    void slotRemoveRepo();

    /**
     * Insert the selected snippet into the current file
     */
    void slotSnippetClicked(const QModelIndex & index);

    /**
     * Open the edit dialog for the selected snippet
     */
    void slotEditSnippet();

    /**
     * Removes the selected snippet from the tree and the filesystem
     */
    void slotRemoveSnippet();

    /**
     * Creates a new snippet and open the edit dialog for it
     */
    void slotAddSnippet();

    /**
     * Slot to get hot new stuff.
     */
    void slotGHNS();

    /**
     * Slot to put the selected snippet to GHNS
     */
    void slotSnippetToGHNS();

    void contextMenu (const QPoint & pos);
    /// disables or enables available actions based on the currently selected item
    void validateActions();

    /// insert snippet on double click
    bool eventFilter(QObject* , QEvent* ) override;
private:
    QStandardItem* currentItem();

    KateSnippetGlobal* m_plugin;
    QSortFilterProxyModel* m_proxy;

    QAction *m_addRepoAction;
    QAction *m_removeRepoAction;
    QAction *m_editRepoAction;
    QAction *m_addSnippetAction;
    QAction *m_removeSnippetAction;
    QAction *m_editSnippetAction;
    QAction *m_getNewStuffAction;
    QAction *m_putNewStuffAction;
};

#endif

