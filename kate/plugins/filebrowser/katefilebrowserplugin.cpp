/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

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

//BEGIN Includes
#include "katefilebrowserplugin.h"
#include "katefilebrowserplugin.moc"
#include "katefilebrowserconfig.h"
#include "katefilebrowser.h"

#include <kate/mainwindow.h>
#include <ktexteditor/view.h>

#include <kaboutdata.h>
#include <kpluginfactory.h>
//END Includes

K_PLUGIN_FACTORY(KateFileBrowserFactory, registerPlugin<KateFileBrowserPlugin>();)
K_EXPORT_PLUGIN(KateFileBrowserFactory(KAboutData("katefilebrowserplugin","katefilebrowserplugin",ki18n("Filesystem Browser"), "0.1", ki18n("Browse through the filesystem"), KAboutData::License_LGPL_V2)) )

//BEGIN KateFileBrowserPlugin
KateFileBrowserPlugin::KateFileBrowserPlugin(QObject* parent, const QList<QVariant>&)
  : Kate::Plugin ((Kate::Application*)parent)
{
}

Kate::PluginView *KateFileBrowserPlugin::createView (Kate::MainWindow *mainWindow)
{
  KateFileBrowserPluginView* kateFileSelectorPluginView = new KateFileBrowserPluginView (mainWindow);
  m_fileBrowser = kateFileSelectorPluginView->m_fileBrowser;
  return kateFileSelectorPluginView;
}

uint KateFileBrowserPlugin::configPages() const
{
  return 1;
}

Kate::PluginConfigPage *KateFileBrowserPlugin::configPage (uint number, QWidget *parent, const char *name)
{
  if (number != 0)
    return 0;
  return new KateFileBrowserConfigPage(parent, name, m_fileBrowser);
}

QString KateFileBrowserPlugin::configPageName (uint number) const
{
  if (number != 0) return QString();
  kDebug() << "Returning a config page name";
  return i18n("Filesystem Browser");
}

QString KateFileBrowserPlugin::configPageFullName (uint number) const
{
  if (number != 0) return QString();
  return i18n("Filesystem Browser Settings");
}

KIcon KateFileBrowserPlugin::configPageIcon (uint number) const
{
  if (number != 0) return KIcon();
  return KIcon("document-open");
}
//END KateFileBrowserPlugin




//BEGIN KateFileBrowserPluginView
KateFileBrowserPluginView::KateFileBrowserPluginView (Kate::MainWindow *mainWindow)
: Kate::PluginView (mainWindow)
{
  // init console
  QWidget *toolview = mainWindow->createToolView ("kate_private_plugin_katefileselectorplugin", Kate::MainWindow::Left, SmallIcon("document-open"), i18n("Filesystem Browser"));
  m_fileBrowser = new KateFileBrowser(mainWindow, toolview);
}

KateFileBrowserPluginView::~KateFileBrowserPluginView ()
{
  // cleanup, kill toolview + console
  delete m_fileBrowser->parentWidget();
}

void KateFileBrowserPluginView::readSessionConfig(KConfigBase* config, const QString& group)
{
  m_fileBrowser->readSessionConfig(config, group);
}

void KateFileBrowserPluginView::writeSessionConfig(KConfigBase* config, const QString& group)
{
  m_fileBrowser->writeSessionConfig(config, group);
}
//ENDKateFileBrowserPluginView


// kate: space-indent on; indent-width 2; replace-tabs on;
