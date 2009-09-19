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
    
  public:
    KateWaiter (QCoreApplication *app, const QString &service)
      : QObject (app), m_app (app), m_service (service) {
      connect ( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( QString, QString, QString ) )
          , this, SLOT(serviceOwnerChanged( QString, QString, QString )) ); 
    }

  public Q_SLOTS:
    void exiting () {
      m_app->quit ();
    }
    
    void serviceOwnerChanged( const QString & name, const QString &, const QString &) {
      if (name != m_service)
          return;
      
      m_app->quit ();
    }
};

class RunningInstanceInfo: public QObject {
  Q_OBJECT
  
  public:
    RunningInstanceInfo(const QString& serviceName_):
      QObject(),
      serviceName(serviceName_),
      dbus_if(new QDBusInterface(serviceName_,QLatin1String("/MainApplication"),
          QString(), //I don't know why it does not work if I specify org.kde.Kate.Application here
          QDBusConnection::sessionBus(),this)) {
      if (!dbus_if->isValid()) {
        std::cerr<<qPrintable(QDBusConnection::sessionBus().lastError().message())<<std::endl;
      }
      QVariant a_s=dbus_if->property("activeSession");            
/*      std::cerr<<a_s.isValid()<<std::endl;
      std::cerr<<"property:"<<qPrintable(a_s.toString())<<std::endl;
      std::cerr<<qPrintable(QDBusConnection::sessionBus().lastError().message())<<std::endl;*/
      if (!a_s.isValid())
        sessionName=QString("___NO_SESSION_OPENED__%1").arg(dummy_session++);
      else
        if (a_s.toString().isEmpty())
          sessionName=QString("___DEFAULT_CONSTRUCTED_SESSION__%1").arg(dummy_session++);
        else
          sessionName=a_s.toString();
    };
    virtual ~RunningInstanceInfo(){}
    QString serviceName;
    QDBusInterface* dbus_if;
    QString sessionName;
    
    
  private:
    static int dummy_session;
};

int RunningInstanceInfo::dummy_session=0;

extern "C" KDE_EXPORT int kdemain( int argc, char **argv )
{
  // here we go, construct the Kate version
  QByteArray kateVersion = KateApp::kateVersion().toLatin1();

  KAboutData aboutData ("kate", 0, ki18n("Kate"), kateVersion,
                        ki18n( "Kate - Advanced Text Editor" ), KAboutData::License_LGPL_V2,
                        ki18n( "(c) 2000-2005 The Kate Authors" ), KLocalizedString(), "http://www.kate-editor.org");
  aboutData.setOrganizationDomain("kde.org");
  aboutData.addAuthor (ki18n("Christoph Cullmann"), ki18n("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
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
  options.add("new", ki18n("Force start of a new kate instance (is ignored if start is used and another kate instance already has the given session opened), forced if no parameters and no urls are given at all"));
  options.add("b");
  options.add("block", ki18n("If using an already running kate instance, block until it exits, if urls given to open"));
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
  options.add("use", ki18n("Reuse existing Kate instance, default, only for compatiblity"));
  options.add("+[URL]", ki18n("Document to open"));
  KCmdLineArgs::addCmdLineOptions (options);
  KCmdLineArgs::addTempFileOption();

  // get our command line args ;)
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  QDBusConnectionInterface *i = QDBusConnection::sessionBus().interface ();
  
  // look up all running kate instances and there sessions
  QDBusReply<QStringList> servicesReply = i->registeredServiceNames ();
  QStringList services;
  if (servicesReply.isValid())
    services = servicesReply.value ();

  QString serviceName;
  QMap<QString, RunningInstanceInfo*> mapSessionRii;
  QStringList kateServices;
  foreach (const QString &s, services)
  {
    if (s.startsWith ("org.kde.kate-"))
    {
      RunningInstanceInfo* rii=new RunningInstanceInfo(s);
      if (mapSessionRii.contains(rii->sessionName)) return 1; //ERROR no two instances may have the same session name
      mapSessionRii.insert(rii->sessionName,rii);
      kateServices<<s;
      std::cerr<<qPrintable(s)<<"running instance:"<< rii->sessionName.toUtf8().data()<<std::endl;
    }
  }
  bool force_new=args->isSet("new");
  if (!force_new) {
    if ( (!
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
    }
  }
  
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
    if (args->isSet ("start"))
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
    
    // open given files...
    for (int z = 0; z < args->count(); z++)
    {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), "org.kde.Kate.Application", "openUrl");

      QList<QVariant> dbusargs;
      dbusargs.append(args->url(z).url());
      dbusargs.append(enc);
      dbusargs.append(tempfileSet);
      m.setArguments(dbusargs);

      QDBusConnection::sessionBus().call (m);
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
    
    // connect dbus signal
    if (needToBlock) {
      KateWaiter *waiter = new KateWaiter (&app, serviceName);
      QDBusConnection::sessionBus().connect(serviceName, QString("/MainApplication"), "org.kde.Kate.Application", "exiting", waiter, SLOT(exiting()));
    }
    
#ifdef Q_WS_X11
    // make the world happy, we are started, kind of...
    KStartupInfo::appStarted ();
#endif

    // this will wait until exiting is emited by the used instance, if wanted...
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
