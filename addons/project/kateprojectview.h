/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QTimer>
#include <QWidget>

class KLineEdit;
class KateProjectPluginView;
class BranchesDialog;
class QToolButton;
class QStackedWidget;
class FileHistoryWidget;
class KateProject;
class KateProjectViewTree;

/**
 * Class representing a view of a project.
 * A tree like view of project content.
 */
class KateProjectView : public QWidget
{
public:
    /**
     * construct project view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectView(KateProjectPluginView *pluginView, KateProject *project);

    /**
     * deconstruct project
     */
    ~KateProjectView() override;

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

private:
    /**
     * React on filter change
     * @param filterText new filter text
     */
    void filterTextChanged();

    /**
     * On project model change, check if project
     * is a git repo and then show/hide the branch
     * button accordingly
     */
    void checkAndRefreshGit();

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
     * our tree view
     */
    KateProjectViewTree *m_treeView;

    /**
     * filter
     */
    KLineEdit *m_filter;

    /**
     * watches for changes to .git/HEAD
     * If this is non-empty, we registered that file in the project watcher
     */
    QString m_branchChangedWatcherFile;

    /**
     * filter timer
     */
    QTimer m_filterStartTimer;

    /**
     * project reload debouncing timer
     */
    QTimer m_projectReloadTimer;
};
