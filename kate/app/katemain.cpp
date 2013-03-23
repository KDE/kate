

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
  // here we go, construct the Kate version
  QByteArray kateVersion = KateApp::kateVersion().toLatin1();

  KAboutData aboutData ("kate", 0, ki18n("Kate"), kateVersion,
                        ki18n( "Kate - Advanced Text Editor" ), KAboutData::License_LGPL_V2,
                        ki18n( "(c) 2000-2005 The Kate Authors" ), KLocalizedString(), "http://www.kate-editor.org");
  aboutData.setOrganizationDomain("kde.org");
  aboutData.addAuthor (ki18n("Christoph Cullmann"), ki18n("Maintainer"), "cullmann@kde.org", "http://www.cullmann.io");
  aboutData.addAuthor (ki18n("Anders Lund"), ki18n("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor (ki18n("Joseph Wenninger"), ki18n("Core Developer"), "jowenn@kde.org", "http://stud3.tuwien.ac.at/~e9925371");
  aboutData.addAuthor (ki18n("Hamish Rodda"), ki18n("Core Developer"), "rodda@kde.org");
  aboutData.addAuthor (ki18n("Dominik Haumann"), ki18n("Developer & Highlight wizard"), "dhdev@gmx.de");
  aboutData.addAuthor (ki18n("Waldo Bastian"), ki18n( "The cool buffersystem" ), "bastian@kde.org" );
  aboutData.addAuthor (ki18n("Charles Samuels"), ki18n("The Editing Commands"), "charles@kde.org");
  aboutData.addAuthor (ki18n("Matt Newell"), ki18n("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor (ki18n("Michael Bartl"), ki18n("Former Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor (ki18n("Michael McCallum"), ki18n("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor (ki18n("Jochen Wilhemly"), ki18n( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor (ki18n("Michael Koch"), ki18n("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor (ki18n("Christian Gebauer"), KLocalizedString(), "gebauer@kde.org" );
  aboutData.addAuthor (ki18n("Simon Hausmann"), KLocalizedString(), "hausmann@kde.org" );
  aboutData.addAuthor (ki18n("Glen Parker"), ki18n("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor (ki18n("Scott Manson"), ki18n("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  aboutData.addAuthor (ki18n("John Firebaugh"), ki18n("Patches and more"), "jfirebaugh@kde.org");
  aboutData.addAuthor (ki18n("Pablo Martín"), ki18n("Python Plugin Developer"), "goinnn@gmail.com", "http://github.com/goinnn/");
  aboutData.addAuthor (ki18n("Gerald Senarclens de Grancy"), ki18n("QA and Scripting"), "oss@senarclens.eu", "http://find-santa.eu/");

  aboutData.addCredit (ki18n("Matteo Merli"), ki18n("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  aboutData.addCredit (ki18n("Rocky Scaletta"), ki18n("Highlighting for VHDL"), "rocky@purdue.edu");
  aboutData.addCredit (ki18n("Yury Lebedev"), ki18n("Highlighting for SQL"));
  aboutData.addCredit (ki18n("Chris Ross"), ki18n("Highlighting for Ferite"));
  aboutData.addCredit (ki18n("Nick Roux"), ki18n("Highlighting for ILERPG"));
  aboutData.addCredit (ki18n("Carsten Niehaus"), ki18n("Highlighting for LaTeX"));
  aboutData.addCredit (ki18n("Per Wigren"), ki18n("Highlighting for Makefiles, Python"));
  aboutData.addCredit (ki18n("Jan Fritz"), ki18n("Highlighting for Python"));
  aboutData.addCredit (ki18n("Daniel Naber"));
  aboutData.addCredit (ki18n("Roland Pabel"), ki18n("Highlighting for Scheme"));
  aboutData.addCredit (ki18n("Cristi Dumitrescu"), ki18n("PHP Keyword/Datatype list"));
  aboutData.addCredit (ki18n("Carsten Pfeiffer"), ki18n("Very nice help"));
  aboutData.addCredit (ki18n("All people who have contributed and I have forgotten to mention"));

  // command line args init and co
  KCmdLineArgs::init (argc, argv, &aboutData);

  KCmdLineOptions options;
  options.add("s");
  options.add("start <name>", ki18n("Start Kate with a given session"));
  options.add("startanon", ki18n("Start Kate with a new anonymous session, implies '-n'"));
  options.add("n");
  options.add("new", ki18n("Force start of a new kate instance (is ignored if start is used and another kate instance already has the given session opened), forced if no parameters and no URLs are given at all"));
  options.add("b");
  options.add("block", ki18n("If using an already running kate instance, block until it exits, if URLs given to open"));
  options.add("p");
  options.add("pid <pid>", ki18n("Only try to reuse kate instance with this pid (is ignored if start is used and another kate instance already has the given session opened)"));
  options.add("e");
  options.add("encoding <name>", ki18n("Set encoding for the file to open"));
  options.add("l");
  options.add("line <line>", ki18n("Navigate to this line"));
  options.add("c");
  options.add("column <column>", ki18n("Navigate to this column"));
  options.add("i");
  options.add("stdin", ki18n("Read the contents of stdin"));
  options.add("u");
  options.add("use", ki18n("Reuse existing Kate instance; default, only for compatibility"));
  options.add("+[URL]", ki18n("Document to open"));
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
    
  bool start_session_set=false;
  QString start_session;
  bool session_already_opened=false;
  
  //check if we try to start an already opened session
  if (args->isSet("startanon"))
  {
    force_new=true;
  }
  else if (args->isSet("start"))
  {
    start_session_set=true;
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

    // do no session managment for this app, just client
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

  // construct the real kate app object ;)
  KateApp app (args);
  if (app.shouldExit()) return 0;

  // execute ourself ;)
  return app.exec();
}

// kate: space-indent on; indent-width 2; replace-tabs on;

#include "katemain.moc"
