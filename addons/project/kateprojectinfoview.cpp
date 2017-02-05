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
#include "kateprojectpluginview.h"
#include "kateprojectinfoviewterminal.h"
#include "kateprojectinfoviewindex.h"
#include "kateprojectinfoviewcodeanalysis.h"
#include "kateprojectinfoviewnotes.h"

#include "klocalizedstring.h"

KateProjectInfoView::KateProjectInfoView(KateProjectPluginView *pluginView, KateProject *project)
    : QTabWidget()
    , m_pluginView(pluginView)
    , m_project(project)
{
    /**
     * terminal
     */
    addTab(new KateProjectInfoViewTerminal(pluginView, project), i18n("Terminal"));

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
