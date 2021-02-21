/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KATE_PROJECT_VIEW_H
#define KATE_PROJECT_VIEW_H

#include "kateproject.h"
#include "kateprojectviewtree.h"

#include <QFileSystemWatcher>

class KLineEdit;
class KateProjectPluginView;
class BranchesDialog;
class QToolButton;

/**
 * Class representing a view of a project.
 * A tree like view of project content.
 */
class KateProjectView : public QWidget
{
    Q_OBJECT

public:
    /**
     * construct project view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectView(KateProjectPluginView *pluginView, KateProject *project, KTextEditor::MainWindow *mainWindow);

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

private Q_SLOTS:
    /**
     * React on filter change
     * @param filterText new filter text
     */
    void filterTextChanged(const QString &filterText);

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
      checkout branch button
     */
    QToolButton *m_branchBtn;

    /**
     * The dialog which displays git branches
     */
    BranchesDialog *m_branchesDialog;

    /**
     * watches for changes to .git/HEAD
     */
    QFileSystemWatcher m_branchChangedWatcher;
};

#endif
