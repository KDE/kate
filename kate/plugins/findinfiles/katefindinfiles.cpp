/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Dominik Haumann <dhaumann@kde.org>

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

#include "katefindinfiles.h"
#include "katefindinfiles.moc"

#include <kicon.h>
#include <kiconloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kde_terminal_interface.h>

#include <kparts/part.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <kauthorized.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>

K_EXPORT_COMPONENT_FACTORY( katefindinfilesplugin, KGenericFactory<KateFindInFilesPlugin>( "KateFindInFilesPlugin" ) )

KateFindInFilesPlugin::KateFindInFilesPlugin( QObject* parent, const QStringList& ):
    Kate::Plugin ( (Kate::Application*)parent )
{
  if (!KAuthorized::authorizeKAction("shell_access"))
  {
    KMessageBox::sorry(0, i18n ("You do not have enough karma to access a shell or terminal emulation"));
  }
}

Kate::PluginView *KateFindInFilesPlugin::createView (Kate::MainWindow *mainWindow)
{
  return new KateFindInFilesView (mainWindow);
}

/*
 * Construct the view, toolview + grepdialog
 */
KateFindInFilesView::KateFindInFilesView (Kate::MainWindow *mw)
    : Kate::PluginView (mw)
    , m_toolView (mw->createToolView ("kate_private_plugin_katefindinfilesplugin", Kate::MainWindow::Bottom, SmallIcon("konsole"), i18n("Find in Files")))
    , m_grepDialog (new KateGrepDialog (m_toolView, mw))
{}

/*
 * delete toolview and children again...
 */
KateFindInFilesView::~KateFindInFilesView ()
{
  delete m_toolView;
}

void KateFindInFilesView::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  m_grepDialog->readSessionConfig(KConfigGroup(config, groupPrefix + ":find-in-files"));
}

void KateFindInFilesView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  KConfigGroup cg(config, groupPrefix + ":find-in-files");
  m_grepDialog->writeSessionConfig(cg);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
