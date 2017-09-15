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

#ifndef KATE_PROJECT_INFO_VIEW_INDEX_H
#define KATE_PROJECT_INFO_VIEW_INDEX_H

#include "kateproject.h"

#include <QLineEdit>
#include <QTreeView>

class KateProjectPluginView;
class KMessageWidget;

/**
 * Class representing a view of a project.
 * A tree like view of project content.
 */
class KateProjectInfoViewIndex : public QWidget
{
    Q_OBJECT

public:
    /**
     * construct project info view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectInfoViewIndex(KateProjectPluginView *pluginView, KateProject *project);

    /**
     * deconstruct info view
     */
    ~KateProjectInfoViewIndex() override;

    /**
     * our project.
     * @return project
     */
    KateProject *project() const {
        return m_project;
    }

private Q_SLOTS:
    /**
     * Called if text in lineedit changes, then we need to search
     * @param text new text
     */
    void slotTextChanged(const QString &text);

    /**
     * item got clicked, do stuff, like open document
     * @param index model index of clicked item
     */
    void slotClicked(const QModelIndex &index);

    /**
     * called whenever the index of the project was updated. Here,
     * it's used to show a warning, if ctags is not installed.
     */
    void indexAvailable();

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
     * information widget showing a warning about missing ctags.
     */
    KMessageWidget *m_messageWidget;

    /**
     * line edit which allows to search index
     */
    QLineEdit *m_lineEdit;

    /**
     * tree view for results
     */
    QTreeView *m_treeView;

    /**
     * standard item model for results
     */
    QStandardItemModel *m_model;
};

#endif

