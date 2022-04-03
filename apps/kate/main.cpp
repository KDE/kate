/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001-2022 Christoph Cullmann <cullmann@kde.org>
 * SPDX-FileCopyrightText: 2001-2002 Joseph Wenninger <jowenn@kde.org>
 * SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "config.h"

#include "kateapp.h"
#include "katerunninginstanceinfo.h"
#include "katewaiter.h"

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>
#include <KStartupInfo>
#include <KWindowSystem>
#include <algorithm>

#include <QApplication>
#include <QByteArray>
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDir>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSessionManager>
#include <QTextCodec>
#include <QUrl>
#include <QVariant>

#include <qglobal.h>
#include <urlinfo.h>

#include "SingleApplication/SingleApplication"

#ifndef Q_OS_WIN
#include <unistd.h>
#endif
#include <iostream>

int main(int argc, char **argv)
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_HAIKU)
    // Prohibit using sudo or kdesu (but allow using the root user directly)
    if (getuid() == 0) {
        if (!qEnvironmentVariableIsEmpty("SUDO_USER")) {
            std::cout << "Running Kate with sudo can cause bugs and expose you to security vulnerabilities. "
                         "Instead use Kate normally and you will be prompted for elevated privileges when "
                         "saving documents if needed."
                      << std::endl;
            return EXIT_FAILURE;
        } else if (!qEnvironmentVariableIsEmpty("KDESU_USER")) {
            std::cout << "Running Kate with kdesu can cause bugs and expose you to security vulnerabilities. "
                         "Instead use Kate normally and you will be prompted for elevated privileges when "
                         "saving documents if needed."
                      << std::endl;
            return EXIT_FAILURE;
        }
    }
#endif
    /**
     * init resources from our static lib
     */
    Q_INIT_RESOURCE(kate);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    /**
     * enable high dpi support
     */
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif

    /**
     * allow fractional scaling
     * we only activate this on Windows, it seems to creates problems on unices
     * (and there the fractional scaling with the QT_... env vars as set by KScreen works)
     * see bug 416078
     *
     * we switched to Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor because of font rendering issues
     * we follow what Krita does here, see https://invent.kde.org/graphics/krita/-/blob/master/krita/main.cc
     * we raise the Qt requirement to  5.15 as it seems some patches went in after 5.14 that are needed
     * see Krita comments, too
     */
#if defined(Q_OS_WIN)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#endif

    /**
     * Create application first
     * We always use a single application that allows to start multiple instances.
     * This allows for communication even without DBus and better testing of these code paths.
     */
    SingleApplication app(argc, argv, true);
    app.setApplicationName(QStringLiteral("kate"));

    /**
     * Connect application with translation catalogs
     */
    KLocalizedString::setApplicationDomain("kate");

    /**
     * construct about data for Kate
     */
    KAboutData aboutData(QStringLiteral("kate"),
                         i18n("Kate"),
                         QStringLiteral(KATE_VERSION),
                         i18n("Kate - Advanced Text Editor"),
                         KAboutLicense::LGPL_V2,
                         i18n("(c) 2000-2022 The Kate Authors"),
                         // use the other text field to get our mascot into the about dialog
                         QStringLiteral("<img height=\"362\" width=\"512\" src=\":/kate/mascot.png\"/>"),
                         QStringLiteral("https://kate-editor.org"));

    /**
     * right dbus prefix == org.kde.
     */
    aboutData.setOrganizationDomain("kde.org");

    /**
     * desktop file association to make application icon work (e.g. in Wayland window decoration)
     */
    aboutData.setDesktopFileName(QStringLiteral("org.kde.kate"));

    /**
     * authors & co.
     * add yourself there, if you helped to work on Kate or KWrite
     */
    KateApp::fillAuthorsAndCredits(aboutData);

    /**
     * set proper Kate icon for our about dialog
     */
    aboutData.setProgramLogo(QIcon(QStringLiteral(":/kate/kate.svg")));

    /**
     * set and register app about data
     */
    KAboutData::setApplicationData(aboutData);

    /**
     * set the program icon
     */
