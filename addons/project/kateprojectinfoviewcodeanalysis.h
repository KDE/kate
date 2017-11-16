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

#ifndef KATE_PROJECT_INFO_VIEW_CODE_ANALYSIS_H
#define KATE_PROJECT_INFO_VIEW_CODE_ANALYSIS_H

#include "kateproject.h"

#include <QPushButton>
#include <QProcess>
#include <QTreeView>
#include <QComboBox>

class KateProjectPluginView;
class KateProjectCodeAnalysisTool;
class KMessageWidget;

/**
 * View for Code Analysis.
 * cppcheck and perhaps later more...
 */
class KateProjectInfoViewCodeAnalysis : public QWidget
{
    Q_OBJECT

public:
    /**
     * construct project info view for given project
     * @param pluginView our plugin view
     * @param project project this view is for
     */
    KateProjectInfoViewCodeAnalysis(KateProjectPluginView *pluginView, KateProject *project);

    /**
     * deconstruct info view
     */
    ~KateProjectInfoViewCodeAnalysis() override;

    /**
     * our project.
     * @return project
     */
    KateProject *project() const {
        return m_project;
    }

private Q_SLOTS:
    /**
     * Called if start/stop button is clicked.
     */
    void slotStartStopClicked();

    /**
     * More checker output is available
     */
    void slotReadyRead();

    /**
     * item got clicked, do stuff, like open document
     * @param index model index of clicked item
     */
    void slotClicked(const QModelIndex &index);

    /**
     * Analysis finished
     * @param exitCode analyser process exit code
     * @param exitStatus analyser process exit status
     */
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

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
     * start/stop analysis button
     */
    QPushButton *m_startStopAnalysis;

    /**
     * tree view for results
     */
    QTreeView *m_treeView;

    /**
     * standard item model for results
     */
    QStandardItemModel *m_model;

    /**
     * running analyzer process
     */
    QProcess *m_analyzer;

    KateProjectCodeAnalysisTool *m_analysisTool;

    QComboBox *m_toolSelector;

};

#endif

