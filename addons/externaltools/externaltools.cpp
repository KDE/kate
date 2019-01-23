/*
   This file is part of the Kate text editor of the KDE project.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.

   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
*/
// TODO
// Icons
// Direct shortcut setting
#include "externaltools.h"
#include "externaltoolsplugin.h"
#include "kateexternaltool.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KIconButton>
#include <KIconLoader>
#include <KMessageBox>
#include <KMimeTypeChooser>
#include <KSharedConfig>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>
#include <QStandardPaths>
#include <QMenu>

// BEGIN KateExternalToolsMenuAction
KateExternalToolsMenuAction::KateExternalToolsMenuAction(const QString& text, KActionCollection* collection,
                                                         KateExternalToolsPlugin* plugin, KTextEditor::MainWindow* mw)
    : KActionMenu(text, mw)
    , m_plugin(plugin)
    , m_mainwindow(mw)
    , m_actionCollection(collection)
{
    reload();

    // track active view to adapt enabled tool actions
    connect(mw, &KTextEditor::MainWindow::viewChanged, this, &KateExternalToolsMenuAction::slotViewChanged);
}

KateExternalToolsMenuAction::~KateExternalToolsMenuAction() {}

void KateExternalToolsMenuAction::reload()
{
    // clear action collection
    bool needs_readd = (m_actionCollection->takeAction(this) != nullptr);
    m_actionCollection->clear();
    if (needs_readd)
        m_actionCollection->addAction(QStringLiteral("tools_external"), this);
    menu()->clear();

    // create tool actions
    for (auto tool : m_plugin->tools()) {
        if (tool->hasexec) {
            auto a = new QAction(tool->name, this);
            a->setIcon(QIcon::fromTheme(tool->icon));
            a->setData(QVariant::fromValue(tool));

            connect(a, &QAction::triggered, [this, a]() {
                m_plugin->runTool(*a->data().value<KateExternalTool*>(), m_mainwindow->activeView());
            });

            m_actionCollection->addAction(tool->actionName, a);
            addAction(a);
        }
    }

    // load shortcuts
    KSharedConfig::Ptr pConfig = KSharedConfig::openConfig(QStringLiteral("externaltools"), KConfig::NoGlobals,
                                                           QStandardPaths::ApplicationsLocation);
    KConfigGroup config(pConfig, "Global");
    config = KConfigGroup(pConfig, "Shortcuts");
    m_actionCollection->readSettings(&config);
    slotViewChanged(m_mainwindow->activeView());
}

void KateExternalToolsMenuAction::slotViewChanged(KTextEditor::View* view)
{
    // no active view, oh oh
    if (!view) {
        return;
    }

    // try to enable/disable to match current mime type
    const QString mimeType = view->document()->mimeType();
    foreach (QAction* action, m_actionCollection->actions()) {
        if (action && action->data().value<KateExternalTool*>()) {
            auto tool = action->data().value<KateExternalTool*>();
            const bool toolActive = tool->mimetypes.isEmpty() || tool->mimetypes.contains(mimeType);
            action->setEnabled(toolActive);
        }
    }
}
// END KateExternalToolsMenuAction

// kate: space-indent on; indent-width 4; replace-tabs on;