#ifndef Q_OS_MACOS // skip this on macOS to have proper mime-type icon visible
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/kate/kate.svg")));
#endif

    /**
     * Create command line parser and feed it with known options
     */
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    // -s/--start session option
    const QCommandLineOption startSessionOption(QStringList() << QStringLiteral("s") << QStringLiteral("start"),
                                                i18n("Start Kate with a given session."),
                                                i18n("session"));
    parser.addOption(startSessionOption);

    // --startanon session option
    const QCommandLineOption startAnonymousSessionOption(QStringList() << QStringLiteral("startanon"),
                                                         i18n("Start Kate with a new anonymous session, implies '-n'."));
    parser.addOption(startAnonymousSessionOption);

    // -n/--new option
    const QCommandLineOption startNewInstanceOption(QStringList() << QStringLiteral("n") << QStringLiteral("new"),
                                                    i18n("Force start of a new kate instance (is ignored if start is used and another kate instance already "
                                                         "has the given session opened), forced if no parameters and no URLs are given at all."));
    parser.addOption(startNewInstanceOption);

    // -b/--block option
    const QCommandLineOption startBlockingOption(QStringList() << QStringLiteral("b") << QStringLiteral("block"),
                                                 i18n("If using an already running kate instance, block until it exits, if URLs given to open."));
    parser.addOption(startBlockingOption);

    // -p/--pid option
    const QCommandLineOption usePidOption(
        QStringList() << QStringLiteral("p") << QStringLiteral("pid"),
        i18n("Only try to reuse kate instance with this pid (is ignored if start is used and another kate instance already has the given session opened)."),
        i18n("pid"));
    parser.addOption(usePidOption);

    // -e/--encoding option
    const QCommandLineOption useEncodingOption(QStringList() << QStringLiteral("e") << QStringLiteral("encoding"),
                                               i18n("Set encoding for the file to open."),
                                               i18n("encoding"));
    parser.addOption(useEncodingOption);

    // -l/--line option
    const QCommandLineOption gotoLineOption(QStringList() << QStringLiteral("l") << QStringLiteral("line"), i18n("Navigate to this line."), i18n("line"));
    parser.addOption(gotoLineOption);

    // -c/--column option
    const QCommandLineOption gotoColumnOption(QStringList() << QStringLiteral("c") << QStringLiteral("column"),
                                              i18n("Navigate to this column."),
                                              i18n("column"));
    parser.addOption(gotoColumnOption);

    // -i/--stdin option
    const QCommandLineOption readStdInOption(QStringList() << QStringLiteral("i") << QStringLiteral("stdin"), i18n("Read the contents of stdin."));
    parser.addOption(readStdInOption);

    // --tempfile option
    const QCommandLineOption tempfileOption(QStringList() << QStringLiteral("tempfile"),
                                            i18n("The files/URLs opened by the application will be deleted after use"));
    parser.addOption(tempfileOption);

    // urls to open
    parser.addPositionalArgument(QStringLiteral("urls"), i18n("Documents to open."), i18n("[urls...]"));

    /**
     * do the command line parsing
     */
    parser.process(app);

    /**
     * handle standard options
     */
    aboutData.processCommandLine(&parser);

    /**
     * remember the urls we shall open
     */
    const QStringList urls = parser.positionalArguments();

    /**
     * compute if we shall start a new instance or reuse
     * an old one
     * this will later be updated once more after detecting some
     * things about already running kate's, like their sessions
     */
    bool force_new = parser.isSet(startNewInstanceOption);
    if (!force_new) {
        if (!(parser.isSet(startSessionOption) || parser.isSet(startNewInstanceOption) || parser.isSet(usePidOption) || parser.isSet(useEncodingOption)
              || parser.isSet(gotoLineOption) || parser.isSet(gotoColumnOption) || parser.isSet(readStdInOption))
            && (urls.isEmpty())) {
            force_new = true;
        } else {
            force_new = std::any_of(urls.begin(), urls.end(), [](const QString &url) {
                return QFileInfo(url).isDir();
            });
        }
    }

    /**
     * only block, if files to open there....
     */
    const bool needToBlock = parser.isSet(startBlockingOption) && !urls.isEmpty();

    /**
     * use dbus, if available for linux and co.
     * allows for reuse of running Kate instances
     * we have some env var to forbid this for easier testing of the single application code paths: KATE_SKIP_DBUS
     */
    if (QDBusConnectionInterface *const sessionBusInterface = QDBusConnection::sessionBus().interface();
        sessionBusInterface && qEnvironmentVariableIsEmpty("KATE_SKIP_DBUS")) {
        /**
         * try to get the current running kate instances
         */
        KateRunningInstanceMap mapSessionRii;
        if (!fillinRunningKateAppInstances(&mapSessionRii)) {
            return 1;
        }

        QString currentActivity;
        QDBusMessage m = QDBusMessage::createMethodCall(QStringLiteral("org.kde.ActivityManager"),
                                                        QStringLiteral("/ActivityManager/Activities"),
                                                        QStringLiteral("org.kde.ActivityManager.Activities"),
                                                        QStringLiteral("CurrentActivity"));
        QDBusMessage res = QDBusConnection::sessionBus().call(m);
        QList<QVariant> answer = res.arguments();
        if (answer.size() == 1) {
            currentActivity = answer.at(0).toString();
        }

        QStringList kateServices;
        for (const auto &[_, katerunninginstanceinfo] : mapSessionRii) {
            Q_UNUSED(_)
            QString serviceName = katerunninginstanceinfo.serviceName;

            if (currentActivity.length() != 0) {
                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                                                QStringLiteral("/MainApplication"),
                                                                QStringLiteral("org.kde.Kate.Application"),
                                                                QStringLiteral("isOnActivity"));

                QList<QVariant> dbargs;

                // convert to an url
                dbargs.append(currentActivity);
                m.setArguments(dbargs);

                QDBusMessage res = QDBusConnection::sessionBus().call(m);
                QList<QVariant> answer = res.arguments();
                if (answer.size() == 1) {
                    const bool canBeUsed = answer.at(0).toBool();

                    // If the Kate instance is in a specific activity, add it to
                    // the list of candidate reusable services
                    if (canBeUsed) {
                        kateServices << serviceName;
                    }
                }
            } else {
                kateServices << serviceName;
            }
        }

        QString serviceName;

        QString start_session;
        bool session_already_opened = false;

        // check if we try to start an already opened session
        if (parser.isSet(startAnonymousSessionOption)) {
            force_new = true;
        } else if (parser.isSet(startSessionOption)) {
            start_session = parser.value(startSessionOption);
            auto it = mapSessionRii.find(start_session);
            if (it != mapSessionRii.end()) {
                serviceName = it->second.serviceName;
                force_new = false;
                session_already_opened = true;
            }
        }

        // if no new instance is forced and no already opened session is requested,
        // check if a pid is given, which should be reused.
        // two possibilities: pid given or not...
        if ((!force_new) && serviceName.isEmpty()) {
            if ((parser.isSet(usePidOption)) || (!qEnvironmentVariableIsEmpty("KATE_PID"))) {
                QString usePid = (parser.isSet(usePidOption)) ? parser.value(usePidOption) : QString::fromLocal8Bit(qgetenv("KATE_PID"));

                serviceName = QLatin1String("org.kde.kate-") + usePid;
                if (!kateServices.contains(serviceName)) {
                    serviceName.clear();
                }
            }
        }

        // prefer the Kate instance running on the current virtual desktop
        bool foundRunningService = false;
        if ((!force_new) && (serviceName.isEmpty())) {
            const int desktopnumber = KWindowSystem::currentDesktop();
            for (int s = 0; s < kateServices.count(); s++) {
                serviceName = kateServices[s];

                if (!serviceName.isEmpty()) {
                    QDBusReply<bool> there = sessionBusInterface->isServiceRegistered(serviceName);

                    if (there.isValid() && there.value()) {
                        // query instance current desktop
                        QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                                                        QStringLiteral("/MainApplication"),
                                                                        QStringLiteral("org.kde.Kate.Application"),
                                                                        QStringLiteral("desktopNumber"));

                        QDBusMessage res = QDBusConnection::sessionBus().call(m);
                        QList<QVariant> answer = res.arguments();
                        if (answer.size() == 1) {
                            // special case: on all desktops! that is -1 aka NET::OnAllDesktops, see KWindowInfo::desktop() docs
                            const int sessionDesktopNumber = answer.at(0).toInt();
                            if (sessionDesktopNumber == desktopnumber || sessionDesktopNumber == NET::OnAllDesktops) {
                                // stop searching. a candidate instance in the current desktop has been found
                                foundRunningService = true;
                                break;
                            }
                        }
                    }
                }
                serviceName.clear();
            }
        }

        // check again if service is still running
        foundRunningService = false;
        if (!serviceName.isEmpty()) {
            QDBusReply<bool> there = sessionBusInterface->isServiceRegistered(serviceName);
            foundRunningService = there.isValid() && there.value();
        }

        if (foundRunningService) {
            // open given session
            if (parser.isSet(startSessionOption) && (!session_already_opened)) {
                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                                                QStringLiteral("/MainApplication"),
                                                                QStringLiteral("org.kde.Kate.Application"),
                                                                QStringLiteral("activateSession"));

                QList<QVariant> dbusargs;
                dbusargs.append(parser.value(startSessionOption));
                m.setArguments(dbusargs);

                QDBusConnection::sessionBus().call(m);
            }

            QString enc = parser.isSet(useEncodingOption) ? parser.value(useEncodingOption) : QString();

            bool tempfileSet = parser.isSet(tempfileOption);

            QStringList tokens;

            // open given files...
            for (int i = 0; i < urls.size(); ++i) {
                const QString &url = urls[i];
                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                                                QStringLiteral("/MainApplication"),
                                                                QStringLiteral("org.kde.Kate.Application"),
                                                                QStringLiteral("tokenOpenUrlAt"));

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
                            if ((!s.isEmpty()) && (s != QLatin1String("ERROR"))) {
                                tokens << s;
                            }
                        }
                    }
                }
            }

            if (parser.isSet(readStdInOption)) {
                // set chosen codec
                QTextCodec *codec = parser.isSet(useEncodingOption) ? QTextCodec::codecForName(parser.value(useEncodingOption).toUtf8()) : nullptr;

                QFile input;
                input.open(stdin, QIODevice::ReadOnly);
                QString text = codec ? codec->toUnicode(input.readAll()) : QString::fromLocal8Bit(input.readAll());

                // normalize line endings, to e.g. catch issues with \r\n on Windows
                text.replace(QRegularExpression(QStringLiteral("\r\n?")), QStringLiteral("\n"));

                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                                                QStringLiteral("/MainApplication"),
                                                                QStringLiteral("org.kde.Kate.Application"),
                                                                QStringLiteral("openInput"));

                QList<QVariant> dbusargs;
                dbusargs.append(text);
                dbusargs.append(codec ? QString::fromLatin1(codec->name()) : QString());
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
                                                                QStringLiteral("/MainApplication"),
                                                                QStringLiteral("org.kde.Kate.Application"),
                                                                QStringLiteral("setCursor"));

                QList<QVariant> args;
                args.append(line);
                args.append(column);
                m.setArguments(args);

                QDBusConnection::sessionBus().call(m);
            }

            // activate the used instance
            QDBusMessage activateMsg = QDBusMessage::createMethodCall(serviceName,
                                                                      QStringLiteral("/MainApplication"),
                                                                      QStringLiteral("org.kde.Kate.Application"),
                                                                      QStringLiteral("activate"));
            activateMsg.setArguments({qEnvironmentVariable("XDG_ACTIVATION_TOKEN")});
            QDBusConnection::sessionBus().call(activateMsg);

            // connect dbus signal
            if (needToBlock) {
                KateWaiter *waiter = new KateWaiter(serviceName, tokens);
                QDBusConnection::sessionBus().connect(serviceName,
                                                      QStringLiteral("/MainApplication"),
                                                      QStringLiteral("org.kde.Kate.Application"),
                                                      QStringLiteral("exiting"),
                                                      waiter,
                                                      SLOT(exiting()));
                QDBusConnection::sessionBus().connect(serviceName,
                                                      QStringLiteral("/MainApplication"),
                                                      QStringLiteral("org.kde.Kate.Application"),
                                                      QStringLiteral("documentClosed"),
                                                      waiter,
                                                      SLOT(documentClosed(QString)));
            }

            // KToolInvocation (and KRun) will wait until we register on dbus
            KDBusService dbusService(KDBusService::Multiple);
            dbusService.unregister();

            // make the world happy, we are started, kind of...
            KStartupInfo::appStarted();

            // We don't want the session manager to restart us on next login
            // if we block
            if (needToBlock) {
                QObject::connect(
                    qApp,
                    &QGuiApplication::saveStateRequest,
                    qApp,
                    [](QSessionManager &session) {
                        session.setRestartHint(QSessionManager::RestartNever);
                    },
                    Qt::DirectConnection);
            }

            // this will wait until exiting is emitted by the used instance, if wanted...
            return needToBlock ? app.exec() : 0;
        }
    }

    /**
     * if we had no DBus session bus, we can try to use the SingleApplication communication.
     * only try to reuse existing kate instances if not already forbidden by arguments
     */
    else if (!force_new && app.isSecondary()) {
        /**
         * construct one big message with all urls to open
         * later we will add additional data to this
         */
        QVariantMap message;
        QVariantList messageUrls;
        for (const QString &url : urls) {
            /**
             * get url info and pack them into the message as extra element in urls list
             */
            UrlInfo info(url);
            QVariantMap urlMessagePart;
            urlMessagePart[QLatin1String("url")] = info.url;
            urlMessagePart[QLatin1String("line")] = info.cursor.line();
            urlMessagePart[QLatin1String("column")] = info.cursor.column();
            messageUrls.append(urlMessagePart);
        }
        message[QLatin1String("urls")] = messageUrls;

        /**
         * try to send message, return success
         */
        return !app.sendMessage(QJsonDocument::fromVariant(QVariant(message)).toJson(), 1000, needToBlock);
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
     * finally register this kate instance for dbus, don't die if no dbus is around!
     */
    const KDBusService dbusService(KDBusService::Multiple | KDBusService::NoExitOnFailure);

    /**
     * listen to single application messages in any case
     */
    QObject::connect(&app, &SingleApplication::receivedMessage, &kateApp, &KateApp::remoteMessageReceived);

    /**
     * start main event loop for our application
     */
    return app.exec();
}
