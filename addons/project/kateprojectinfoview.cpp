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

#include <KAuthorized>
#include <KLocalizedString>

#include <QFileInfo>

KateProjectInfoView::KateProjectInfoView(KateProjectPluginView *pluginView, KateProject *project)
    : m_project(project)
    , m_pluginView(pluginView)
{
    setDocumentMode(true);
}

void KateProjectInfoView::showEvent(QShowEvent *)
{
    initialize();
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
    initialize();
    if (m_terminal) {
        m_terminal->respawn(directory);
    }
}

void KateProjectInfoView::initialize()
{
    if (m_initialized) {
        return;
    }
    m_initialized = true;
    /**
     * skip terminal toolviews if no terminal aka KonsolePart around
     */
    if (KateProjectInfoViewTerminal::isLoadable()) {
        const QFileInfo projectInfo(QFileInfo(m_project->fileName()).path());
        const QString projectPath = projectInfo.absoluteFilePath();

        const QFileInfo baseInfo(m_project->baseDir());
        const QString basePath = baseInfo.absoluteFilePath();

        const QFileInfo buildInfo(m_project->projectMap().value(QStringLiteral("build")).toMap().value(QStringLiteral("directory")).toString());
        const QString buildPath = buildInfo.absoluteFilePath();

        const bool projectTerminalAvailable = !projectPath.isEmpty() && projectInfo.exists() && KAuthorized::authorize(QStringLiteral("shell_access"));
        const bool baseTerminalAvailable =
            !basePath.isEmpty() && projectPath != basePath && baseInfo.exists() && KAuthorized::authorize(QStringLiteral("shell_access"));
        const bool buildTerminalAvailable = !buildPath.isEmpty() && projectPath != buildPath && basePath != buildPath && buildInfo.exists()
            && KAuthorized::authorize(QStringLiteral("shell_access"));

        const int availableTabCount =
            static_cast<int>(projectTerminalAvailable) + static_cast<int>(baseTerminalAvailable) + static_cast<int>(buildTerminalAvailable);

        // True if there is more than one terminal tab, and we have to label what each one is for.
        const bool tabsNeedDisambiguation = availableTabCount > 1;

        const QString genericTerminalLabel = i18nc("@title:tab", "Terminal");

        /**
         * terminal for the directory with the .kateproject file inside
         */
        if (projectTerminalAvailable) {
            m_terminal = new KateProjectInfoViewTerminal(m_pluginView, projectPath);
            addTab(m_terminal,
                   QIcon::fromTheme(QStringLiteral("utilities-terminal-symbolic")),
                   tabsNeedDisambiguation ? i18nc("@title:tab", "Terminal (Project)") : genericTerminalLabel);
        }

        /**
         * terminal for the base directory, if different to directory of .kateproject
         */
        if (baseTerminalAvailable) {
            addTab(new KateProjectInfoViewTerminal(m_pluginView, basePath),
                   QIcon::fromTheme(QStringLiteral("utilities-terminal-symbolic")),
                   tabsNeedDisambiguation ? i18nc("@title:tab", "Terminal (Base)") : genericTerminalLabel);
        }

        /**
         * terminal for the build directory
         */
        if (buildTerminalAvailable) {
            addTab(new KateProjectInfoViewTerminal(m_pluginView, buildPath),
                   QIcon::fromTheme(QStringLiteral("utilities-terminal-symbolic")),
                   tabsNeedDisambiguation ? i18nc("@title:tab", "Terminal (Build)") : genericTerminalLabel);
        }
    }

    /**
     * index
     */
    addTab(new KateProjectInfoViewIndex(m_pluginView, m_project), i18n("Code Index"));

    /**
     * code analysis
     */
    addTab(new KateProjectInfoViewCodeAnalysis(m_pluginView, m_project), i18n("Code Analysis"));

    /**
     * notes
     */
    addTab(new KateProjectInfoViewNotes(m_project->notesDocument()), i18n("Notes"));
}

void KateProjectInfoView::runCmdInTerminal(const QString &cmd)
{
    initialize();
    m_terminal->runCommand({}, cmd);
}

#include "moc_kateprojectinfoview.cpp"
