/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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
#include "kateexternaltoolsview.h"
#include "externaltoolsplugin.h"
#include "kateexternaltool.h"

#include <KTextEditor/MainWindow>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KAuthorized>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QStandardPaths>
#include <QMenu>
#include <KXMLGUIFactory>
#include <KLocalizedString>

#include <map>
#include <vector>


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

KateExternalToolsMenuAction::~KateExternalToolsMenuAction() = default;

void KateExternalToolsMenuAction::reload()
{
    // clear action collection
    bool needs_readd = (m_actionCollection->takeAction(this) != nullptr);
    m_actionCollection->clear();
    if (needs_readd)
        m_actionCollection->addAction(QStringLiteral("tools_external"), this);
    menu()->clear();

    // create tool actions
    std::map<QString, KActionMenu*> categories;
    std::vector<QAction*> uncategorizedActions;

    // first add categorized actions, such that the submenus appear at the top
    for (auto tool : m_plugin->tools()) {
        if (tool->hasexec) {
            auto a = new QAction(tool->name, this);
            a->setIcon(QIcon::fromTheme(tool->icon));
            a->setData(QVariant::fromValue(tool));

            connect(a, &QAction::triggered, [this, a]() {
                m_plugin->runTool(*a->data().value<KateExternalTool*>(), m_mainwindow->activeView());
            });

            m_actionCollection->addAction(tool->actionName, a);
            if (!tool->category.isEmpty()) {
                auto categoryMenu = categories[tool->category];
                if (!categoryMenu) {
                    categoryMenu = new KActionMenu(tool->category, this);
                    categories[tool->category] = categoryMenu;
                    addAction(categoryMenu);
                }
                categoryMenu->addAction(a);
            } else {
                uncategorizedActions.push_back(a);
            }
        }
    }

    // now add uncategorized actions below
    for (auto uncategorizedAction : uncategorizedActions) {
        addAction(uncategorizedAction);
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
            action->setEnabled(tool->matchesMimetype(mimeType));
        }
    }
}
// END KateExternalToolsMenuAction





// BEGIN KateExternalToolsPluginView
KateExternalToolsPluginView::KateExternalToolsPluginView(KTextEditor::MainWindow* mainWindow,
                                                         KateExternalToolsPlugin* plugin)
    : QObject(mainWindow)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
{
    m_plugin->registerPluginView(this);

    KXMLGUIClient::setComponentName(QLatin1String("externaltools"), i18n("External Tools"));
    setXMLFile(QLatin1String("ui.rc"));

    if (KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
        m_externalToolsMenu = new KateExternalToolsMenuAction(i18n("External Tools"), actionCollection(), plugin, mainWindow);
        actionCollection()->addAction(QStringLiteral("tools_external"), m_externalToolsMenu);
        m_externalToolsMenu->setWhatsThis(i18n("Launch external helper applications"));
    }

    mainWindow->guiFactory()->addClient(this);

KateExternalToolsPluginView::~KateExternalToolsPluginView()
{
    m_plugin->unregisterPluginView(this);

    m_mainWindow->guiFactory()->removeClient(this);


    delete m_externalToolsMenu;
    m_externalToolsMenu = nullptr;
}

void KateExternalToolsPluginView::rebuildMenu()
{
    if (m_externalToolsMenu) {
        KXMLGUIFactory* f = factory();
        f->removeClient(this);
        reloadXML();
        m_externalToolsMenu->reload();
        f->addClient(this);
    }
}

KTextEditor::MainWindow* KateExternalToolsPluginView::mainWindow() const
{
    return m_mainWindow;
}
// END KateExternalToolsPluginView

// kate: space-indent on; indent-width 4; replace-tabs on;
