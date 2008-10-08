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
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kio/netaccess.h>
#include <kactioncollection.h>


K_PLUGIN_FACTORY(KateOpenHeaderFactory, registerPlugin<PluginKateOpenHeader>();)
K_EXPORT_PLUGIN(KateOpenHeaderFactory(KAboutData("kateopenheader","kateopenheader",ki18n("Open Header"), "0.1", ki18n("Open header for a source file"), KAboutData::License_LGPL_V2)) )


PluginViewKateOpenHeader::PluginViewKateOpenHeader(PluginKateOpenHeader *plugin,Kate::MainWindow *mainwindow): Kate::PluginView(mainwindow),KXMLGUIClient()
{
    setComponentData (KateOpenHeaderFactory::componentData());
    setXMLFile( "plugins/kateopenheader/ui.rc" );
    QAction *a = actionCollection()->addAction("file_openheader");
    a->setText(i18n("Open .h/.cpp/.c"));
    a->setShortcut( Qt::Key_F12 );
    connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( slotOpenHeader() ) );

    mainwindow->guiFactory()->addClient (this);
}

PluginViewKateOpenHeader::~PluginViewKateOpenHeader()
{
      mainWindow()->guiFactory()->removeClient (this);

}


PluginKateOpenHeader::PluginKateOpenHeader( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin ( (Kate::Application *)parent, "open-header-plugin" )
{
}

PluginKateOpenHeader::~PluginKateOpenHeader()
{
}

Kate::PluginView *PluginKateOpenHeader::createView (Kate::MainWindow *mainWindow)
{
    return new PluginViewKateOpenHeader(this,mainWindow);
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

  kDebug() << "Trying to open " << url.prettyUrl() << " with extensions " << extensions.join(" ");
  QString basename = QFileInfo( url.path() ).baseName();
  KUrl newURL( url );
  for( QStringList::ConstIterator it = extensions.begin(); it != extensions.end(); ++it ) {
    newURL.setFileName( basename + '.' + *it );
    if( KIO::NetAccess::exists( newURL , KIO::NetAccess::SourceSide, application()->activeMainWindow()->window()) )
      application()->activeMainWindow()->openUrl( newURL );
    newURL.setFileName( basename + '.' + (*it).toUpper() );
    if( KIO::NetAccess::exists( newURL , KIO::NetAccess::SourceSide, application()->activeMainWindow()->window()) )
      application()->activeMainWindow()->openUrl( newURL );
  }
}
