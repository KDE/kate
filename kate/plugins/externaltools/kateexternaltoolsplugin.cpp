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

#include <kparts/part.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <kurl.h>
#include <klibloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <KToolInvocation>

#include <kgenericfactory.h>
#include <kauthorized.h>

K_EXPORT_COMPONENT_FACTORY( kateexternaltoolsplugin, KGenericFactory<KateExternalToolsPlugin>( "KateExternalToolsPlugin" ) )

KateExternalToolsPlugin::KateExternalToolsPlugin( QObject* parent, const QStringList& ):
    Kate::Plugin ( (Kate::Application*)parent )
{}

Kate::PluginView *KateExternalToolsPlugin::createView (Kate::MainWindow *mainWindow)
{
  return new KateExternalToolsPluginView (mainWindow);
}

KateExternalToolsPluginView::KateExternalToolsPluginView (Kate::MainWindow *mainWindow)
    : Kate::PluginView (mainWindow)
{
  /*    actionCollection()->addAction( KStandardAction::Mail, this, SLOT(slotMail()) )
        ->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));
      setInstance (new KInstance("kate"));
      setXMLFile("plugins/kateexternaltools/ui.rc");
      mainWindow->guiFactory()->addClient (this);*/
}

KateExternalToolsPluginView::~KateExternalToolsPluginView ()
{}

/*
  if ( KAuthorized::authorize("shell_access") )
  {
    KTextEditor::CommandInterface* cmdIface =
        qobject_cast<KTextEditor::CommandInterface*>( KateDocManager::self()->editor() );
    if( cmdIface )
      cmdIface->registerCommand( KateExternalToolsCommand::self() );
  }
 
    if ( KAuthorized::authorize("shell_access") )
  {
    externalTools = new KateExternalToolsMenuAction( i18n("External Tools"), this, this );
    actionCollection()->addAction( "tools_external", externalTools );
    externalTools->setWhatsThis( i18n("Launch external helper applications") );
  }
  */
// kate: space-indent on; indent-width 2; replace-tabs on;

