/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001-2022 Christoph Cullmann <cullmann@kde.org>
 * SPDX-FileCopyrightText: 2001-2002 Joseph Wenninger <jowenn@kde.org>
 * SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateapp.h"

#include <KAboutData>
#include <KLocalizedString>

// X11 startup handling
#if __has_include(<KStartupInfo>)
#include <KStartupInfo>
#endif

#include <KWindowSystem>
#include <algorithm>

#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QIcon>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSessionManager>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QVariant>

#include <qglobal.h>
#include <urlinfo.h>

// X11 startup handling
#define HAVE_X11 __has_include(<KX11Extras>)
#if HAVE_X11
#include <KX11Extras>
#include <private/qtx11extras_p.h>
#endif

#ifdef WITH_DBUS
#include "katerunninginstanceinfo.h"
#include "katewaiter.h"
#include <KDBusService>
#include <QDBusMessage>
#include <QDBusReply>
#endif

#include "SingleApplication/SingleApplication"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

Q_LOGGING_CATEGORY(KateTime, "kate.time", QtWarningMsg)

// helper to get activation token for second instance
static QString activationToken()
{
    if (KWindowSystem::isPlatformWayland()) {
        return qEnvironmentVariable("XDG_ACTIVATION_TOKEN");
    }

#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        return QString::fromUtf8(QX11Info::nextStartupId());
    }
#endif

    return {};
}

