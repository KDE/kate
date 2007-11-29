/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include "kateapp.h"
#include "kateapp.moc"

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateviewmanager.h"
#include "katesession.h"
#include "katemainwindow.h"

#include <kate/application.h>

#include <kdeversion.h>
#include <KCmdLineArgs>
#include <KConfig>
#include <KWindowSystem>
#include <KTipDialog>
#include <kdebug.h>
#include <KLibLoader>
#include <KMessageBox>
#include <KLocale>
#include <KStartupInfo>

#include <QFile>
#include <QTimer>
#include <QDir>
#include <QTextCodec>
#include <QTextIStream>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "kateappadaptor.h"

KateApp::KateApp (KCmdLineArgs *args)
    : KApplication ()
    , m_shouldExit(false)
    , m_args (args)
{
  // insert right translations for the katepart
  KGlobal::locale()->insertCatalog("katepart4");
  setQuitOnLastWindowClosed (false);

  // application interface
  m_application = new Kate::Application (this);

  // doc man
  m_docManager = new KateDocManager (this);

  // init all normal plugins
  m_pluginManager = new KatePluginManager (this);

  // session manager up
  m_sessionManager = new KateSessionManager (this);
  
  // real init
  initKate ();
}

KateApp::~KateApp ()
{
  // cu plugin manager
  delete m_pluginManager;

  // delete this now, or we crash
  delete m_docManager;
}

KateApp *KateApp::self ()
{
  return (KateApp *) kapp;
}

Kate::Application *KateApp::application ()
{
  return m_application;
}

/**
 * Has always been the Kate Versioning Scheme:
 * KDE X.Y.Z contains Kate X-1.Y.Z
 */
QString KateApp::kateVersion (bool fullVersion)
{
  return fullVersion ? QString ("%1.%2.%3").arg(KDE::versionMajor() - 1).arg(KDE::versionMinor()).arg(KDE::versionRelease())
         : QString ("%1.%2").arg(KDE::versionMajor() - 1).arg(KDE::versionMinor());
}

void KateApp::initKate ()
{

  kDebug() << "Setting KATE_PID: '" << getpid() << "'";
  ::setenv( "KATE_PID", QString("%1").arg(getpid()).toLatin1(), 1 );

  // handle restore different
  if (isSessionRestored())
  {
    restoreKate ();
  }
  else
  {
    // let us handle our command line args and co ;)
    // we can exit here if session chooser decides
    if (!startupKate ())
    {
      kDebug() << "startupKate returned false";
      m_shouldExit = true;
      return ;
    }
  }

  // application dcop interface
  ( void ) new KateAppAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String("/MainApplication"), this );
}

void KateApp::restoreKate ()
{
  // activate again correct session!!!
  QString lastSession (sessionConfig()->group("General").readEntry ("Last Session", "default.katesession"));
  sessionManager()->activateSession (KateSession::Ptr(new KateSession (sessionManager(), lastSession)), false, false, false);

  // plugins
  KatePluginManager::self ()->loadConfig (sessionConfig());

  // restore the files we need
  m_docManager->restoreDocumentList (sessionConfig());

  // restore all windows ;)
  for (int n = 1; KMainWindow::canBeRestored(n); n++)
    newMainWindow(sessionConfig(), QString ("%1").arg(n));

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

#ifdef Q_WS_X11
  // notify about start
  KStartupInfo::setNewStartupId( activeMainWindow(), startupId());
#endif
}

