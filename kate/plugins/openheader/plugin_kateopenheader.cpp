/* This file is part of the KDE project
   Copyright (C) 2001 Joseph Wenninger
   Copyright (C) 2009 Erlend Hamberg <ehamberg@gmail.com>

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

#include "plugin_kateopenheader.h"
#include "plugin_kateopenheader.moc"

#include <kate/application.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>

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


PluginViewKateOpenHeader::PluginViewKateOpenHeader(PluginKateOpenHeader *plugin, Kate::MainWindow *mainwindow)
: Kate::PluginView(mainwindow), KXMLGUIClient(), KTextEditor::Command(), m_plugin(plugin)
{
    KAction *a = actionCollection()->addAction("file_openheader");
    a->setText(i18n("Open .h/.cpp/.c"));
    a->setShortcut( Qt::Key_F12 );
    connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( slotOpenHeader() ) );

    mainwindow->guiFactory()->addClient (this);

    KTextEditor::CommandInterface* cmdIface =
      qobject_cast<KTextEditor::CommandInterface*>( Kate::application()->editor() );

    if( cmdIface ) {
        cmdIface->registerCommand( this );
    }
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

  QFileInfo info( url.toLocalFile() );
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

const QStringList& PluginViewKateOpenHeader::cmds()
{
    static QStringList l;

    if (l.empty()) {
        l << "toggle-header";
    }

    return l;
}

bool PluginViewKateOpenHeader::exec(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    Q_UNUSED(view)
    Q_UNUSED(cmd)
    Q_UNUSED(msg)

    m_plugin->slotOpenHeader();
    return true;
}

bool PluginViewKateOpenHeader::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    Q_UNUSED(view)
    Q_UNUSED(cmd)

    msg = "<p><b>toggle-header &mdash; switch between header and corresponding c/cpp file</b></p>"
            "<p>usage: <tt><b>toggle-header</b></tt></p>"
            "<p>When editing C or C++ code, this command will switch between a header file and "
            "its corresponding C/C++ file or vice verca.</p>"
            "<p>For example, if you are editing myclass.cpp, <tt>toggle-header</tt> will change "
            "to myclass.h if this file is available.</p>"
            "<p>Pairs of the following filename suffixes will work:<br />"
            " Header files: h, H, hh, hpp<br />"
            " Source files: c, cpp, cc, cp, cxx</p>";

    return true;
}
