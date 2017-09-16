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

#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/application.h>

#include <QFileInfo>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <QAction>
#include <klocalizedstring.h>
#include <kactioncollection.h>
#include <KXMLGUIFactory>
#include <KJobWidgets>
#include <KIO/StatJob>
#include <QDir>


K_PLUGIN_FACTORY_WITH_JSON(KateOpenHeaderFactory,"kateopenheaderplugin.json", registerPlugin<PluginKateOpenHeader>();)
//K_EXPORT_PLUGIN(KateOpenHeaderFactory(KAboutData("kateopenheader","kateopenheader",ki18n("Open Header"), "0.1", ki18n("Open header for a source file"), KAboutData::License_LGPL_V2)) )


PluginViewKateOpenHeader::PluginViewKateOpenHeader(PluginKateOpenHeader *plugin, KTextEditor::MainWindow *mainwindow)
  : KTextEditor::Command(QStringList() << QStringLiteral("toggle-header"), mainwindow)
  , KXMLGUIClient()
  , m_plugin(plugin)
  , m_mainWindow(mainwindow)
{
    KXMLGUIClient::setComponentName (QStringLiteral("kateopenheaderplugin"), i18n ("Open Header"));
    setXMLFile( QStringLiteral("ui.rc") );
    QAction *a = actionCollection()->addAction(QStringLiteral("file_openheader"));
    a->setText(i18n("Open .h/.cpp/.c"));
    actionCollection()->setDefaultShortcut(a, Qt::Key_F12 );
    connect(a, &QAction::triggered, plugin, &PluginKateOpenHeader::slotOpenHeader);
    mainwindow->guiFactory()->addClient (this);
}

PluginViewKateOpenHeader::~PluginViewKateOpenHeader()
{
      m_mainWindow->guiFactory()->removeClient (this);
}

PluginKateOpenHeader::PluginKateOpenHeader( QObject* parent, const QList<QVariant>& )
    : KTextEditor::Plugin ( parent )
{
}

PluginKateOpenHeader::~PluginKateOpenHeader()
{
}

QObject *PluginKateOpenHeader::createView (KTextEditor::MainWindow *mainWindow)
{
    return new PluginViewKateOpenHeader(this,mainWindow);
}

void PluginKateOpenHeader::slotOpenHeader ()
{
  KTextEditor::Application *application=KTextEditor::Editor::instance()->application();
  if (!application->activeMainWindow())
    return;

  KTextEditor::View * kv (application->activeMainWindow()->activeView());
  if (!kv) return;

  QUrl url=kv->document()->url();
  if ((!url.isValid()) || (url.isEmpty())) return;

  qDebug() << "Trying to open opposite of " << url.toString();
  qDebug() << "Trying to open opposite of toLocalFile:" << url.toLocalFile();
  qDebug() << "Trying to open opposite of path:" << url.path();
  QFileInfo info( url.path() );
  QString extension = info.suffix().toLower();

  QStringList headers( QStringList() << QStringLiteral("h") << QStringLiteral("H") << QStringLiteral("hh") << QStringLiteral("hpp") << QStringLiteral("cuh"));
  QStringList sources( QStringList() << QStringLiteral("c") << QStringLiteral("cpp") << QStringLiteral("cc") << QStringLiteral("cp") << QStringLiteral("cxx") << QStringLiteral("m")<< QStringLiteral("cu"));

  if( sources.contains( extension ) ) {
    if (tryOpenInternal(url, headers)) return;
    tryOpen( url, headers );
  } else if ( headers.contains( extension ) ) {
    if (tryOpenInternal(url, sources)) return;
    tryOpen( url, sources );
  }
}


