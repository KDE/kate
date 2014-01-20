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

#include "config.h"

#include "kateapp.h"
#include "katerunninginstanceinfo.h"

#include <KAboutData>
#include <KLocalizedString>
#include <kdbusservice.h>

#include <QByteArray>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QTextCodec>
#include <QUrl>
#include <QVariant>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QApplication>
#include <QDir>

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


extern "C" Q_DECL_EXPORT int kdemain( int argc, char **argv )
{
  QLoggingCategory::setFilterRules(QStringLiteral("kate = true"));

  /**
   * construct about data for Kate
   */
  KAboutData aboutData (QLatin1String("kate"), QString(), i18n("Kate"), QLatin1String(KATE_VERSION),
                        i18n( "Kate - Advanced Text Editor" ), KAboutData::License_LGPL_V2,
                        i18n( "(c) 2000-2014 The Kate Authors" ), QString(), QLatin1String("http://kate-editor.org"));

  /**
   * right dbus prefix == org.kde.
   */
  aboutData.setOrganizationDomain ("kde.org");
  
  aboutData.addAuthor (i18n("Christoph Cullmann"), i18n("Maintainer"), QLatin1String("cullmann@kde.org"), QLatin1String("http://www.cullmann.io"));
  aboutData.addAuthor (i18n("Anders Lund"), i18n("Core Developer"), QLatin1String("anders@alweb.dk"), QLatin1String("http://www.alweb.dk"));
  aboutData.addAuthor (i18n("Joseph Wenninger"), i18n("Core Developer"), QLatin1String("jowenn@kde.org"), QLatin1String("http://stud3.tuwien.ac.at/~e9925371"));
  aboutData.addAuthor (i18n("Hamish Rodda"), i18n("Core Developer"), QLatin1String("rodda@kde.org"));
  aboutData.addAuthor (i18n("Dominik Haumann"), i18n("Developer & Highlight wizard"), QLatin1String("dhdev@gmx.de"));
  aboutData.addAuthor (i18n("Waldo Bastian"), i18n("The cool buffersystem"), QLatin1String("bastian@kde.org") );
  aboutData.addAuthor (i18n("Charles Samuels"), i18n("The Editing Commands"), QLatin1String("charles@kde.org"));
  aboutData.addAuthor (i18n("Matt Newell"), i18n("Testing, ..."), QLatin1String("newellm@proaxis.com"));
  aboutData.addAuthor (i18n("Michael Bartl"), i18n("Former Core Developer"), QLatin1String("michael.bartl1@chello.at"));
  aboutData.addAuthor (i18n("Michael McCallum"), i18n("Core Developer"), QLatin1String("gholam@xtra.co.nz"));
  aboutData.addAuthor (i18n("Jochen Wilhemly"), i18n("KWrite Author"), QLatin1String("digisnap@cs.tu-berlin.de") );
  aboutData.addAuthor (i18n("Michael Koch"), i18n("KWrite port to KParts"), QLatin1String("koch@kde.org"));
  aboutData.addAuthor (i18n("Christian Gebauer"), QString(), QLatin1String("gebauer@kde.org") );
  aboutData.addAuthor (i18n("Simon Hausmann"), QString(), QLatin1String("hausmann@kde.org") );
  aboutData.addAuthor (i18n("Glen Parker"), i18n("KWrite Undo History, Kspell integration"), QLatin1String("glenebob@nwlink.com"));
  aboutData.addAuthor (i18n("Scott Manson"), i18n("KWrite XML Syntax highlighting support"), QLatin1String("sdmanson@alltel.net"));
  aboutData.addAuthor (i18n("John Firebaugh"), i18n("Patches and more"), QLatin1String("jfirebaugh@kde.org"));
  aboutData.addAuthor (i18n("Pablo Martín"), i18n("Python Plugin Developer"), QLatin1String("goinnn@gmail.com"), QLatin1String("http://github.com/goinnn/"));
  aboutData.addAuthor (i18n("Gerald Senarclens de Grancy"), i18n("QA and Scripting"), QLatin1String("oss@senarclens.eu"), QLatin1String("http://find-santa.eu/"));

  aboutData.addCredit (i18n("Matteo Merli"), i18n("Highlighting for RPM Spec-Files, Perl, Diff and more"), QLatin1String("merlim@libero.it"));
  aboutData.addCredit (i18n("Rocky Scaletta"), i18n("Highlighting for VHDL"), QLatin1String("rocky@purdue.edu"));
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
   * register about data
   */
  KAboutData::setApplicationData (aboutData);
  
  /**
   * Create the QApplication with the right options set
   * take component name and org. name from KAboutData
   */
  QApplication app (argc, argv);
  app.setApplicationName (aboutData.componentName());
  app.setApplicationDisplayName (aboutData.displayName());
  app.setOrganizationDomain (aboutData.organizationDomain());
  app.setApplicationVersion (aboutData.version());
  app.setQuitOnLastWindowClosed (false);

  /**
   * Create command line parser and feed it with known options
   */  
  QCommandLineParser parser;
  parser.setApplicationDescription (aboutData.shortDescription());
  parser.addHelpOption ();
  parser.addVersionOption ();
  
  // -s/--start session option
  const QCommandLineOption startSessionOption (QStringList () << QLatin1String("s") << QLatin1String("start"), i18n("Start Kate with a given session."), QLatin1String("session"));
  parser.addOption (startSessionOption);
  
  // --startanon session option
  const QCommandLineOption startAnonymousSessionOption (QStringList () << QLatin1String("startanon"), i18n("Start Kate with a new anonymous session, implies '-n'."));
  parser.addOption (startAnonymousSessionOption);
  
  // -n/--new option
  const QCommandLineOption startNewInstanceOption (QStringList () << QLatin1String("n") << QLatin1String("new"), i18n("Force start of a new kate instance (is ignored if start is used and another kate instance already has the given session opened), forced if no parameters and no URLs are given at all."));
  parser.addOption (startNewInstanceOption);
  
  // -b/--block option
  const QCommandLineOption startBlockingOption (QStringList () << QLatin1String("b") << QLatin1String("block"), i18n("If using an already running kate instance, block until it exits, if URLs given to open."));
  parser.addOption (startBlockingOption);
  
  // -p/--pid option
  const QCommandLineOption usePidOption (QStringList () << QLatin1String("p") << QLatin1String("pid"), i18n("Only try to reuse kate instance with this pid (is ignored if start is used and another kate instance already has the given session opened)."), QLatin1String("pid"));
  parser.addOption (usePidOption);
  
  // -e/--encoding option
  const QCommandLineOption useEncodingOption (QStringList () << QLatin1String("e") << QLatin1String("encoding"), i18n("Set encoding for the file to open."), QLatin1String("encoding"));
  parser.addOption (useEncodingOption);
  
  // -l/--line option
  const QCommandLineOption gotoLineOption (QStringList () << QLatin1String("l") << QLatin1String("line"), i18n("Navigate to this line."), QLatin1String("line"));
  parser.addOption (gotoLineOption);
  
  // -c/--column option
  const QCommandLineOption gotoColumnOption (QStringList () << QLatin1String("c") << QLatin1String("column"), i18n("Navigate to this column."), QLatin1String("column"));
  parser.addOption (gotoColumnOption);
  
  // -i/--stdin option
  const QCommandLineOption readStdInOption (QStringList () << QLatin1String("i") << QLatin1String("stdin"), i18n("Read the contents of stdin."));
  parser.addOption (readStdInOption);

  // --tempfile option
  const QCommandLineOption tempfileOption (QStringList () << QLatin1String("tempfile"), i18n("The files/URLs opened by the application will be deleted after use"));
  parser.addOption (tempfileOption);
  
  // urls to open
  parser.addPositionalArgument(QLatin1String("urls"), i18n("Documents to open."), QLatin1String("[urls...]"));
  
  /**
   * do the command line parsing
   */
  parser.process (app);

  QDBusConnectionInterface *i = QDBusConnection::sessionBus().interface ();

  KateRunningInstanceMap mapSessionRii;
  if (!fillinRunningKateAppInstances(&mapSessionRii)) return 1;
  
  QStringList kateServices;
  for(KateRunningInstanceMap::const_iterator it=mapSessionRii.constBegin();it!=mapSessionRii.constEnd();++it)
  {
    kateServices<<(*it)->serviceName;
  }
  QString serviceName;

  const QStringList urls = parser.positionalArguments();

  bool force_new = parser.isSet(startNewInstanceOption);

  if (!force_new) {
    if ( !(
      parser.isSet(startSessionOption) ||
      parser.isSet(startNewInstanceOption) ||
      parser.isSet(usePidOption) ||
      parser.isSet(useEncodingOption) ||
      parser.isSet(gotoLineOption) ||
      parser.isSet(gotoColumnOption) ||
      parser.isSet(readStdInOption)
      ) && (urls.isEmpty())) force_new = true;
  }
    
  QString start_session;
  bool session_already_opened=false;
  
  //check if we try to start an already opened session
  if (parser.isSet(startAnonymousSessionOption))
  {
    force_new = true;
  }
  else if (parser.isSet(startSessionOption))
  {
    start_session = parser.value(startSessionOption);
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
    if ( (parser.isSet(usePidOption)) || (!qgetenv("KATE_PID").isEmpty()))
    {
      QString usePid = (parser.isSet(usePidOption)) ?
        parser.value(usePidOption) :
        QString::fromLocal8Bit(qgetenv("KATE_PID"));
      
      serviceName = QLatin1String("org.kde.kate-") + usePid;
      if (!kateServices.contains(serviceName)) serviceName.clear();
    }
  }
  
  if ( (!force_new) && ( serviceName.isEmpty()))
  {
    if (kateServices.count()>0)
      serviceName = kateServices[0];
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
    if (parser.isSet(startSessionOption) && (!session_already_opened) )
    {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), QLatin1String("org.kde.Kate.Application"), QLatin1String("activateSession"));

      QList<QVariant> dbusargs;
      dbusargs.append(parser.value(startSessionOption));
      m.setArguments(dbusargs);

      QDBusConnection::sessionBus().call (m);
    }

    QString enc = parser.isSet(useEncodingOption) ? parser.value(useEncodingOption) : QString();

    bool tempfileSet = parser.isSet(tempfileOption);

    // only block, if files to open there....
    bool needToBlock = parser.isSet(startBlockingOption) && !urls.isEmpty();
    
    QStringList tokens;
    
    // open given files...
    foreach(const QString &url, urls)
    {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), QLatin1String("org.kde.Kate.Application"), QLatin1String("tokenOpenUrl"));

      QList<QVariant> dbusargs;
      
      // convert to an url
      QRegExp withProtocol(QLatin1String("^[a-zA-Z]+:")); // TODO: remove after Qt supports this on its own
      if (withProtocol.indexIn(url) == 0) {
        dbusargs.append(QUrl::fromUserInput(url));
      } else {
        const QString path = QDir::current().absoluteFilePath(url);
        dbusargs.append(QUrl::fromLocalFile(path));
      }
      
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
              if ((!s.isEmpty()) && (s != QLatin1String("ERROR")))
              {
                tokens<<s;
              }
            }
        }
      }
    }
    
    if(parser.isSet(readStdInOption))
    {
      QTextStream input(stdin, QIODevice::ReadOnly);

      // set chosen codec
      QTextCodec *codec = parser.isSet(useEncodingOption) ?
        QTextCodec::codecForName(parser.value(useEncodingOption).toUtf8()) : 0;

      if (codec)
        input.setCodec (codec);

      QString line;
      QString text;

      do
      {
        line = input.readLine();
        text.append( line + QLatin1Char('\n') );
      }
      while( !line.isNull() );

      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), QLatin1String("org.kde.Kate.Application"), QLatin1String("openInput"));

      QList<QVariant> dbusargs;
      dbusargs.append(text);
      m.setArguments(dbusargs);

      QDBusConnection::sessionBus().call (m);
    }

    int line = 0;
    int column = 0;
    bool nav = false;

    if (parser.isSet(gotoLineOption))
    {
      line = parser.value(gotoLineOption).toInt() - 1;
      nav = true;
    }

    if (parser.isSet(gotoColumnOption))
    {
      column = parser.value(gotoColumnOption).toInt() - 1;
      nav = true;
    }

    if (nav)
    {
      QDBusMessage m = QDBusMessage::createMethodCall (serviceName,
              QLatin1String("/MainApplication"), QLatin1String("org.kde.Kate.Application"), QLatin1String("setCursor"));

      QList<QVariant> args;
      args.append(line);
      args.append(column);
      m.setArguments(args);

      QDBusConnection::sessionBus().call (m);
    }

    // activate the used instance
    QDBusMessage activateMsg = QDBusMessage::createMethodCall (serviceName,
      QLatin1String("/MainApplication"), QLatin1String("org.kde.Kate.Application"), QLatin1String("activate"));
    QDBusConnection::sessionBus().call (activateMsg);
    
    // connect dbus signal
    if (needToBlock) {
      KateWaiter *waiter = new KateWaiter (&app, serviceName,tokens);
      QDBusConnection::sessionBus().connect(serviceName, QLatin1String("/MainApplication"), QLatin1String("org.kde.Kate.Application"), QLatin1String("exiting"), waiter, SLOT(exiting()));
      QDBusConnection::sessionBus().connect(serviceName, QLatin1String("/MainApplication"), QLatin1String("org.kde.Kate.Application"), QLatin1String("documentClosed"), waiter, SLOT(documentClosed(QString)));
    }
    
#ifdef Q_WS_X11
    // make the world happy, we are started, kind of...
    KStartupInfo::appStarted ();
#endif

    // this will wait until exiting is emitted by the used instance, if wanted...
    return needToBlock ? app.exec () : 0;
  }

  /**
   * construct the real kate app object ;)
   * behaves like a singleton, one unique instance
   * we are passing our local command line parser to it
   */
  KateApp kateApp (parser);
  
  /**
   * init kate
   * if this returns false, we shall exit
   * else we may enter the main event loop
   */
  if (!kateApp.init())
    return 0;
  
  /**
   * finally register this kate instance for dbus
   */
  const KDBusService dbusService (KDBusService::Multiple);

  /**
   * start main event loop for our application
   */
  return app.exec();
}

#include "main.moc"
