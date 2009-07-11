/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002,2005,2009 Joseph Wenninger <jowenn@kde.org>
   
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
#include "plugin.h"

#include "application.h"
#include "mainwindow.h"

#include "plugin.moc"

#include <kparts/componentfactory.h>
#include <klibloader.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include <qdir.h>

namespace Kate
{

  class PrivatePlugin
  {
    public:
      PrivatePlugin ()
      {}

      ~PrivatePlugin ()
      {}
  };

  class PrivatePluginView
  {
    public:
      PrivatePluginView ()
      {}

      ~PrivatePluginView ()
      {}

      MainWindow *mainWindow;
  };

  Plugin::Plugin( Application *application, const char *name ) : QObject (application )
  {
    setObjectName( name );
  }

  Plugin::~Plugin()
  {}

  Plugin *createPlugin ( const char* libname, Application *application,
                         const QStringList &args )
  {
    return KLibLoader::createInstance<Plugin>( libname, application, args );
  }

  Application *Plugin::application () const
  {
    return Kate::application();
  }

  PluginView *Plugin::createView (MainWindow *)
  {
    return 0;
  }

  void Plugin::readSessionConfig (KConfigBase*, const QString&)
  {}

  void Plugin::writeSessionConfig (KConfigBase*, const QString&)
  {}

  PluginView::PluginView (MainWindow *mainWindow)
      : QObject (mainWindow), d (new PrivatePluginView ())
  {
    // remember mainWindow of this view...
    d->mainWindow = mainWindow;
  }

  PluginView::~PluginView ()
  {
    delete d;
  }

  MainWindow *PluginView::mainWindow() const
  {
    return d->mainWindow;
  }

  void PluginView::readSessionConfig (KConfigBase*, const QString&)
  {}

  void PluginView::writeSessionConfig (KConfigBase*, const QString&)
  {}



  XMLGUIClient::XMLGUIClient(const KComponentData &componentData)
    : KXMLGUIClient()
  {
    setComponentData (componentData);
    setXMLFile( xmlDataFile( componentData, "ui.rc" ));
    setLocalXMLFile( localXmlDataFile( componentData, "ui.rc" ));
  }

  QString XMLGUIClient::xmlDataFile(const KComponentData &componentData, const QString &filename)
  {
    const QString filter = "kate/plugins/" + componentData.componentName() + '/' + filename;
    const QStringList allFiles = KGlobal::dirs()->findAllResources("data", filter);
    QString file;
    QString doc;
    if (!allFiles.isEmpty())
        file = KXMLGUIClient::findMostRecentXMLFile(allFiles, doc);
    return file;
  }
  
  QString XMLGUIClient::localXmlDataFile(const KComponentData &componentData, const QString &filename)
  {
    QString result = KStandardDirs::locateLocal( "data", "kate/plugins/" + componentData.componentName() + QDir::separator() + filename);
    kDebug(13000) << "File for shortcut storage" << result;
    return result;
  }

}

// kate: space-indent on; indent-width 2; replace-tabs on;

