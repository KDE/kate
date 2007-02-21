/***************************************************************************
                          plugin_katetextfilter.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugin_kateopenheader.h"
#include "plugin_kateopenheader.moc"

#include <kate/application.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <QFileInfo>
#include <kgenericfactory.h>
#include <kaction.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kio/netaccess.h>
#include <kactioncollection.h>
class PluginView : public KXMLGUIClient
{
  friend class PluginKateOpenHeader;

  public:
    Kate::MainWindow *win;
};

K_EXPORT_COMPONENT_FACTORY( kateopenheaderplugin, KGenericFactory<PluginKateOpenHeader>( "kateopenheader" ) )

PluginKateOpenHeader::PluginKateOpenHeader( QObject* parent, const QStringList& )
    : Kate::Plugin ( (Kate::Application *)parent, "open-header-plugin" )
{
}

PluginKateOpenHeader::~PluginKateOpenHeader()
{
}

void PluginKateOpenHeader::addView(Kate::MainWindow *win)
{
    PluginView *view = new PluginView ();

    QAction *a = view->actionCollection()->addAction("file_openheader");
    a->setText(i18n("Open .h/.cpp/.c"));
    a->setShortcut( Qt::Key_F12 );
    connect( a, SIGNAL( triggered(bool) ), this, SLOT( slotOpenHeader() ) );

    view->setComponentData (KComponentData("kate"));
    view->setXMLFile( "plugins/kateopenheader/ui.rc" );
    win->guiFactory()->addClient (view);
    view->win = win;

   m_views.append (view);
}

void PluginKateOpenHeader::removeView(Kate::MainWindow *win)
{
  for (int z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginView *view = m_views.at(z);
      m_views.removeAll (view);
      win->guiFactory()->removeClient (view);
      delete view;
    }
}

void PluginKateOpenHeader::storeGeneralConfig(KConfig* config,const QString& groupPrefix)
{
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void PluginKateOpenHeader::loadGeneralConfig(KConfig* config,const QString& groupPrefix)
{
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void PluginKateOpenHeader::storeViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix)
{
  Q_UNUSED( config );
  Q_UNUSED( mainwindow );
  Q_UNUSED( groupPrefix );
}

void PluginKateOpenHeader::loadViewConfig(KConfig* config, Kate::MainWindow* mainwindow, const QString& groupPrefix)
{
  Q_UNUSED( config );
  Q_UNUSED( mainwindow );
  Q_UNUSED( groupPrefix );
}

void PluginKateOpenHeader::slotOpenHeader ()
{
  if (!application()->activeMainWindow())
    return;

  KTextEditor::View * kv (application()->activeMainWindow()->activeView());
  if (!kv) return;

  KUrl url=kv->document()->url();
  if ((!url.isValid()) || (url.isEmpty())) return;

  QFileInfo info( url.path() );
  QString extension = info.suffix().toLower();

  QStringList headers( QStringList() << "h" << "H" << "hh" << "hpp" );
  QStringList sources( QStringList() << "c" << "cpp" << "cc" << "cp" << "cxx" );

  if( sources.contains( extension ) ) {
    tryOpen( url, headers );
  } else if ( headers.contains( extension ) ) {
    tryOpen( url, sources );
  }
}

void PluginKateOpenHeader::tryOpen( const KUrl& url, const QStringList& extensions )
{
  if (!application()->activeMainWindow())
    return;

  kDebug() << "Trying to open " << url.prettyUrl() << " with extensions " << extensions.join(" ") << endl;
  QString basename = QFileInfo( url.path() ).baseName();
  KUrl newURL( url );
  for( QStringList::ConstIterator it = extensions.begin(); it != extensions.end(); ++it ) {
    newURL.setFileName( basename + "." + *it );
    if( KIO::NetAccess::exists( newURL , true, application()->activeMainWindow()->window()) )
      application()->activeMainWindow()->openUrl( newURL );
    newURL.setFileName( basename + "." + (*it).toUpper() );
    if( KIO::NetAccess::exists( newURL , true, application()->activeMainWindow()->window()) )
      application()->activeMainWindow()->openUrl( newURL );
  }
}
