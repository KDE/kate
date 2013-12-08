

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
#include "katerunninginstanceinfo.h"
#include <kate_export.h>
#include <KLocale>
#include <KCmdLineArgs>
#include <KAboutData>
#include <KStartupInfo>
#include <kdebug.h>

#include <QTextCodec>
#include <QByteArray>
#include <QCoreApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariant>

#include <iostream>

class KateWaiter : public QObject {
  Q_OBJECT
  
  private:
    QCoreApplication *m_app;
    QString m_service;
    QStringList m_tokens;
  public:
    KateWaiter (QCoreApplication *app, const QString &service,const QStringList &tokens)
      : QObject (app), m_app (app), m_service (service),m_tokens(tokens) {
      connect ( QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString))
          , this, SLOT(serviceOwnerChanged(QString,QString,QString)) ); 
    }

  public Q_SLOTS:
    void exiting () {
      m_app->quit ();
    }
    
    void documentClosed(const QString& token) {
      m_tokens.removeAll(token);
      if (m_tokens.count()==0) m_app->quit();
    }
    
    void serviceOwnerChanged( const QString & name, const QString &, const QString &) {
      if (name != m_service)
          return;
      
      m_app->quit ();
    }
};


extern "C" KDE_EXPORT int kdemain( int argc, char **argv )
{
  /**
   * construct about data for Kate
   */
  KAboutData aboutData ("kate", 0, i18n("Kate"), KateApp::kateVersion().toLatin1(),
                        i18n( "Kate - Advanced Text Editor" ), KAboutData::License_LGPL_V2,
                        i18n( "(c) 2000-2013 The Kate Authors" ), QString(), "http://www.kate-editor.org");
  aboutData.setOrganizationDomain("kde.org");
  aboutData.addAuthor (i18n("Christoph Cullmann"), i18n("Maintainer"), "cullmann@kde.org", "http://www.cullmann.io");
  aboutData.addAuthor (i18n("Anders Lund"), i18n("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor (i18n("Joseph Wenninger"), i18n("Core Developer"), "jowenn@kde.org", "http://stud3.tuwien.ac.at/~e9925371");
  aboutData.addAuthor (i18n("Hamish Rodda"), i18n("Core Developer"), "rodda@kde.org");
  aboutData.addAuthor (i18n("Dominik Haumann"), i18n("Developer & Highlight wizard"), "dhdev@gmx.de");
  aboutData.addAuthor (i18n("Waldo Bastian"), i18n( "The cool buffersystem" ), "bastian@kde.org" );
  aboutData.addAuthor (i18n("Charles Samuels"), i18n("The Editing Commands"), "charles@kde.org");
  aboutData.addAuthor (i18n("Matt Newell"), i18n("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor (i18n("Michael Bartl"), i18n("Former Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor (i18n("Michael McCallum"), i18n("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor (i18n("Jochen Wilhemly"), i18n( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor (i18n("Michael Koch"), i18n("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor (i18n("Christian Gebauer"), QString(), "gebauer@kde.org" );
  aboutData.addAuthor (i18n("Simon Hausmann"), QString(), "hausmann@kde.org" );
  aboutData.addAuthor (i18n("Glen Parker"), i18n("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor (i18n("Scott Manson"), i18n("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  aboutData.addAuthor (i18n("John Firebaugh"), i18n("Patches and more"), "jfirebaugh@kde.org");
  aboutData.addAuthor (i18n("Pablo Martín"), i18n("Python Plugin Developer"), "goinnn@gmail.com", "http://github.com/goinnn/");
  aboutData.addAuthor (i18n("Gerald Senarclens de Grancy"), i18n("QA and Scripting"), "oss@senarclens.eu", "http://find-santa.eu/");

  aboutData.addCredit (i18n("Matteo Merli"), i18n("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  aboutData.addCredit (i18n("Rocky Scaletta"), i18n("Highlighting for VHDL"), "rocky@purdue.edu");
  aboutData.addCredit (i18n("Yury Lebedev"), i18n("Highlighting for SQL"));
  aboutData.addCredit (i18n("Chris Ross"), i18n("Highlighting for Ferite"));
  aboutData.addCredit (i18n("Nick Roux"), i18n("Highlighting for ILERPG"));
  aboutData.addCredit (i18n("Carsten Niehaus"), i18n("Highlighting for LaTeX"));
  aboutData.addCredit (i18n("Per Wigren"), i18n("Highlighting for Makefiles, Python"));
  aboutData.addCredit (i18n("Jan Fritz"), i18n("Highlighting for Python"));
  aboutData.addCredit (i18n("Daniel Naber"));
  aboutData.addCredit (i18n("Roland Pabel"), i18n("Highlighting for Scheme"));
  aboutData.addCredit (i18n("Cristi Dumitrescu"), i18n("PHP Keyword/Datatype list"));
  aboutData.addCredit (i18n("Carsten Pfeiffer"), i18n("Very nice help"));
  aboutData.addCredit (i18n("All people who have contributed and I have forgotten to mention"));
  
  /**
   * Create the QApplication with the right options set
   * take component name and org. name from KAboutData
   */
  QApplication app (argc, argv);
  app.setApplicationName(aboutData.componentName());
  app.setOrganizationDomain(aboutData.organizationDomain());
  app.setQuitOnLastWindowClosed (false);
  
#ifdef TODO
  
  // FIXME KF5
  
  
  // command line args init and co
  KCmdLineArgs::init (argc, argv, &aboutData);

  KCmdLineOptions options;
  options.add("s");
  options.add("start <name>", i18n("Start Kate with a given session"));
  options.add("startanon", i18n("Start Kate with a new anonymous session, implies '-n'"));
  options.add("n");
  options.add("new", i18n("Force start of a new kate instance (is ignored if start is used and another kate instance already has the given session opened), forced if no parameters and no URLs are given at all"));
  options.add("b");
  options.add("block", i18n("If using an already running kate instance, block until it exits, if URLs given to open"));
  options.add("p");
  options.add("pid <pid>", i18n("Only try to reuse kate instance with this pid (is ignored if start is used and another kate instance already has the given session opened)"));
  options.add("e");
  options.add("encoding <name>", i18n("Set encoding for the file to open"));
  options.add("l");
  options.add("line <line>", i18n("Navigate to this line"));
  options.add("c");
  options.add("column <column>", i18n("Navigate to this column"));
  options.add("i");
  options.add("stdin", i18n("Read the contents of stdin"));
  options.add("u");
  options.add("use", i18n("Reuse existing Kate instance; default, only for compatibility"));
  options.add("+[URL]", i18n("Document to open"));
  KCmdLineArgs::addCmdLineOptions (options);
  KCmdLineArgs::addTempFileOption();

  // get our command line args ;)
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  QDBusConnectionInterface *i = QDBusConnection::sessionBus().interface ();

  KateRunningInstanceMap mapSessionRii;
  if (!fillinRunningKateAppInstances(&mapSessionRii)) return 1;
  
  QStringList kateServices;
  for(KateRunningInstanceMap::const_iterator it=mapSessionRii.constBegin();it!=mapSessionRii.constEnd();++it)
  {
    kateServices<<(*it)->serviceName;
  }
  QString serviceName;


  bool force_new=args->isSet("new");
  if (!force_new) {
    if ( !(
      args->isSet("start") ||
      args->isSet("new") ||
      args->isSet("pid") ||
      args->isSet("encoding") ||
      args->isSet("line") ||
      args->isSet("column") ||
      args->isSet("stdin") ||
      args->isSet("use")
      ) && (args->count()==0)) force_new=true;
  }
    
  QString start_session;
  bool session_already_opened=false;
  
  //check if we try to start an already opened session
  if (args->isSet("startanon"))
  {
    force_new=true;
  }
  else if (args->isSet("start"))
  {
    start_session=args->getOption("start");
    if (mapSessionRii.contains(start_session)) {
      serviceName=mapSessionRii[start_session]->serviceName;
      force_new=false;
      session_already_opened=true;
    }
  }
  
  //cleanup map
  cleanupRunningKateAppInstanceMap(&mapSessionRii);

  //if no new instance is forced and no already opened session is requested,
  //check if a pid is given, which should be reused.
  // two possibilities: pid given or not...
  if ((!force_new) && serviceName.isEmpty())
  {
    if ( (args->isSet("pid")) || (!qgetenv("KATE_PID").isEmpty()))
    {
      QString usePid;
      if (args->isSet("pid")) usePid = args->getOption("pid");
      else usePid = QString::fromLocal8Bit(qgetenv("KATE_PID"));
      
      serviceName = "org.kde.kate-" + usePid;
      if (!kateServices.contains(serviceName)) serviceName.clear();
    }
  }
  
  if ( (!force_new) && ( serviceName.isEmpty()))
  {
    if (kateServices.count()>0)
      serviceName=kateServices[0];
  } 
    
  
  //check again if service is still running
  bool foundRunningService = false;
  if (!serviceName.isEmpty ())
  {
    QDBusReply<bool> there = i->isServiceRegistered (serviceName);
    foundRunningService = there.isValid () && there.value();
  }

         
  if (foundRunningService)
  {
    // open given session
    if (args->isSet ("start") && (!session_already_opened) )
    {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), "org.kde.Kate.Application", "activateSession");

      QList<QVariant> dbusargs;
      dbusargs.append(args->getOption("start"));
      m.setArguments(dbusargs);

      QDBusConnection::sessionBus().call (m);
    }

    QString enc = args->isSet("encoding") ? args->getOption("encoding") : QByteArray("");

    bool tempfileSet = KCmdLineArgs::isTempFileSet();

    // only block, if files to open there....
    bool needToBlock = args->isSet( "block" ) && (args->count() > 0);
    
    QStringList tokens;
    
    // open given files...
    for (int z = 0; z < args->count(); z++)
    {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), "org.kde.Kate.Application", "tokenOpenUrl");

      QList<QVariant> dbusargs;
      dbusargs.append(args->url(z).url());
      dbusargs.append(enc);
      dbusargs.append(tempfileSet);
      m.setArguments(dbusargs);

      QDBusMessage res=QDBusConnection::sessionBus().call (m);
      if (res.type()==QDBusMessage::ReplyMessage)
      {
        if (res.arguments().count()==1)
        {
            QVariant v=res.arguments()[0];
            if (v.isValid())
            {
              QString s=v.toString();
              if ((!s.isEmpty()) && (s!=QString("ERROR")))
              {
                tokens<<s;
              }
            }
        }
      }
    }
    
    if( args->isSet( "stdin" ) )
    {
      QTextStream input(stdin, QIODevice::ReadOnly);

      // set chosen codec
      QTextCodec *codec = args->isSet("encoding") ? QTextCodec::codecForName(args->getOption("encoding").toUtf8()) : 0;

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

      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), "org.kde.Kate.Application", "openInput");

      QList<QVariant> dbusargs;
      dbusargs.append(text);
      m.setArguments(dbusargs);

      QDBusConnection::sessionBus().call (m);
    }

    int line = 0;
    int column = 0;
    bool nav = false;

    if (args->isSet ("line"))
    {
      line = args->getOption ("line").toInt() - 1;
      nav = true;
    }

    if (args->isSet ("column"))
    {
      column = args->getOption ("column").toInt() - 1;
      nav = true;
    }

    if (nav)
    {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), "org.kde.Kate.Application", "setCursor");

      QList<QVariant> args;
      args.append(line);
      args.append(column);
      m.setArguments(args);

      QDBusConnection::sessionBus().call (m);
    }

    // activate the used instance
    QDBusMessage activateMsg = QDBusMessage::createMethodCall (serviceName,
      QLatin1String("/MainApplication"), "org.kde.Kate.Application", "activate");
    QDBusConnection::sessionBus().call (activateMsg);

    
    // application object to have event loop
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // It's too bad, that we have to use KApplication here, since this forces us to
    // register a service to dbus. If we don't use KApplication we cannot use KStartupInfo
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    KApplication app(true);

    // do no session management for this app, just client
    app.disableSessionManagement ();
    
    // connect dbus signal
    if (needToBlock) {
      KateWaiter *waiter = new KateWaiter (&app, serviceName,tokens);
      QDBusConnection::sessionBus().connect(serviceName, QString("/MainApplication"), "org.kde.Kate.Application", "exiting", waiter, SLOT(exiting()));
      QDBusConnection::sessionBus().connect(serviceName, QString("/MainApplication"), "org.kde.Kate.Application", "documentClosed", waiter, SLOT(documentClosed(QString)));
    }
    
#ifdef Q_WS_X11
    // make the world happy, we are started, kind of...
    KStartupInfo::appStarted ();
#endif

    // this will wait until exiting is emitted by the used instance, if wanted...
    return needToBlock ? app.exec () : 0;
  }
#endif 

  /**
   * construct the real kate app object ;)
   * behaves like a singleton, one unique instance
   */
  KateApp kateApp;
  if (kateApp.shouldExit()) return 0;

  /**
   * start main event loop for our application
   */
  return app.exec();
}

#include "katemain.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