bool PluginKateOpenHeader::tryOpenInternal( const QUrl& url, const QStringList& extensions )
{
  KTextEditor::Application *application=KTextEditor::Editor::instance()->application();
  if (!application->activeMainWindow())
    return false;

  qDebug() << "Trying to find already opened" << url.toString() << " with extensions " << extensions.join(QStringLiteral(" "));
  QString basename = QFileInfo( url.path() ).baseName();
  QUrl newURL( url );
    
  for( QStringList::ConstIterator it = extensions.begin(); it != extensions.end(); ++it ) {
    setFileName( &newURL,basename + QStringLiteral(".") + *it );
    KTextEditor::Document *doc= application->findUrl(newURL);
    if (doc) {
      application->activeMainWindow()->openUrl(newURL);
      return true;
    }
    setFileName(&newURL, basename + QStringLiteral(".") + (*it).toUpper() );
    doc= application->findUrl(newURL);
    if (doc) {
      application->activeMainWindow()->openUrl(newURL);
      return true;
    }   
  }
  return false;
}


void PluginKateOpenHeader::tryOpen( const QUrl& url, const QStringList& extensions )
{
  KTextEditor::Application *application=KTextEditor::Editor::instance()->application();
  if (!application->activeMainWindow())
    return;

  qDebug() << "Trying to open " << url.toString() << " with extensions " << extensions.join(QStringLiteral(" "));
  QString basename = QFileInfo( url.path() ).baseName();
  QUrl newURL( url );
    
  
  for( QStringList::ConstIterator it = extensions.begin(); it != extensions.end(); ++it ) {
    setFileName( &newURL,basename + QStringLiteral(".") + *it );
    if( fileExists( newURL) ) {
      application->activeMainWindow()->openUrl( newURL );
      return;
    }
    setFileName(&newURL, basename + QStringLiteral(".") + (*it).toUpper() );
    if( fileExists( newURL) ) {
      application->activeMainWindow()->openUrl( newURL );
      return;
    }    
  }
}

bool PluginKateOpenHeader::fileExists(const QUrl &url)
{
    if (url.isLocalFile()) {
        return QFile::exists(url.toLocalFile());
    }
    
    KIO::JobFlags flags =  KIO::DefaultFlags;
    KIO::StatJob *job = KIO::stat(url, flags);
    KJobWidgets::setWindow(job, KTextEditor::Editor::instance()->application()->activeMainWindow()->window());
    job->setSide(KIO::StatJob::DestinationSide/*SourceSide*/);
    job->exec();
    return !job->error();

}

void PluginKateOpenHeader::setFileName(QUrl *url,const QString &_txt)
{
    url->setFragment(QString());
    int i = 0;
    while (i < _txt.length() && _txt[i] == QLatin1Char('/')) {
        ++i;
    }
    QString tmp = i ? _txt.mid(i) : _txt;

    QString path = url->path();
    if (path.isEmpty())
#ifdef Q_OS_WIN
        path = url->isLocalFile() ? QDir::rootPath() : QStringLiteral("/");
#else
        path = QDir::rootPath();
#endif    
    else {
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));
        if (lastSlash == -1) {
            path.clear();    // there's only the file name, remove it
        } else if (!path.endsWith(QLatin1Char('/'))) {
            path.truncate(lastSlash + 1);    // keep the "/"
        }
    }

    path += tmp;
    url->setPath(path);
}

bool PluginViewKateOpenHeader::exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &)
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

    msg = i18n("<p><b>toggle-header &mdash; switch between header and corresponding c/cpp file</b></p>"
            "<p>usage: <tt><b>toggle-header</b></tt></p>"
            "<p>When editing C or C++ code, this command will switch between a header file and "
            "its corresponding C/C++ file or vice versa.</p>"
            "<p>For example, if you are editing myclass.cpp, <tt>toggle-header</tt> will change "
            "to myclass.h if this file is available.</p>"
            "<p>Pairs of the following filename suffixes will work:<br />"
            " Header files: h, H, hh, hpp<br />"
            " Source files: c, cpp, cc, cp, cxx</p>");

    return true;
}

#include "plugin_kateopenheader.moc"
