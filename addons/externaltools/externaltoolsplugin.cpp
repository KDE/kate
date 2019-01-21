/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>

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
*/

#include "externaltoolsplugin.h"

#include "kateexternaltool.h"
#include "kateexternaltoolscommand.h"
#include "katemacroexpander.h"
#include "katetoolrunner.h"

#include <KIconLoader>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <QDebug>
#include <QIcon>

#include <KActionCollection>
#include <QAction>
#include <kparts/part.h>

#include <kmessagebox.h>

#include <KConfig>
#include <KConfigGroup>
#include <KAboutData>
#include <KAuthorized>
#include <KPluginFactory>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(KateExternalToolsFactory, "externaltoolsplugin.json",
                           registerPlugin<KateExternalToolsPlugin>();)

KateExternalToolsPlugin::KateExternalToolsPlugin(QObject* parent, const QList<QVariant>&)
    : KTextEditor::Plugin(parent)
{
    reload();
}

KateExternalToolsPlugin::~KateExternalToolsPlugin()
{
    delete m_command;
    m_command = nullptr;
}

QObject* KateExternalToolsPlugin::createView(KTextEditor::MainWindow* mainWindow)
{
    KateExternalToolsPluginView* view = new KateExternalToolsPluginView(mainWindow, this);
    connect(view, SIGNAL(destroyed(QObject*)), this, SLOT(viewDestroyed(QObject*)));
    m_views.append(view);
    return view;
}

KateExternalToolsPluginView* KateExternalToolsPlugin::extView(QWidget* widget)
{
    foreach (KateExternalToolsPluginView* view, m_views) {
        if (view->mainWindow()->window() == widget)
            return view;
    }
    return nullptr;
}

void KateExternalToolsPlugin::viewDestroyed(QObject* view)
{
    m_views.removeAll(dynamic_cast<KateExternalToolsPluginView*>(view));
}

void KateExternalToolsPlugin::reload()
{
    m_commands.clear();

    KConfig _config(QStringLiteral("externaltools"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);
    KConfigGroup config(&_config, "Global");
    const QStringList tools = config.readEntry("tools", QStringList());

    for (QStringList::const_iterator it = tools.begin(); it != tools.end(); ++it) {
        if (*it == QStringLiteral("---"))
            continue;

        config = KConfigGroup(&_config, *it);

        KateExternalTool t;
        t.load(config);
        m_tools.push_back(t);

        // FIXME test for a command name first!
        if (t.hasexec && (!t.cmdname.isEmpty())) {
            m_commands.push_back(t.cmdname);
        }
    }

    if (KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
        delete m_command;
        m_command = new KateExternalToolsCommand(this);
    }
    foreach (KateExternalToolsPluginView* view, m_views) {
        view->rebuildMenu();
    }
}

QStringList KateExternalToolsPlugin::commands() const
{
    return m_commands;
}

const KateExternalTool * KateExternalToolsPlugin::toolForCommand(const QString & cmd) const
{
    for (auto & tool : m_tools) {
        if (tool.cmdname == cmd) {
            return &tool;
        }
    }
    return nullptr;
}

void KateExternalToolsPlugin::runTool(const KateExternalTool & tool, KTextEditor::View * view)
{
    // expand the macros in command if any,
    // and construct a command with an absolute path
    auto mw = view->mainWindow();

    // save documents if requested
    if (tool.saveMode == KateExternalTool::SaveMode::CurrentDocument) {
        view->document()->save();
    } else if (tool.saveMode == KateExternalTool::SaveMode::AllDocuments) {
        foreach (KXMLGUIClient* client, mw->guiFactory()->clients()) {
            if (QAction* a = client->actionCollection()->action(QStringLiteral("file_save_all"))) {
                a->trigger();
                break;
            }
        }
    }

    // copy tool
    auto copy = new KateExternalTool(tool);

    MacroExpander macroExpander(view);
    if (!macroExpander.expandMacrosShellQuote(copy->arguments)) {
        KMessageBox::sorry(view, i18n("Failed to expand the arguments '%1'.", copy->arguments), i18n("Kate External Tools"));
        return;
    }

    if (!macroExpander.expandMacrosShellQuote(copy->workingDir)) {
        KMessageBox::sorry(view, i18n("Failed to expand the working directory '%1'.", copy->workingDir), i18n("Kate External Tools"));
        return;
    }

    // FIXME: The tool runner must live as long as the child process is running.
    //        --> it must be allocated on the heap, and deleted with a ->deleteLater() call.
    KateToolRunner runner(copy);
    runner.run();
    runner.waitForFinished();
}

int KateExternalToolsPlugin::configPages() const
{
    return 1;
}

KTextEditor::ConfigPage* KateExternalToolsPlugin::configPage(int number, QWidget* parent)
{
    if (number == 0) {
        return new KateExternalToolsConfigWidget(parent, this);
    }
    return nullptr;
}

KateExternalToolsPluginView::KateExternalToolsPluginView(KTextEditor::MainWindow* mainWindow, KateExternalToolsPlugin* plugin)
    : QObject(mainWindow)
    , m_plugin(plugin)
    , m_mainWindow(mainWindow)
{
    KXMLGUIClient::setComponentName(QLatin1String("externaltools"), i18n("External Tools"));
    setXMLFile(QLatin1String("ui.rc"));

    if (KAuthorized::authorizeAction(QStringLiteral("shell_access"))) {
        externalTools
            = new KateExternalToolsMenuAction(i18n("External Tools"), actionCollection(), plugin, mainWindow);
        actionCollection()->addAction(QStringLiteral("tools_external"), externalTools);
        externalTools->setWhatsThis(i18n("Launch external helper applications"));
    }

    mainWindow->guiFactory()->addClient(this);
}

void KateExternalToolsPluginView::rebuildMenu()
{
    if (externalTools) {
        KXMLGUIFactory* f = factory();
        f->removeClient(this);
        reloadXML();
        externalTools->reload();
        qDebug() << "has just returned from externalTools->reload()";
        f->addClient(this);
    }
}

KateExternalToolsPluginView::~KateExternalToolsPluginView()
{
    m_mainWindow->guiFactory()->removeClient(this);

    delete externalTools;
    externalTools = nullptr;
}

KTextEditor::MainWindow* KateExternalToolsPluginView::mainWindow() const
{
    return m_mainWindow;
}

#include "externaltoolsplugin.moc"

// kate: space-indent on; indent-width 4; replace-tabs on;
