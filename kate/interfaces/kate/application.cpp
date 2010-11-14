/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
 
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

#include "application.h"
#include "application.moc"

#include "documentmanager.h"
#include "mainwindow.h"
#include "pluginmanager.h"

#include "../app/kateapp.h"
#include "../app/katedocmanager.h"
#include "../app/katepluginmanager.h"
#include "../app/katemainwindow.h"
#include <ktexteditor/editor.h>

namespace Kate
{

  class PrivateApplication
  {
    public:
      PrivateApplication ()
      {}

      ~PrivateApplication ()
      {
      }

      KateApp *app;
  };

  Application::Application (void *application) : QObject ((KateApp *) application)
  {
    d = new PrivateApplication;
    d->app = (KateApp *) application;
  }

  Application::~Application ()
  {
    delete d;
  }

  DocumentManager *Application::documentManager ()
  {
    return d->app->documentManager ()->documentManager ();
  }

  PluginManager *Application::pluginManager ()
  {
    return d->app->pluginManager ()->pluginManager ();
  }

  MainWindow *Application::activeMainWindow ()
  {
    if (!d->app->activeMainWindow())
      return 0;

    return d->app->activeMainWindow()->mainWindow();
  }

  const QList<MainWindow*> &Application::mainWindows () const
  {
    return d->app->mainWindowsInterfaces ();
  }

  Application *application ()
  {
    return KateApp::self()->application ();
  }

  KTextEditor::Editor *Application::editor ()
  {
    return d->app->documentManager ()->editor();
  }


}

// kate: space-indent on; indent-width 2; replace-tabs on;