int main(int argc, char **argv)
{
    QElapsedTimer timer;
    timer.start();

    /**
     * fork into the background if we don't need to be blocking
     * we need to do that early
     */
    bool detach = true;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--block") || !strcmp(argv[i], "-i") || !strcmp(argv[i], "--stdin") || !(strcmp(argv[i], "-v"))
            || !(strcmp(argv[i], "--version")) || !(strcmp(argv[i], "-h")) || !(strcmp(argv[i], "--help")) || !(strcmp(argv[i], "--help-all"))
            || !(strcmp(argv[i], "--author")) || !(strcmp(argv[i], "--license"))) {
            detach = false;
            break;
        }
    }

    /**
     * Do all needed pre-application init steps, shared between Kate and KWrite
     */
    KateApp::initPreApplicationCreation(detach);

    /**
     * Create application first
     * For DBus just a normal application
     */
    QApplication app(argc, argv);
    qCDebug(KateTime, "QApplication initialized in %lld ms", timer.elapsed());
    timer.restart();

    /**
     * Enforce application name even if the executable is renamed
     * Connect application with translation catalogs, Kate & KWrite share the same one
     */
    QApplication::setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));

    /**
     * construct about data for Kate
     */
    KAboutData aboutData(QStringLiteral("kate"),
                         i18n("Kate"),
                         QStringLiteral(KATE_VERSION),
                         i18n("Kate - Advanced Text Editor"),
                         KAboutLicense::LGPL_V2,
                         i18n("(c) 2000-2024 The Kate Authors"),
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
    KateApp::initPostApplicationCreation(aboutData);

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
    const QCommandLineOption startSessionOption(QStringList{QStringLiteral("s"), QStringLiteral("start")},
                                                i18n("Start Kate with a given session."),
                                                i18n("session"));
    parser.addOption(startSessionOption);

    // --startanon session option
    const QCommandLineOption startAnonymousSessionOption(QStringList{QStringLiteral("startanon")},
                                                         i18n("Start Kate with a new anonymous session, implies '-n'."));
    parser.addOption(startAnonymousSessionOption);

    // -n/--new option
    const QCommandLineOption startNewInstanceOption(QStringList{QStringLiteral("n"), QStringLiteral("new")},
                                                    i18n("Force start of a new kate instance (is ignored if start is used and another kate instance already "
                                                         "has the given session opened), forced if no parameters and no URLs are given at all."));
    parser.addOption(startNewInstanceOption);

    // -b/--block option
    const QCommandLineOption startBlockingOption(QStringList{QStringLiteral("b"), QStringLiteral("block")},
                                                 i18n("If using an already running kate instance, block until it exits, if URLs given to open."));
    parser.addOption(startBlockingOption);

    // -p/--pid option
    const QCommandLineOption usePidOption(
        QStringList{QStringLiteral("p"), QStringLiteral("pid")},
        i18n("Only try to reuse kate instance with this pid (is ignored if start is used and another kate instance already has the given session opened)."),
        i18n("pid"));
    parser.addOption(usePidOption);

    // -e/--encoding option
    const QCommandLineOption useEncodingOption(QStringList{QStringLiteral("e"), QStringLiteral("encoding")},
                                               i18n("Set encoding for the file to open."),
                                               i18n("encoding"));
    parser.addOption(useEncodingOption);

    // -l/--line option
    const QCommandLineOption gotoLineOption(QStringList{QStringLiteral("l"), QStringLiteral("line")}, i18n("Navigate to this line."), i18n("line"));
    parser.addOption(gotoLineOption);

    // -c/--column option
    const QCommandLineOption gotoColumnOption(QStringList{QStringLiteral("c"), QStringLiteral("column")}, i18n("Navigate to this column."), i18n("column"));
    parser.addOption(gotoColumnOption);

    // -i/--stdin option
    const QCommandLineOption readStdInOption(QStringList{QStringLiteral("i"), QStringLiteral("stdin")}, i18n("Read the contents of stdin."));
    parser.addOption(readStdInOption);

    // --tempfile option
    const QCommandLineOption tempfileOption(QStringList{QStringLiteral("tempfile")},
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
    QStringList urls = parser.positionalArguments();

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
     * we will later try SingleApplication if dbus is no option
     */
    bool dbusNotThere = true;

#ifdef WITH_DBUS
    /**
     * use dbus, if available for linux and co.
     * allows for reuse of running Kate instances
     */
    if (const auto sessionBusInterface = QDBusConnection::sessionBus().interface()) {
        // we have dbus, don't try single application
        dbusNotThere = false;

        /**
         * try to get the current running kate instances
         */
        const auto kateServices = fillinRunningKateAppInstances();

        QString serviceName;
        QString start_session;
        bool session_already_opened = false;

        // check if we try to start an already opened session
        if (parser.isSet(startAnonymousSessionOption)) {
            force_new = true;
        } else if (parser.isSet(startSessionOption)) {
            start_session = parser.value(startSessionOption);
            const auto it = std::find_if(kateServices.cbegin(), kateServices.cend(), [&start_session](const auto &instance) {
                return instance.sessionName == start_session;
            });
            if (it != kateServices.end()) {
                serviceName = it->serviceName;
                force_new = false;
                session_already_opened = true;
            }
        }

        // if no new instance is forced and no already opened session is requested,
        // check if a pid is given, which should be reused.
        // two possibilities: pid given or not...
        if ((!force_new) && serviceName.isEmpty()) {
            if ((parser.isSet(usePidOption)) || (!qEnvironmentVariableIsEmpty("KATE_PID"))) {
                QString usePid = (parser.isSet(usePidOption)) ? parser.value(usePidOption) : qEnvironmentVariable("KATE_PID");

                serviceName = QLatin1String("org.kde.kate-") + usePid;
                const auto it = std::find_if(kateServices.cbegin(), kateServices.cend(), [&serviceName](const auto &instance) {
                    return instance.serviceName == serviceName;
                });
                if (it == kateServices.end()) {
                    serviceName.clear();
                }
            }
        }

        // prefer the Kate instance that got activated last, lastActivationChange is negative for wrong virtual desktop on X11
        // the start with lastUsedChosen = 0 will therefore filter that out, see bug 486066
        bool foundRunningService = false;
        if (!force_new && serviceName.isEmpty()) {
            qint64 lastUsedChosen = 0;
            for (const auto &currentService : kateServices) {
                if (currentService.lastActivationChange > lastUsedChosen) {
                    serviceName = currentService.serviceName;
                    lastUsedChosen = currentService.lastActivationChange;
                }
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

                QVariantList dbusargs;
                dbusargs.append(parser.value(startSessionOption));
                m.setArguments(dbusargs);

                QDBusConnection::sessionBus().call(m);
            }

            QString enc = parser.isSet(useEncodingOption) ? parser.value(useEncodingOption) : QString();

            bool tempfileSet = parser.isSet(tempfileOption);

            QStringList tokens;

            // support +xyz line number as first argument
            KTextEditor::Cursor firstLineNumberArgument = UrlInfo::parseLineNumberArgumentAndRemoveIt(urls);

            // open given files...
            for (UrlInfo info : std::as_const(urls)) {
                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                                                QStringLiteral("/MainApplication"),
                                                                QStringLiteral("org.kde.Kate.Application"),
                                                                QStringLiteral("tokenOpenUrlAt"));

                if (!info.cursor.isValid() && firstLineNumberArgument.isValid()) {
                    // same semantics as e.g. in mcedit, just use for the first file
                    info.cursor = firstLineNumberArgument;
                    firstLineNumberArgument = KTextEditor::Cursor::invalid();
                }

                QVariantList dbusargs;

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
                        QVariant v = res.arguments().constFirst();
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
                const QString codec_name = parser.isSet(QStringLiteral("encoding")) ? parser.value(QStringLiteral("encoding")) : QString();

                QFile input;
                if (!input.open(stdin, QIODevice::ReadOnly)) {
                    std::ignore = fprintf(stderr, "Error: Failed to open stdin\n");
                }
                auto decoder = QStringDecoder(codec_name.toUtf8().constData());
                QString text = decoder.isValid() ? decoder.decode(input.readAll()) : QString::fromLocal8Bit(input.readAll());

                // normalize line endings, to e.g. catch issues with \r\n on Windows
                static const auto lineEndingsRE = QRegularExpression(QStringLiteral("\r\n?"));
                text.replace(lineEndingsRE, QStringLiteral("\n"));

                QDBusMessage m = QDBusMessage::createMethodCall(serviceName,
                                                                QStringLiteral("/MainApplication"),
                                                                QStringLiteral("org.kde.Kate.Application"),
                                                                QStringLiteral("openInput"));

                QVariantList dbusargs;
                dbusargs.append(text);
                dbusargs.append(codec_name);
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

                QVariantList args;
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
            activateMsg.setArguments({activationToken()});
            QDBusConnection::sessionBus().call(activateMsg);

            // connect dbus signal
            if (needToBlock) {
                auto *waiter = new KateWaiter(serviceName, tokens);
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

#if __has_include(<KStartupInfo>)
            // make the world happy, we are started, kind of...
            KStartupInfo::appStarted();
#endif

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
            return needToBlock ? QApplication::exec() : 0;
        }
    }
#endif

    /**
     * if we had no DBus session bus, we can try to use the SingleApplication communication.
     * only try to reuse existing kate instances if not already forbidden by arguments
     */
    std::unique_ptr<SingleApplication> singleApplicationInstance;
    if (dbusNotThere) {
        singleApplicationInstance = std::make_unique<SingleApplication>(argc, argv, true);
        if (!force_new && singleApplicationInstance->isSecondary()) {
#if defined(Q_OS_WIN)
            // allow primary instance to raise its window
            AllowSetForegroundWindow(singleApplicationInstance->primaryPid());
#endif

            // support +xyz line number as first argument
            KTextEditor::Cursor firstLineNumberArgument = UrlInfo::parseLineNumberArgumentAndRemoveIt(urls);

            /**
             * construct one big message with all urls to open
             * later we will add additional data to this
             */
            QVariantMap message;
            QVariantList messageUrls;
            for (UrlInfo info : std::as_const(urls)) {
                if (!info.cursor.isValid() && firstLineNumberArgument.isValid()) {
                    // same semantics as e.g. in mcedit, just use for the first file
                    info.cursor = firstLineNumberArgument;
                    firstLineNumberArgument = KTextEditor::Cursor::invalid();
                }

                /**
                 * pack info into the message as extra element in urls list
                 */
                QVariantMap urlMessagePart;
                urlMessagePart[QStringLiteral("url")] = info.url;
                urlMessagePart[QStringLiteral("line")] = info.cursor.line();
                urlMessagePart[QStringLiteral("column")] = info.cursor.column();
                messageUrls.append(urlMessagePart);
            }
            message[QStringLiteral("urls")] = messageUrls;

            // add activation token
            message[QStringLiteral("activationToken")] = activationToken();

            /**
             * try to send message, return success
             */
            return !singleApplicationInstance->sendMessage(QJsonDocument::fromVariant(QVariant(message)).toJson(),
                                                           1000,
                                                           needToBlock ? SingleApplication::BlockUntilPrimaryExit : SingleApplication::NonBlocking);
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
    KateApp kateApp(parser, KateApp::ApplicationKate, QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kate/sessions"));

    /**
     * init kate
     * if this returns false, we shall exit
     * else we may enter the main event loop
     */
    if (!kateApp.init()) {
        return 0;
    }
    qCDebug(KateTime, "KateApp initialized in %lld ms", timer.elapsed());

#ifdef WITH_DBUS
    /**
     * finally register this kate instance for dbus, don't die if no dbus is around!
     */
    const KDBusService dbusService(KDBusService::Multiple | KDBusService::NoExitOnFailure);
#endif

    /**
     * listen to single application messages if no DBus
     */
    if (singleApplicationInstance) {
        QObject::connect(singleApplicationInstance.get(), &SingleApplication::receivedMessage, &kateApp, &KateApp::remoteMessageReceived);
    }

    /**
     * start main event loop for our application
     */
    return QApplication::exec();
}
