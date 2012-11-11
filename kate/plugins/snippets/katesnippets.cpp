/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#include "katesnippets.h"
#include "katesnippets.moc"

#include <kicon.h>
#include <kiconloader.h>
#include <ktexteditor/editor.h>

#include <KToolBar>

#include <kurl.h>
#include <klibloader.h>
#include <klocale.h>

#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kpluginfactory.h>

K_PLUGIN_FACTORY(KateSnippetsFactory, registerPlugin<KateSnippetsPlugin>();)
K_EXPORT_PLUGIN(KateSnippetsFactory(KAboutData("katesnippets","katesnippetsplugin",ki18n("Snippets"), "0.1", ki18n("Embedded Snippets"), KAboutData::License_LGPL_V2)) )

KateSnippetsPlugin::KateSnippetsPlugin( QObject* parent, const QList<QVariant>& ):
    Kate::Plugin ( (Kate::Application*)parent )
{
}

KateSnippetsPlugin::~KateSnippetsPlugin()
{
}

Kate::PluginView *KateSnippetsPlugin::createView (Kate::MainWindow *mainWindow)
{
  KateSnippetsPluginView *view = new KateSnippetsPluginView (this, mainWindow);
  return view;
}

KateSnippetsPluginView::KateSnippetsPluginView (KateSnippetsPlugin* plugin, Kate::MainWindow *mainWindow)
    : Kate::PluginView (mainWindow), m_plugin(plugin), m_toolView (0), m_snippets(0)
{
  // use snippets widget provided by editor component, if any
  if ((m_snippets = Kate::application()->editor()->property("snippetWidget").value<QWidget*>())) {
    // Toolview for snippets
    m_toolView = mainWindow->createToolView (0,"kate_private_plugin_katesnippetsplugin", Kate::MainWindow::Right, SmallIcon("document-new"), i18n("Snippets"));
    
    // snippets toolbar
    KToolBar *topToolbar = new KToolBar (m_toolView, "snippetsToolBar");
    topToolbar->setToolButtonStyle (Qt::ToolButtonIconOnly);
    topToolbar->addActions (m_snippets->actions());

    // add snippets widget
    m_snippets->setParent (m_toolView);
  }
  
  // register this view
  m_plugin->mViews.append ( this );
}

KateSnippetsPluginView::~KateSnippetsPluginView ()
{
  // unregister this view
  m_plugin->mViews.removeAll (this);
  
  // cleanup, kill toolview
  delete m_snippets;
  delete m_toolView;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
