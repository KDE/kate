/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectinfoview.h"
#include "kateprojectinfoviewcodeanalysis.h"
#include "kateprojectinfoviewindex.h"
#include "kateprojectinfoviewnotes.h"
#include "kateprojectinfoviewterminal.h"
#include "kateprojectpluginview.h"

#include <KLocalizedString>

#include <QFileInfo>

KateProjectInfoView::KateProjectInfoView(KateProjectPluginView *pluginView, KateProject *project)
    : m_pluginView(pluginView)
    , m_project(project)
{
    /**
     * skip terminal toolviews if no terminal aka KonsolePart around
     */
    if (KateProjectInfoViewTerminal::pluginFactory()) {
        /**
         * terminal for the directory with the .kateproject file inside
         */
        const QString projectPath = QFileInfo(QFileInfo(m_project->fileName()).path()).canonicalFilePath();
        if (!projectPath.isEmpty()) {
            addTab(new KateProjectInfoViewTerminal(pluginView, projectPath), i18n("Terminal (.kateproject)"));
        }

        /**
         * terminal for the base directory, if different to directory of .kateproject
         */
        const QString basePath = QFileInfo(m_project->baseDir()).canonicalFilePath();
        if (!basePath.isEmpty() && projectPath != basePath) {
            addTab(new KateProjectInfoViewTerminal(pluginView, basePath), i18n("Terminal (Base)"));
        }
    }

    /**
     * index
     */
    addTab(new KateProjectInfoViewIndex(pluginView, project), i18n("Code Index"));

    /**
     * code analysis
     */
    addTab(new KateProjectInfoViewCodeAnalysis(pluginView, project), i18n("Code Analysis"));

    /**
     * notes
     */
    addTab(new KateProjectInfoViewNotes(pluginView, project), i18n("Notes"));
}

KateProjectInfoView::~KateProjectInfoView()
{
}

void KateProjectInfoView::showEvent(QShowEvent *)
{
    setFocusProxy(currentWidget());
}