bool KateApp::startupKate ()
{
  // user specified session to open
  if (m_args->isSet ("start"))
  {
    sessionManager()->activateSession (sessionManager()->giveSession (m_args->getOption("start")), false, false);
  }
  else if (!m_args->isSet( "stdin" ) && (m_args->count() == 0)) // only start session if no files specified
  {
    // let the user choose session if possible
    if (!sessionManager()->chooseSession ())
    {
      kDebug() << "chooseSession returned false, exiting";
      // we will exit kate now, notify the rest of the world we are done
#ifdef Q_WS_X11
      KStartupInfo::appStarted (startupId());
#endif
      return false;
    }
  }

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // notify about start
#ifdef Q_WS_X11
  KStartupInfo::setNewStartupId( activeMainWindow(), startupId());
#endif
  QTextCodec *codec = m_args->isSet("encoding") ? QTextCodec::codecForName(m_args->getOption("encoding").toUtf8()) : 0;

  bool tempfileSet = KCmdLineArgs::isTempFileSet();

  KTextEditor::Document *doc = 0;
  for (int z = 0; z < m_args->count(); z++)
  {
    // this file is no local dir, open it, else warn
    bool noDir = !m_args->url(z).isLocalFile() || !QDir (m_args->url(z).path()).exists();

    if (noDir)
    {
      // open a normal file
      if (codec)
        doc = activeMainWindow()->viewManager()->openUrl( m_args->url(z), codec->name(), false, tempfileSet);
      else
        doc = activeMainWindow()->viewManager()->openUrl( m_args->url(z), QString(), false, tempfileSet);
    }
    else
      KMessageBox::sorry( activeMainWindow(),
                          i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", m_args->url(z).url()) );
  }

  // handle stdin input
  if( m_args->isSet( "stdin" ) )
  {
    QTextStream input(stdin, QIODevice::ReadOnly);

    // set chosen codec
    if (codec)
      input.setCodec (codec);

    QString line;
    QString text;

    do
    {
      line = input.readLine();
      text.append( line + '\n' );
    }
    while( !line.isNull() );

    openInput (text);
  }
  else if ( doc )
    activeMainWindow()->viewManager()->activateView( doc );

  if ( activeMainWindow()->viewManager()->viewCount () == 0 )
    activeMainWindow()->viewManager()->activateView(m_docManager->document (0));

  int line = 0;
  int column = 0;
  bool nav = false;

  if (m_args->isSet ("line"))
  {
    line = m_args->getOption ("line").toInt() - 1;
    nav = true;
  }

  if (m_args->isSet ("column"))
  {
    column = m_args->getOption ("column").toInt() - 1;
    nav = true;
  }

  if (nav && activeMainWindow()->viewManager()->activeView ())
    activeMainWindow()->viewManager()->activeView ()->setCursorPosition (KTextEditor::Cursor (line, column));

  // show the nice tips
  KTipDialog::showTip(activeMainWindow());

  kDebug() << "KateApplication::init finished successful";
  return true;
}

void KateApp::shutdownKate (KateMainWindow *win)
{
  if (!win->queryClose_internal())
    return;

  sessionManager()->saveActiveSession(true, true);

  // unregister...
  QDBusConnection::sessionBus().unregisterObject( QLatin1String("/MainApplication") );

  // cu main windows
  while (!m_mainWindows.isEmpty())
  {
    // mainwindow itself calls KateApp::removeMainWindow(this)
    delete m_mainWindows[0];
  }

  quit ();
}

KatePluginManager *KateApp::pluginManager()
{
  return m_pluginManager;
}

KateDocManager *KateApp::documentManager ()
{
  return m_docManager;
}

KateSessionManager *KateApp::sessionManager ()
{
  return m_sessionManager;
}

bool KateApp::openUrl (const KUrl &url, const QString &encoding, bool isTempFile)
{
  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  QTextCodec *codec = encoding.isEmpty() ? 0 : QTextCodec::codecForName(encoding.toLatin1());

  // this file is no local dir, open it, else warn
  bool noDir = !url.isLocalFile() || !QDir (url.path()).exists();

  if (noDir)
  {
    // show no errors...
    documentManager()->setSuppressOpeningErrorDialogs (true);

    // open a normal file
    if (codec)
      mainWindow->viewManager()->openUrl( url, codec->name(), true, isTempFile);
    else
      mainWindow->viewManager()->openUrl( url, QString(), true, isTempFile );
    
    // back to normal....
    documentManager()->setSuppressOpeningErrorDialogs (false);
  }
  else
    KMessageBox::sorry( mainWindow,
                        i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", url.url()) );

  return true;
}

bool KateApp::setCursor (int line, int column)
{
  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  if (mainWindow->viewManager()->activeView ())
    mainWindow->viewManager()->activeView ()->setCursorPosition (KTextEditor::Cursor (line, column));

  return true;
}

bool KateApp::openInput (const QString &text)
{
  activeMainWindow()->viewManager()->openUrl( KUrl(), "", true );

  if (!activeMainWindow()->viewManager()->activeView ())
    return false;

  KTextEditor::Document *doc = activeMainWindow()->viewManager()->activeView ()->document();

  if (!doc)
    return false;

  return doc->setText (text);
}

KateMainWindow *KateApp::newMainWindow (KConfig *sconfig_, const QString &sgroup_)
{
  KConfig *sconfig = sconfig_ ? sconfig_ : new KConfig(m_sessionManager->defaultSessionFile());
  QString sgroup = !sgroup_.isEmpty() ? sgroup_ : "MainWindow0";
  KateMainWindow *mainWindow = new KateMainWindow (sconfig, sgroup);

  if ((mainWindows() > 1) && m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView())
    mainWindow->viewManager()->activateView ( m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView()->document() );
  else if ((mainWindows() > 1) && (m_docManager->documents() > 0))
    mainWindow->viewManager()->activateView ( (m_docManager->document(m_docManager->documents() - 1)) );
  else if ((mainWindows() > 1) && (m_docManager->documents() < 1))
    mainWindow->viewManager()->openUrl ( KUrl() );

  mainWindow->show ();

  return mainWindow;
}

void KateApp::addMainWindow (KateMainWindow *mainWindow)
{
  m_mainWindows.push_back (mainWindow);
  m_mainWindowsInterfaces.push_back (mainWindow->mainWindow());
}

void KateApp::removeMainWindow (KateMainWindow *mainWindow)
{
  m_mainWindowsInterfaces.removeAll(mainWindow->mainWindow());
  m_mainWindows.removeAll(mainWindow);
}

KateMainWindow *KateApp::activeMainWindow ()
{
  if (m_mainWindows.isEmpty())
    return 0;

  int n = m_mainWindows.indexOf ((KateMainWindow *)activeWindow());

  if (n < 0)
    n = 0;

  return m_mainWindows[n];
}

int KateApp::mainWindows () const
{
  return m_mainWindows.size();
}

int KateApp::mainWindowID(KateMainWindow *window)
{
  for (int i = 0;i < m_mainWindows.size();i++)
    if (window == m_mainWindows[i]) return i;
  return -1;
}

KateMainWindow *KateApp::mainWindow (int n)
{
  if (n < m_mainWindows.size())
    return m_mainWindows[n];

  return 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
