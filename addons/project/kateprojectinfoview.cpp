/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2012 Christoph Cullmann <cullmann@kde.org>
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
