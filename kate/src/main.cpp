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
#include "katewaiter.h"

#include <KAboutData>
#include <KLocalizedString>
#include <KStartupInfo>
#include <kdbusservice.h>

#include <QByteArray>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QUrl>
#include <QVariant>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QApplication>
#include <QDir>

#include "../../urlinfo.h"

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    /**
     * Create application first
     */
    QApplication app(argc, argv);

    /**
     * enable high dpi support
     */
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    /**
     * Connect application with translation catalogs
     */
    KLocalizedString::setApplicationDomain("kate");

    /**
     * construct about data for Kate
     */
    KAboutData aboutData(QStringLiteral("kate"), i18n("Kate"), QStringLiteral(KATE_VERSION),
                         i18n("Kate - Advanced Text Editor"), KAboutLicense::LGPL_V2,
                         i18n("(c) 2000-2015 The Kate Authors"), QString(), QStringLiteral("http://kate-editor.org"));

    /**
     * right dbus prefix == org.kde.
     */
    aboutData.setOrganizationDomain("kde.org");

    aboutData.addAuthor(i18n("Christoph Cullmann"), i18n("Maintainer"), QStringLiteral("cullmann@kde.org"), QStringLiteral("http://www.cullmann.io"));
    aboutData.addAuthor(i18n("Anders Lund"), i18n("Core Developer"), QStringLiteral("anders@alweb.dk"), QStringLiteral("http://www.alweb.dk"));
    aboutData.addAuthor(i18n("Joseph Wenninger"), i18n("Core Developer"), QStringLiteral("jowenn@kde.org"), QStringLiteral("http://stud3.tuwien.ac.at/~e9925371"));
    aboutData.addAuthor(i18n("Hamish Rodda"), i18n("Core Developer"), QStringLiteral("rodda@kde.org"));
    aboutData.addAuthor(i18n("Dominik Haumann"), i18n("Developer & Highlight wizard"), QStringLiteral("dhdev@gmx.de"));
    aboutData.addAuthor(i18n("Alexander Neundorf"), i18n("Developer"), QStringLiteral("neundorf@kde.org"));
    aboutData.addAuthor(i18n("Waldo Bastian"), i18n("The cool buffersystem"), QStringLiteral("bastian@kde.org"));
    aboutData.addAuthor(i18n("Charles Samuels"), i18n("The Editing Commands"), QStringLiteral("charles@kde.org"));
    aboutData.addAuthor(i18n("Matt Newell"), i18n("Testing, ..."), QStringLiteral("newellm@proaxis.com"));
    aboutData.addAuthor(i18n("Michael Bartl"), i18n("Former Core Developer"), QStringLiteral("michael.bartl1@chello.at"));
    aboutData.addAuthor(i18n("Michael McCallum"), i18n("Core Developer"), QStringLiteral("gholam@xtra.co.nz"));
    aboutData.addAuthor(i18n("Jochen Wilhemly"), i18n("KWrite Author"), QStringLiteral("digisnap@cs.tu-berlin.de"));
    aboutData.addAuthor(i18n("Michael Koch"), i18n("KWrite port to KParts"), QStringLiteral("koch@kde.org"));
    aboutData.addAuthor(i18n("Christian Gebauer"), QString(), QStringLiteral("gebauer@kde.org"));
    aboutData.addAuthor(i18n("Simon Hausmann"), QString(), QStringLiteral("hausmann@kde.org"));
    aboutData.addAuthor(i18n("Glen Parker"), i18n("KWrite Undo History, Kspell integration"), QStringLiteral("glenebob@nwlink.com"));
    aboutData.addAuthor(i18n("Scott Manson"), i18n("KWrite XML Syntax highlighting support"), QStringLiteral("sdmanson@alltel.net"));
    aboutData.addAuthor(i18n("John Firebaugh"), i18n("Patches and more"), QStringLiteral("jfirebaugh@kde.org"));
    aboutData.addAuthor(i18n("Pablo Martín"), i18n("Python Plugin Developer"), QStringLiteral("goinnn@gmail.com"), QStringLiteral("http://github.com/goinnn/"));
    aboutData.addAuthor(i18n("Gerald Senarclens de Grancy"), i18n("QA and Scripting"), QStringLiteral("oss@senarclens.eu"), QStringLiteral("http://find-santa.eu/"));

    aboutData.addCredit(i18n("Matteo Merli"), i18n("Highlighting for RPM Spec-Files, Perl, Diff and more"), QStringLiteral("merlim@libero.it"));
    aboutData.addCredit(i18n("Rocky Scaletta"), i18n("Highlighting for VHDL"), QStringLiteral("rocky@purdue.edu"));
    aboutData.addCredit(i18n("Yury Lebedev"), i18n("Highlighting for SQL"));
    aboutData.addCredit(i18n("Chris Ross"), i18n("Highlighting for Ferite"));
    aboutData.addCredit(i18n("Nick Roux"), i18n("Highlighting for ILERPG"));
    aboutData.addCredit(i18n("Carsten Niehaus"), i18n("Highlighting for LaTeX"));
    aboutData.addCredit(i18n("Per Wigren"), i18n("Highlighting for Makefiles, Python"));
    aboutData.addCredit(i18n("Jan Fritz"), i18n("Highlighting for Python"));
    aboutData.addCredit(i18n("Daniel Naber"));
    aboutData.addCredit(i18n("Roland Pabel"), i18n("Highlighting for Scheme"));
    aboutData.addCredit(i18n("Cristi Dumitrescu"), i18n("PHP Keyword/Datatype list"));
    aboutData.addCredit(i18n("Carsten Pfeiffer"), i18n("Very nice help"));
    aboutData.addCredit(i18n("All people who have contributed and I have forgotten to mention"));

    /**
     * set the new Kate mascot
     */
    aboutData.setProgramLogo (QImage(QLatin1String(":/kate/mascot.png")));

    /**
     * register about data
     */
    KAboutData::setApplicationData(aboutData);

    /**
     * take component name and org. name from KAboutData
     */
    app.setApplicationName(aboutData.componentName());
    app.setApplicationDisplayName(aboutData.displayName());
    app.setOrganizationDomain(aboutData.organizationDomain());
    app.setApplicationVersion(aboutData.version());
    app.setQuitOnLastWindowClosed(false);

    /**
     * set the program icon
     */
    QApplication::setWindowIcon(QIcon::fromTheme(QLatin1String("kate")));

    /**
     * Create command line parser and feed it with known options
     */
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.setApplicationDescription(aboutData.shortDescription());
    parser.addHelpOption();
    parser.addVersionOption();

    // -s/--start session option
    const QCommandLineOption startSessionOption(QStringList() << QStringLiteral("s") << QStringLiteral("start"), i18n("Start Kate with a given session."), QStringLiteral("session"));
    parser.addOption(startSessionOption);

    // --startanon session option
    const QCommandLineOption startAnonymousSessionOption(QStringList() << QStringLiteral("startanon"), i18n("Start Kate with a new anonymous session, implies '-n'."));
    parser.addOption(startAnonymousSessionOption);

    // -n/--new option
    const QCommandLineOption startNewInstanceOption(QStringList() << QStringLiteral("n") << QStringLiteral("new"), i18n("Force start of a new kate instance (is ignored if start is used and another kate instance already has the given session opened), forced if no parameters and no URLs are given at all."));
    parser.addOption(startNewInstanceOption);

    // -b/--block option
    const QCommandLineOption startBlockingOption(QStringList() << QStringLiteral("b") << QStringLiteral("block"), i18n("If using an already running kate instance, block until it exits, if URLs given to open."));
    parser.addOption(startBlockingOption);

    // -p/--pid option
    const QCommandLineOption usePidOption(QStringList() << QStringLiteral("p") << QStringLiteral("pid"), i18n("Only try to reuse kate instance with this pid (is ignored if start is used and another kate instance already has the given session opened)."), QStringLiteral("pid"));
    parser.addOption(usePidOption);

    // -e/--encoding option
    const QCommandLineOption useEncodingOption(QStringList() << QStringLiteral("e") << QStringLiteral("encoding"), i18n("Set encoding for the file to open."), QStringLiteral("encoding"));
    parser.addOption(useEncodingOption);

    // -l/--line option
    const QCommandLineOption gotoLineOption(QStringList() << QStringLiteral("l") << QStringLiteral("line"), i18n("Navigate to this line."), QStringLiteral("line"));
    parser.addOption(gotoLineOption);

    // -c/--column option
    const QCommandLineOption gotoColumnOption(QStringList() << QStringLiteral("c") << QStringLiteral("column"), i18n("Navigate to this column."), QStringLiteral("column"));
    parser.addOption(gotoColumnOption);

    // -i/--stdin option
    const QCommandLineOption readStdInOption(QStringList() << QStringLiteral("i") << QStringLiteral("stdin"), i18n("Read the contents of stdin."));
    parser.addOption(readStdInOption);

    // --tempfile option
    const QCommandLineOption tempfileOption(QStringList() << QStringLiteral("tempfile"), i18n("The files/URLs opened by the application will be deleted after use"));
    parser.addOption(tempfileOption);

    // urls to open
    parser.addPositionalArgument(QStringLiteral("urls"), i18n("Documents to open."), QStringLiteral("[urls...]"));

    /**
     * do the command line parsing
     */
    parser.process(app);

    /**
     * handle standard options
     */
    aboutData.processCommandLine(&parser);

    /**
     * use dbus, if available
     * allows for resuse of running Kate instances
     */
    if (QDBusConnectionInterface * const sessionBusInterface = QDBusConnection::sessionBus().interface()) {
        /**
         * try to get the current running kate instances
         */
        KateRunningInstanceMap mapSessionRii;
        if (!fillinRunningKateAppInstances(&mapSessionRii)) {
            return 1;
        }

        QStringList kateServices;
        for (KateRunningInstanceMap::const_iterator it = mapSessionRii.constBegin(); it != mapSessionRii.constEnd(); ++it) {
            kateServices << (*it)->serviceName;
        }
        QString serviceName;

        const QStringList urls = parser.positionalArguments();

        bool force_new = parser.isSet(startNewInstanceOption);

        if (!force_new) {
            if (!(
                        parser.isSet(startSessionOption) ||
                        parser.isSet(startNewInstanceOption) ||
                        parser.isSet(usePidOption) ||
                        parser.isSet(useEncodingOption) ||
                        parser.isSet(gotoLineOption) ||
                        parser.isSet(gotoColumnOption) ||
                        parser.isSet(readStdInOption)
                    ) && (urls.isEmpty())) {
                force_new = true;
            }
        }

        QString start_session;
        bool session_already_opened = false;

        //check if we try to start an already opened session
        if (parser.isSet(startAnonymousSessionOption)) {
            force_new = true;
        } else if (parser.isSet(startSessionOption)) {
            start_session = parser.value(startSessionOption);
            if (mapSessionRii.contains(start_session)) {
                serviceName = mapSessionRii[start_session]->serviceName;
                force_new = false;
                session_already_opened = true;
            }
        }

        //cleanup map
        cleanupRunningKateAppInstanceMap(&mapSessionRii);

        //if no new instance is forced and no already opened session is requested,
        //check if a pid is given, which should be reused.
        // two possibilities: pid given or not...
        if ((!force_new) && serviceName.isEmpty()) {
            if ((parser.isSet(usePidOption)) || (!qgetenv("KATE_PID").isEmpty())) {
                QString usePid = (parser.isSet(usePidOption)) ?
                                parser.value(usePidOption) :
                                QString::fromLocal8Bit(qgetenv("KATE_PID"));

                serviceName = QStringLiteral("org.kde.kate-") + usePid;
                if (!kateServices.contains(serviceName)) {
                    serviceName.clear();
                }
            }
        }

        if ((!force_new) && (serviceName.isEmpty())) {
            if (kateServices.count() > 0) {
                serviceName = kateServices[0];
            }
        }

        //check again if service is still running
        bool foundRunningService = false;
        if (!serviceName.isEmpty()) {
            QDBusReply<bool> there = sessionBusInterface->isServiceRegistered(serviceName);
            foundRunningService = there.isValid() && there.value();
        }

        if (foundRunningService) {
            // open given session
            if (parser.isSet(startSessionOption) && (!session_already_opened)) {
                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                QStringLiteral("/MainApplication"), QStringLiteral("org.kde.Kate.Application"), QStringLiteral("activateSession"));

                QList<QVariant> dbusargs;
                dbusargs.append(parser.value(startSessionOption));
                m.setArguments(dbusargs);

                QDBusConnection::sessionBus().call(m);
            }

            QString enc = parser.isSet(useEncodingOption) ? parser.value(useEncodingOption) : QString();

            bool tempfileSet = parser.isSet(tempfileOption);

            // only block, if files to open there....
            bool needToBlock = parser.isSet(startBlockingOption) && !urls.isEmpty();

            QStringList tokens;

            // open given files...
            foreach(const QString & url, urls) {
                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                QStringLiteral("/MainApplication"), QStringLiteral("org.kde.Kate.Application"), QStringLiteral("tokenOpenUrlAt"));

                UrlInfo info(url);
                QList<QVariant> dbusargs;

                // convert to an url
                dbusargs.append(info.url.toString());
                dbusargs.append(info.cursor.line());
                dbusargs.append(info.cursor.column());
                dbusargs.append(enc);
                dbusargs.append(tempfileSet);
                m.setArguments(dbusargs);

                QDBusMessage res = QDBusConnection::sessionBus().call(m);
                if (res.type() == QDBusMessage::ReplyMessage) {
                    if (res.arguments().count() == 1) {
                        QVariant v = res.arguments()[0];
                        if (v.isValid()) {
                            QString s = v.toString();
                            if ((!s.isEmpty()) && (s != QStringLiteral("ERROR"))) {
                                tokens << s;
                            }
                        }
                    }
                }
            }

            if (parser.isSet(readStdInOption)) {
                QTextStream input(stdin, QIODevice::ReadOnly);

                // set chosen codec
                QTextCodec *codec = parser.isSet(useEncodingOption) ?
                                    QTextCodec::codecForName(parser.value(useEncodingOption).toUtf8()) : 0;

                if (codec) {
                    input.setCodec(codec);
                }

                QString line;
                QString text;

                do {
                    line = input.readLine();
                    text.append(line + QLatin1Char('\n'));
                } while (!line.isNull());

                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                QStringLiteral("/MainApplication"), QStringLiteral("org.kde.Kate.Application"), QStringLiteral("openInput"));

                QList<QVariant> dbusargs;
                dbusargs.append(text);
                m.setArguments(dbusargs);

                QDBusConnection::sessionBus().call(m);
            }

            int line = 0;
            int column = 0;
            bool nav = false;

            if (parser.isSet(gotoLineOption)) {
                line = parser.value(gotoLineOption).toInt() - 1;
                nav = true;
            }

            if (parser.isSet(gotoColumnOption)) {
                column = parser.value(gotoColumnOption).toInt() - 1;
                nav = true;
            }

            if (nav) {
                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                QStringLiteral("/MainApplication"), QStringLiteral("org.kde.Kate.Application"), QStringLiteral("setCursor"));

                QList<QVariant> args;
                args.append(line);
                args.append(column);
                m.setArguments(args);

                QDBusConnection::sessionBus().call(m);
            }

            // activate the used instance
            QDBusMessage activateMsg = QDBusMessage::createMethodCall(serviceName,
                                    QStringLiteral("/MainApplication"), QStringLiteral("org.kde.Kate.Application"), QStringLiteral("activate"));
            QDBusConnection::sessionBus().call(activateMsg);

            // connect dbus signal
            if (needToBlock) {
                KateWaiter *waiter = new KateWaiter(serviceName, tokens);
                QDBusConnection::sessionBus().connect(serviceName, QStringLiteral("/MainApplication"), QStringLiteral("org.kde.Kate.Application"), QStringLiteral("exiting"), waiter, SLOT(exiting()));
                QDBusConnection::sessionBus().connect(serviceName, QStringLiteral("/MainApplication"), QStringLiteral("org.kde.Kate.Application"), QStringLiteral("documentClosed"), waiter, SLOT(documentClosed(QString)));
            }

            // KToolInvocation (and KRun) will wait until we register on dbus
            KDBusService dbusService(KDBusService::Multiple);
            dbusService.unregister();

            // make the world happy, we are started, kind of...
            KStartupInfo::appStarted();

            // this will wait until exiting is emitted by the used instance, if wanted...
            return needToBlock ? app.exec() : 0;
        }
    }

    /**
     * if we arrive here, we need to start a new kate instance!
     */

    /**
     * construct the real kate app object ;)
     * behaves like a singleton, one unique instance
     * we are passing our local command line parser to it
     */
    KateApp kateApp(parser);

    /**
     * init kate
     * if this returns false, we shall exit
     * else we may enter the main event loop
     */
    if (!kateApp.init()) {
        return 0;
    }

    /**
     * finally register this kate instance for dbus
     */
    const KDBusService dbusService(KDBusService::Multiple);

    /**
     * start main event loop for our application
     */
    return app.exec();
}
