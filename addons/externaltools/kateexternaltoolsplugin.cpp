/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#include "kateexternaltoolsplugin.h"

#include <kate/application.h>
#include <kicon.h>
#include <kiconloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <kparts/part.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>

#include <KAboutData.h>
#include <KAuthorized>
#include <KPluginFactory>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(KateExternalToolsFactory, "externaltoolsplugin.json", registerPlugin<KateExternalToolsPlugin>();)

KateExternalToolsPlugin::KateExternalToolsPlugin(QObject* parent, const QList<QVariant>&)
    : KTextEditor::Plugin(parent)
    , m_command(0)
{
    if (KAuthorized::authorizeKAction("shell_access")) {
        KTextEditor::CommandInterface* cmdIface
            = qobject_cast<KTextEditor::CommandInterface*>(Kate::application()->editor());
        if (cmdIface) {
            m_command = new KateExternalToolsCommand(this);
            cmdIface->registerCommand(m_command);
        }
    }
}

KateExternalToolsPlugin::~KateExternalToolsPlugin()
{
    if (KAuthorized::authorizeKAction("shell_access")) {
        if (m_command) {
            KTextEditor::CommandInterface* cmdIface
                = qobject_cast<KTextEditor::CommandInterface*>(Kate::application()->editor());
            if (cmdIface)
                cmdIface->unregisterCommand(m_command);
            delete m_command;
        }
    }
}

Kate::PluginView* KateExternalToolsPlugin::createView(Kate::MainWindow* mainWindow)
{
    KateExternalToolsPluginView* view = new KateExternalToolsPluginView(mainWindow);
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
    return 0;
}

void KateExternalToolsPlugin::viewDestroyed(QObject* view)
{
    m_views.removeAll(dynamic_cast<KateExternalToolsPluginView*>(view));
}

void KateExternalToolsPlugin::reload()
{
    if (KAuthorized::authorizeKAction("shell_access")) {
        KTextEditor::CommandInterface* cmdIface
            = qobject_cast<KTextEditor::CommandInterface*>(Kate::application()->editor());
        if (cmdIface)
            if (m_command)
                m_command->reload();
    }
    foreach (KateExternalToolsPluginView* view, m_views) {
        view->rebuildMenu();
    }
}

uint KateExternalToolsPlugin::configPages() const
{
    return 1;
}

Kate::PluginConfigPage* KateExternalToolsPlugin::configPage(uint number, QWidget* parent, const char* name)
{
    if (number == 0) {
        return new KateExternalToolsConfigWidget(parent, this, name);
    }
    return 0;
}

QString KateExternalToolsPlugin::configPageName(uint number) const
{
    if (number == 0) {
        return i18n("External Tools");
    }
    return QString();
}

QString KateExternalToolsPlugin::configPageFullName(uint number) const
{
    if (number == 0) {
        return i18n("External Tools");
    }
    return QString();
}

KIcon KateExternalToolsPlugin::configPageIcon(uint number) const
{
    if (number == 0) {
        return KIcon();
    }
    return KIcon();
}

KateExternalToolsPluginView::KateExternalToolsPluginView(Kate::MainWindow* mainWindow)
    : Kate::PluginView(mainWindow)
    , Kate::XMLGUIClient(KateExternalToolsFactory::componentData())
{
    externalTools = 0;

    if (KAuthorized::authorizeKAction("shell_access")) {
        externalTools
            = new KateExternalToolsMenuAction(i18n("External Tools"), actionCollection(), mainWindow, mainWindow);
        actionCollection()->addAction("tools_external", externalTools);
        externalTools->setWhatsThis(i18n("Launch external helper applications"));
    }

    mainWindow->guiFactory()->addClient(this);
}

void KateExternalToolsPluginView::rebuildMenu()
{
    kDebug(13001);
    if (externalTools) {
        KXMLGUIFactory* f = factory();
        f->removeClient(this);
        reloadXML();
        externalTools->reload();
        kDebug(13001) << "has just returned from externalTools->reload()";
        f->addClient(this);
    }
}

KateExternalToolsPluginView::~KateExternalToolsPluginView()
{
    mainWindow()->guiFactory()->removeClient(this);

    delete externalTools;
    externalTools = 0;
}

#include "kateexternaltoolsplugin.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
