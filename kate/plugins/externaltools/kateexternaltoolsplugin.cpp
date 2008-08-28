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
#include "kateexternaltoolsplugin.moc"

#include <kicon.h>
#include <kiconloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/editor.h>
#include <kate/application.h>

#include <kparts/part.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <kurl.h>
#include <klibloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <kgenericfactory.h>
#include <kauthorized.h>

K_EXPORT_COMPONENT_FACTORY( kateexternaltoolsplugin, KGenericFactory<KateExternalToolsPlugin>( "kateexternaltoolsplugin" ) )

KateExternalToolsPlugin::KateExternalToolsPlugin( QObject* parent, const QStringList& ):
    Kate::Plugin ( (Kate::Application*)parent )
{}

Kate::PluginView *KateExternalToolsPlugin::createView (Kate::MainWindow *mainWindow)
{
  return new KateExternalToolsPluginView (mainWindow);
}

uint KateExternalToolsPlugin::configPages() const
{
  return 1;
}

Kate::PluginConfigPage *KateExternalToolsPlugin::configPage (uint number, QWidget *parent, const char *name )
{
  if (number == 0) {
    return new KateExternalToolsConfigWidget(parent, name);
  }
  return 0;
}

QString KateExternalToolsPlugin::configPageName (uint number) const
{
  if (number == 0) {
    return i18n("External Tools");
  }
  return QString();
}

QString KateExternalToolsPlugin::configPageFullName (uint number) const
{
  if (number == 0) {
    return i18n("External Tools");
  }
  return QString();
}

KIcon KateExternalToolsPlugin::configPageIcon (uint number) const
{
  if (number == 0) {
    return KIcon();
  }
  return KIcon();
}


KateExternalToolsPluginView::KateExternalToolsPluginView (Kate::MainWindow *mainWindow)
    : Kate::PluginView (mainWindow), KXMLGUIClient()
{
  externalTools = 0;

  if (KAuthorized::authorizeKAction("shell_access"))
  {
    KTextEditor::CommandInterface* cmdIface =
      qobject_cast<KTextEditor::CommandInterface*>( Kate::application()->editor() );
    if( cmdIface )
      cmdIface->registerCommand( KateExternalToolsCommand::self() );

    externalTools = new KateExternalToolsMenuAction( i18n("External Tools"), actionCollection(), mainWindow, mainWindow );
    actionCollection()->addAction("tools_external", externalTools);
    externalTools->setWhatsThis( i18n("Launch external helper applications") );

    setComponentData(KComponentData("kate"));
    setXMLFile("plugins/kateexternaltools/ui.rc");
  }

  mainWindow->guiFactory()->addClient (this);
}

KateExternalToolsPluginView::~KateExternalToolsPluginView ()
{
  mainWindow()->guiFactory()->removeClient (this);

  delete externalTools;
  externalTools = 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
