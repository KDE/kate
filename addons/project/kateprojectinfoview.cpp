/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectinfoview.h"
#include "kateproject.h"
#include "kateprojectinfoviewcodeanalysis.h"
#include "kateprojectinfoviewindex.h"
#include "kateprojectinfoviewnotes.h"
#include "kateprojectinfoviewterminal.h"
#include "kateprojectpluginview.h"

#include <KLocalizedString>

#include <QFileInfo>

KateProjectInfoView::KateProjectInfoView(KateProjectPluginView *pluginView, KateProject *project)
    : m_project(project)
    , m_terminal(nullptr)
{
    setDocumentMode(true);

    /**
     * skip terminal toolviews if no terminal aka KonsolePart around
     */
    if (KateProjectInfoViewTerminal::isLoadable()) {
        /**
         * terminal for the directory with the .kateproject file inside
         */
        const QFileInfo projectInfo(QFileInfo(m_project->fileName()).path());
        const QString projectPath = projectInfo.absoluteFilePath();
        if (!projectPath.isEmpty() && projectInfo.exists()) {
            m_terminal = new KateProjectInfoViewTerminal(pluginView, projectPath);
            addTab(m_terminal, i18n("Terminal (.kateproject)"));
        }

        /**
         * terminal for the base directory, if different to directory of .kateproject
         */
        const QFileInfo baseInfo(m_project->baseDir());
        const QString basePath = baseInfo.absoluteFilePath();
        if (!basePath.isEmpty() && projectPath != basePath && baseInfo.exists()) {
            addTab(new KateProjectInfoViewTerminal(pluginView, basePath), i18n("Terminal (Base)"));
        }

        /**
         * terminal for the build directory
         */
        const QFileInfo buildInfo(m_project->projectMap().value(QStringLiteral("build")).toMap().value(QStringLiteral("directory")).toString());
        const QString buildPath = buildInfo.absoluteFilePath();
        if (!buildPath.isEmpty() && projectPath != buildPath && basePath != buildPath && buildInfo.exists()) {
            m_terminal = new KateProjectInfoViewTerminal(pluginView, buildPath);
            addTab(m_terminal, i18n("Terminal (build)"));
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
    addTab(new KateProjectInfoViewNotes(project), i18n("Notes"));
}

KateProjectInfoView::~KateProjectInfoView()
{
}

void KateProjectInfoView::showEvent(QShowEvent *)
{
    setFocusProxy(currentWidget());
}

bool KateProjectInfoView::ignoreEsc() const
{
    // we want to ignore stuff for some kinds of running shell processes like vim
    if (const auto terminal = qobject_cast<const KateProjectInfoViewTerminal *>(currentWidget())) {
        return terminal->ignoreEsc();
    }

    // else: always hide toolview, nothing to ignore
    return false;
}

void KateProjectInfoView::resetTerminal(const QString &directory)
{
    if (m_terminal) {
        m_terminal->respawn(directory);
    }
}

void KateProjectInfoView::runCmdInTerminal(const QString &workingDir, const QString &cmd)
{
    if (auto terminal = qobject_cast<KateProjectInfoViewTerminal *>(currentWidget())) {
        terminal->runCommand(workingDir, cmd);
    }
}

#include "moc_kateprojectinfoview.cpp"
