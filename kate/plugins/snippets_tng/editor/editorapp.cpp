/* This file is part of the KDE project
 * Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "editorapp.h"
#include "editorapp.moc"
#include "snippeteditorwindow.h"
#include <kwindowsystem.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <qdir.h>
#include <ktexteditor/editorchooser.h>

EditorApp::EditorApp(): KUniqueApplication(),m_first(true)
{
}

EditorApp::~EditorApp()
{
}

int EditorApp::newInstance() {
  KCmdLineArgs::setCwd(QDir::currentPath().toUtf8());
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  if (args->count() > 0)
  {
    for (int i = 0; i < args->count(); ++i)
    {
      //openWindow(args->url(i));
      KUrl u=args->url(i);
      if (!u.isLocalFile()) {
        KMessageBox::error(0,
                          i18n("The specified URL (%1) is not a local file",args->url(i).prettyUrl()),
                          "URL not supported");
        continue;
      }
      if (openWindow(u))
        m_first=false;            
    }
    if (m_first) quit();
  } else {
    //no session restoration yet and no stand alone starting
    quit();
  }
  args->clear();  
  return 0;
}

bool EditorApp::openWindow(const KUrl& url)
{
  if (m_urlWindowMap.contains(url)) {
    KWindowSystem::raiseWindow(m_urlWindowMap[url]->effectiveWinId());
    return true;
  } else {
    SnippetEditorWindow *w=new SnippetEditorWindow(modes(),url);
    m_urlWindowMap.insert(url,w);
    w->show();
    return true;
  }

}

const QStringList& EditorApp::modes() {
  if (!m_modes.isEmpty()) return m_modes;
  KTextEditor::Editor *editor = KTextEditor::EditorChooser::editor();
  KTextEditor::Document *document=editor->createDocument(0);
  m_modes=document->modes();
  m_modes.sort();
  delete document;
  delete editor;
  return m_modes;
}

// kate: space-indent on; indent-width 2; replace-tabs on;