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

#include <KIconLoader>
#include <KToolBar>
#include <KLocalizedString>
#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON (KateSnippetsPluginFactory, "katesnippetsplugin.json", registerPlugin<KateSnippetsPlugin>();)

KateSnippetsPlugin::KateSnippetsPlugin( QObject* parent, const QList<QVariant>& ):
    KTextEditor::ApplicationPlugin ( parent )
{
}

KateSnippetsPlugin::~KateSnippetsPlugin()
{
}

QObject *KateSnippetsPlugin::createView (KTextEditor::MainWindow *mainWindow)
{
  KateSnippetsPluginView *view = new KateSnippetsPluginView (this, mainWindow);
  return view;
}

KateSnippetsPluginView::KateSnippetsPluginView (KateSnippetsPlugin* plugin, KTextEditor::MainWindow *mainWindow)
    : QObject (mainWindow), m_plugin(plugin), m_toolView (0), m_snippets(0)
{
  // use snippets widget provided by editor component, if any
  if ((m_snippets = KTextEditor::Editor::instance()->property("snippetWidget").value<QWidget*>())) {
    // Toolview for snippets
    m_toolView = mainWindow->createToolView (0,"kate_private_plugin_katesnippetsplugin", KTextEditor::MainWindow::Right, SmallIcon("document-new"), i18n("Snippets"));
    
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

#include "katesnippets.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
