/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateapp.h"

#include "doc_or_widget.h"
#include "kate_timings_debug.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#ifdef WITH_DBUS
#include "kateappadaptor.h"
#endif

#include <KAboutData>
#include <KConfigGui>
#include <KCrash>
#include <KIconTheme>
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNetworkMounts>
#include <KSandbox>
#include <KSharedConfig>
#include <KTextEditor/View>
#include <KWindowSystem>

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

#ifdef WITH_KUSERFEEDBACK
#include <KUserFeedback/Provider>
#endif

// signal handler for SIGINT & SIGTERM
#ifdef Q_OS_UNIX
#include <KSignalHandler>
#include <signal.h>
#include <unistd.h>
#endif

// X11 startup handling
#define HAVE_X11 __has_include(<KStartupInfo>)
#if HAVE_X11
#include <KStartupInfo>
#include <KWindowInfo>
#include <KX11Extras>
#endif

#ifdef WITH_KUSERFEEDBACK
#include <KUserFeedback/ApplicationVersionSource>
#include <KUserFeedback/PlatformInfoSource>
#include <KUserFeedback/QtVersionSource>
#include <KUserFeedback/ScreenInfoSource>
#include <KUserFeedback/StartCountSource>
#include <KUserFeedback/UsageTimeSource>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QTimer>
#include <QUrlQuery>

#ifdef WITH_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#endif

#include <urlinfo.h>

#ifndef Q_OS_WIN
#include <unistd.h>
#ifndef Q_OS_HAIKU
#include <libintl.h>
#endif
#endif

#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef HAVE_CTERMID
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifdef HAVE_DAEMON
#include <unistd.h>
#endif

#if defined(Q_OS_MACOS)
#include <sys/sysctl.h>
#endif

// remember if we were started inside a terminal
static bool insideTerminal = false;

#ifdef WITH_DBUS
/**
 * dbus interface, must survive longer than m_docManager
 * e.g. the destroyed signal of the document might access this
 * global static as we register one on the qApp instance
 * Qt docs tell one shall not delete it, it will disappear with it's parent
 */
static KateAppAdaptor *adaptor = nullptr;
#endif

static bool ignoreUnixSignals = false;

/**
 * singleton instance pointer
 */
static KateApp *appSelf = nullptr;

Q_LOGGING_CATEGORY(LOG_KATE, "kate", QtWarningMsg)

void KateApp::initPreApplicationCreation(bool detach)
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_HAIKU)
    // Prohibit using sudo or kdesu (but allow using the root user directly)
    if (getuid() == 0) {
        setlocale(LC_ALL, "");
        bindtextdomain("kate", KDE_INSTALL_FULL_LOCALEDIR);
        if (!qEnvironmentVariableIsEmpty("SUDO_USER")) {
            auto message = kli18n(
                "Running this editor with sudo can cause bugs and expose you to security vulnerabilities. "
                "Instead use this editor normally and you will be prompted for elevated privileges when "
                "saving documents if needed.");
            std::cout << dgettext("kate", message.untranslatedText()) << std::endl;
            exit(EXIT_FAILURE);
        } else if (!qEnvironmentVariableIsEmpty("KDESU_USER")) {
            auto message = kli18n(
                "Running this editor with kdesu can cause bugs and expose you to security vulnerabilities. "
                "Instead use this editor normally and you will be prompted for elevated privileges when "
                "saving documents if needed.");
            std::cout << dgettext("kate", message.untranslatedText()) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
#endif

    /**
     * ensure we have a proper tmp dir for Flatpak
     * https://invent.kde.org/utilities/kate/-/merge_requests/1987
     * https://github.com/flatpak/flatpak/issues/3438
     */
    if (KSandbox::isFlatpak() && !qEnvironmentVariableIsEmpty("XDG_RUNTIME_DIR") && !qEnvironmentVariableIsEmpty("FLATPAK_ID")) {
        // construct a private tmp dir, only use it, if we can successfully create it or it is already there
        const QString tmpDir = qEnvironmentVariable("XDG_RUNTIME_DIR") + QStringLiteral("/app/") + qEnvironmentVariable("FLATPAK_ID");
        QDir().mkpath(tmpDir);
        if (QDir().exists(tmpDir)) {
            qputenv("TMPDIR", tmpDir.toUtf8());
        }
    }

    /**
     * trigger initialisation of proper icon theme
     * see https://invent.kde.org/frameworks/kiconthemes/-/merge_requests/136
     */
#if KICONTHEMES_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    KIconTheme::initTheme();
#endif

#if defined(Q_OS_WIN)
    // try to attach to console for terminal detection and output
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // we are inside a terminal
        insideTerminal = true;

        // don't enable output if we shall detach, to avoid terminal pollution
        if (!detach) {
            if (fileno(stdout) < 0)
                freopen("CON", "w", stdout);
            if (fileno(stderr) < 0)
                freopen("CON", "w", stderr);
        } else {
            // detach again to not die on ctrl-c and Co.
            FreeConsole();
        }
    }
#endif

#ifdef HAVE_CTERMID
    /**
     * https://stackoverflow.com/questions/1312922/detect-if-stdin-is-a-terminal-or-pipe-in-c-c-qt
     */
    char tty[L_ctermid + 1] = {0};
    ctermid(tty);
    if (int fd = ::open(tty, O_RDONLY); fd >= 0) {
        insideTerminal = true;
        ::close(fd);
    }
#endif

    // fork without exec not supported on macOS, will just crash, avoid that there
#if !defined(Q_OS_MACOS) && defined(HAVE_DAEMON)
    if (detach) {
        // just try it, if it doesn't work we just continue in the foreground
        const int ret = daemon(1, 0 /* close in and outputs to avoid pollution of shell */);
        (void)ret;
    }
#endif

#if defined(Q_OS_MACOS)
    if (!insideTerminal) {
        int mib[2] = {CTL_USER, USER_CS_PATH};
        size_t len = 0;
        sysctl(mib, 2, nullptr, &len, nullptr, 0);
        QByteArray path(len, '\0');
        sysctl(mib, 2, &path[0], &len, nullptr, 0);
        path.removeLast();
        QByteArray p = qgetenv("PATH");
        qDebug("Adding '%s' to existing PATH: %s", path.constData(), p.constData());
        path.append(':').append(p);
        qputenv("PATH", p);
    }
#endif
}

KateApp::KateApp(const QCommandLineParser &args, const ApplicationMode mode, const QString &sessionsDir)
    : m_args(args)
    , m_mode(mode)
    , m_wrapper(appSelf = this)
    , m_docManager(this)
    , m_sessionManager(this, sessionsDir)
#ifdef WITH_KUSERFEEDBACK
    , m_userFeedbackProvider(new KUserFeedback::Provider(this))
#endif
    , m_lastActivationChange(QDateTime::currentMSecsSinceEpoch())
{
#ifdef WITH_DBUS
    // init adaptor if not already there, that can happen in tests
    if (!adaptor) {
        adaptor = new KateAppAdaptor;
    }
#endif

#if HAVE_STYLE_MANAGER
    /**
     * trigger initialisation of proper application style
     * see https://invent.kde.org/frameworks/kconfigwidgets/-/merge_requests/239
     */
    KStyleManager::initStyle();
#else
    /**
     * For Windows and macOS: use Breeze if available
     * Of all tested styles that works the best for us
     */
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QApplication::setStyle(QStringLiteral("breeze"));
#endif
#endif

    /**
     * Enable crash handling through KCrash.
     */
    KCrash::initialize();

    /**
     * re-route some signals to application wrapper
     */
    connect(&m_docManager, &KateDocManager::documentCreated, &m_wrapper, &KTextEditor::Application::documentCreated);
    connect(&m_docManager, &KateDocManager::documentWillBeDeleted, &m_wrapper, &KTextEditor::Application::documentWillBeDeleted);
    connect(&m_docManager, &KateDocManager::documentDeleted, &m_wrapper, &KTextEditor::Application::documentDeleted);

    /**
     * handle mac os x like file open request via event filter
     */
    qApp->installEventFilter(this);

#ifdef WITH_KUSERFEEDBACK
    /**
     * defaults, inspired by plasma
     * important: choose between kate and kwrite mode here to submit the right product id
     */
    m_userFeedbackProvider->setProductIdentifier(isKate() ? QStringLiteral("org.kde.kate") : QStringLiteral("org.kde.kwrite"));
    m_userFeedbackProvider->setFeedbackServer(QUrl(QStringLiteral("https://telemetry.kde.org/")));
    m_userFeedbackProvider->setSubmissionInterval(7);
    m_userFeedbackProvider->setApplicationStartsUntilEncouragement(5);
    m_userFeedbackProvider->setEncouragementDelay(30);

    /**
     * add some feedback providers
     */

    // software version info
    m_userFeedbackProvider->addDataSource(new KUserFeedback::ApplicationVersionSource);
    m_userFeedbackProvider->addDataSource(new KUserFeedback::QtVersionSource);

    // info about the machine
    m_userFeedbackProvider->addDataSource(new KUserFeedback::PlatformInfoSource);
    m_userFeedbackProvider->addDataSource(new KUserFeedback::ScreenInfoSource);

    // usage info
    m_userFeedbackProvider->addDataSource(new KUserFeedback::StartCountSource);
    m_userFeedbackProvider->addDataSource(new KUserFeedback::UsageTimeSource);
#endif
}

KateApp::~KateApp()
{
    qCDebug(LOG_KATE, "%s", Q_FUNC_INFO);
    // we want no auto saving during application closing, we handle that explicitly
    KateSessionManager::AutoSaveBlocker blocker(sessionManager());

#ifdef WITH_DBUS
    /**
     * unregister from dbus before we get unusable...
     */
    if (QDBusConnection::sessionBus().interface()) {
        adaptor->emitExiting();
        QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/MainApplication"));
    }
#endif

    /**
     * delete all main windows before the document manager & co. die
     */
    while (!m_mainWindows.isEmpty()) {
        // mainwindow itself calls KateApp::removeMainWindow(this)
        delete m_mainWindows[0];
    }
}

KateApp *KateApp::self()
{
    return appSelf;
}

void KateApp::initPostApplicationCreation(KAboutData &aboutData)
{
#ifdef WITH_DBUS
    // on wayland: init token if we are launched by Konsole and have none
    if (KWindowSystem::isPlatformWayland() && qEnvironmentVariable("XDG_ACTIVATION_TOKEN").isEmpty() && QDBusConnection::sessionBus().interface()) {
        // can we ask Konsole for a token?
        const auto konsoleService = qEnvironmentVariable("KONSOLE_DBUS_SERVICE");
        const auto konsoleSession = qEnvironmentVariable("KONSOLE_DBUS_SESSION");
        const auto konsoleActivationCookie = qEnvironmentVariable("KONSOLE_DBUS_ACTIVATION_COOKIE");
        if (!konsoleService.isEmpty() && !konsoleSession.isEmpty() && !konsoleActivationCookie.isEmpty()) {
            // we ask the current shell session
            QDBusMessage m =
                QDBusMessage::createMethodCall(konsoleService, konsoleSession, QStringLiteral("org.kde.konsole.Session"), QStringLiteral("activationToken"));

            // use the cookie from the environment
            m.setArguments({konsoleActivationCookie});

            // get the token, if possible and export it to environment for later use
            const auto tokenAnswer = QDBusConnection::sessionBus().call(m);
            if (tokenAnswer.type() == QDBusMessage::ReplyMessage && !tokenAnswer.arguments().isEmpty()) {
                const auto token = tokenAnswer.arguments().constFirst().toString();
                if (!token.isEmpty()) {
                    qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
                }
            }
        }
    }
#endif

    // fill author data
    aboutData.addAuthor(i18n("Christoph Cullmann"), i18n("Maintainer"), QStringLiteral("cullmann@kde.org"), QStringLiteral("https://cullmann.dev"));
    aboutData.addAuthor(i18n("Dominik Haumann"), i18n("Core Developer"), QStringLiteral("dhaumann@kde.org"));
    aboutData.addAuthor(i18n("Sven Brauch"), i18n("Developer"), QStringLiteral("mail@svenbrauch.de"));
    aboutData.addAuthor(i18n("Kåre Särs"), i18n("Developer"), QStringLiteral("kare.sars@iki.fi"));
    aboutData.addAuthor(i18n("Waqar Ahmed"), i18n("Core Developer"), QStringLiteral("waqar.17a@gmail.com"));
    aboutData.addAuthor(i18n("Anders Lund"), i18n("Core Developer"), QStringLiteral("anders@alweb.dk"));
    aboutData.addAuthor(i18n("Joseph Wenninger"), i18n("Core Developer"), QStringLiteral("jowenn@kde.org"));
    aboutData.addAuthor(i18n("Hamish Rodda"), i18n("Core Developer"), QStringLiteral("rodda@kde.org"));
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
    aboutData.addAuthor(i18n("Pablo Martín"),
                        i18n("Python Plugin Developer"),
                        QStringLiteral("goinnn@gmail.com"),
                        QStringLiteral("https://github.com/goinnn/"));
    aboutData.addAuthor(i18n("Gerald Senarclens de Grancy"), i18n("QA and Scripting"), QStringLiteral("oss@senarclens.eu"));

    // fill credits
    aboutData.addCredit(i18n("Tyson Tan"),
                        i18n("Designer of Kate's mascot 'Kate the Cyber Woodpecker'"),
                        QString(),
                        QStringLiteral("https://www.tysontan.com/"));
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
}

bool KateApp::init()
{
    // we want no auto saving during application startup, we handle that explicitly
    KateSessionManager::AutoSaveBlocker blocker(sessionManager());

    // set KATE_PID for use in child processes
    if (isKate()) {
        qputenv("KATE_PID", QStringLiteral("%1").arg(QCoreApplication::applicationPid()).toLatin1().constData());
    }

#ifdef Q_OS_UNIX
    /**
     * Set up signal handler for SIGINT and SIGTERM
     */
    KSignalHandler::self()->watchSignal(SIGINT);
    KSignalHandler::self()->watchSignal(SIGTERM);
    connect(KSignalHandler::self(), &KSignalHandler::signalReceived, this, [this](int signal) {
        if ((signal == SIGINT || signal == SIGTERM) && !ignoreUnixSignals) {
            const char *str = signal == SIGINT ? "SIGINT" : "SIGTERM";
            qCDebug(LOG_KATE, "signal received: %s, Shutting down...", str);
            quit();
        }
    });
#endif

    if (isKate()) {
        KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("General"));
        const QString katePATH = cg.readEntry("Kate PATH", QString());
        if (!katePATH.isEmpty()) {
            QByteArray envPATH = qgetenv("PATH");
            qputenv("PATH", katePATH.toUtf8().append(QDir::listSeparator().toLatin1()).append(envPATH));
        }
    }

    // handle restore different
    if (qApp->isSessionRestored()) {
        restoreKate();
    } else {
        // let us handle our command line args and co ;)
        // we can exit here if session chooser decides
        if (!startupKate()) {
            // session chooser told to exit kate
            return false;
        }
    }

    // Save session now, else if the user never opens a new doc or closes one session-autosave will never trigger and
    // the session will only get saved if Kate is properly closed and not if it gets killed.
    // Do it delayed, don't block the startup
    QTimer::singleShot(100, this, [this] {
        qCDebug(LOG_KATE, "KateApp::init() save session on startup");
        sessionManager()->saveActiveSession(true, /*isAutoSave=*/true);
    });

    return true;
}

void KateApp::restoreKate()
{
    // we want no auto saving during application startup, we handle that explicitly
    KateSessionManager::AutoSaveBlocker blocker(sessionManager());

    KConfig *sessionConfig = KConfigGui::sessionConfig();

    // activate again correct session!!!
    QString lastSession(sessionConfig->group(QStringLiteral("General")).readEntry("Last Session", QString()));
    sessionManager()->activateSession(lastSession, false, false);

    // plugins
    KateApp::self()->pluginManager()->loadConfig(sessionConfig);

    // restore the files we need
    m_docManager.restoreDocumentList(sessionConfig);

    // restore all windows ;)
    for (int n = 1; KMainWindow::canBeRestored(n); n++) {
        newMainWindow(sessionConfig, QString::number(n));
    }

    // oh, no mainwindow, create one, should not happen, but make sure ;)
    if (mainWindowsCount() == 0) {
        newMainWindow();
    }
}

bool KateApp::startupKate()
{
    // we want no auto saving during application startup, we handle that explicitly
    KateSessionManager::AutoSaveBlocker blocker(sessionManager());

    // KWrite is session less
    if (isKWrite()) {
        sessionManager()->activateAnonymousSession();
    } else {
        // user specified session to open
        if (m_args.isSet(QStringLiteral("start"))) {
            sessionManager()->activateSession(m_args.value(QStringLiteral("start")), false);
        } else if (m_args.isSet(QStringLiteral("startanon"))) {
            sessionManager()->activateAnonymousSession();
        }

        // only start session if we are not started from the terminal or if no are files specified
        // we added the !isInsideTerminal() case to allow e.g. session start on use of Dolphin to open a file
        // see bug 446852
        else if (!isInsideTerminal() || (!m_args.isSet(QStringLiteral("stdin")) && (m_args.positionalArguments().count() == 0))) {
            // let the user choose session if possible
            if (!sessionManager()->chooseSession()) {
#if HAVE_X11
                // we will exit kate now, notify the rest of the world we are done
                KStartupInfo::appStarted();
#endif

                return false;
            }
        } else {
            sessionManager()->activateAnonymousSession();
        }
    }

    // oh, no mainwindow, create one, should not happen, but make sure ;)
    if (mainWindowsCount() == 0) {
        newMainWindow();
    }

    bool tempfileSet = m_args.isSet(QStringLiteral("tempfile"));

    KTextEditor::Document *doc = nullptr;
    const QString codec_name = m_args.isSet(QStringLiteral("encoding")) ? m_args.value(QStringLiteral("encoding")) : QString();

    auto args = m_args.positionalArguments();

    // support +xyz line number as first argument
    KTextEditor::Cursor firstLineNumberArgument = UrlInfo::parseLineNumberArgumentAndRemoveIt(args);

    for (UrlInfo info : std::as_const(args)) {
        // this file is no local dir, open it, else warn
        if (!info.url.isLocalFile() || KNetworkMounts::self()->isOptionEnabledForPath(info.url.toLocalFile(), KNetworkMounts::LowSideEffectsOptimizations)
            || !QFileInfo(info.url.toLocalFile()).isDir()) {
            if (!info.cursor.isValid()) {
                if (hasCursorInArgs()) {
                    info.cursor = cursorFromArgs();
                } else if (info.url.hasQuery()) {
                    info.cursor = cursorFromQueryString(info.url);
                } else if (firstLineNumberArgument.isValid()) {
                    // same semantics as e.g. in mcedit, just use for the first file
                    info.cursor = firstLineNumberArgument;
                    firstLineNumberArgument = KTextEditor::Cursor::invalid();
                }
            }
            doc = openDocUrl(info.url, codec_name, tempfileSet, /*activateView=*/false, info.cursor);
        } else if (isKate() && !KateApp::self()->pluginManager()->plugin(QStringLiteral("kateprojectplugin"))) {
            KMessageBox::error(activeKateMainWindow(), i18n("Folders can only be opened when the projects plugin is enabled"));
        }
    }

    // handle stdin input
    if (m_args.isSet(QStringLiteral("stdin"))) {
        QString text;
        if (QFile input; input.open(stdin, QIODevice::ReadOnly)) {
            auto decoder = QStringDecoder(codec_name.toUtf8().constData());
            text = decoder.isValid() ? decoder.decode(input.readAll()) : QString::fromLocal8Bit(input.readAll());
            // normalize line endings, to e.g. catch issues with \r\n on Windows
            static const auto re = QRegularExpression(QStringLiteral("\r\n?"));
            text.replace(re, QStringLiteral("\n"));
        }
        openInput(text, codec_name);
    } else if (doc) {
        activeKateMainWindow()->viewManager()->activateView(doc);
    }

    return true;
}

void KateApp::shutdownKate(KateMainWindow *win)
{
    // we want no auto saving during application closing, we handle that explicitly
    KateSessionManager::AutoSaveBlocker blocker(sessionManager());

    if (win && !win->queryClose_internal()) {
        return;
    }

    //  we are ready to quit, ignore any unix signals now to avoid incorrect session save. BUG: 508494
    setIgnoreSignals();

    qCDebug(LOG_KATE, "KateApp::%s save session", __func__);
    sessionManager()->saveActiveSession(true);

    /**
     * all main windows will be cleaned up
     * in the KateApp destructor after the event
     * loop is left
     *
     * NOTE: From Qt 6, quit() will ask all windows to close,
     * but we already do our cleanup (and save prompts) when
     * the event loop quits, so we explicitly call exit() here.
     */
    QApplication::exit();
}

KatePluginManager *KateApp::pluginManager()
{
    return &m_pluginManager;
}

KateDocManager *KateApp::documentManager()
{
    return &m_docManager;
}

KateSessionManager *KateApp::sessionManager()
{
    return &m_sessionManager;
}

KateStashManager *KateApp::stashManager()
{
    return &m_stashManager;
}

KTextEditor::Document *KateApp::openDocUrl(const QUrl &url, const QString &encoding, bool isTempFile, bool activateView, KTextEditor::Cursor c)
{
    // temporary file handling
    // ensure we will delete the local file we opened via --tempfile at end of program
    // we can only do this properly for local files
    isTempFile = (isTempFile && !url.isEmpty() && url.isLocalFile() && QFile::exists(url.toLocalFile()));

    KateMainWindow *mainWindow = activeKateMainWindow();

    if (!mainWindow) {
        return nullptr;
    }

    // this file is no local dir, open it, else warn
    bool noDir = !url.isLocalFile() || KNetworkMounts::self()->isOptionEnabledForPath(url.toLocalFile(), KNetworkMounts::LowSideEffectsOptimizations)
        || !QFileInfo(url.toLocalFile()).isDir();

    KTextEditor::Document *doc = nullptr;

    if (noDir) {
        KateDocumentInfo docInfo;
        docInfo.startCursor = c;
        doc = mainWindow->viewManager()->openUrl(url, encoding, activateView, isTempFile, &docInfo);
    } else {
        KMessageBox::error(mainWindow, i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", url.url()));
    }

    // document was successfully opened, ensure we will handle destroy properly
    if (doc) {
        // connect to slot & register the temp file handling if needed
        connect(doc, &QObject::destroyed, this, &KateApp::openDocUrlDocumentDestroyed);
        if (isTempFile) {
            m_tempFilesToDelete[doc].push_back(url.toLocalFile());
        }
    }

    // unable to open document, directly dispose of the temporary file
    else if (isTempFile) {
        QFile::remove(url.toLocalFile());
    }

    return doc;
}

void KateApp::openDocUrlDocumentDestroyed(QObject *document)
{
    // do we need to kill the temporary files for this document?
    if (const auto tempFilesIt = m_tempFilesToDelete.find(document); tempFilesIt != m_tempFilesToDelete.end()) {
        const auto &files = tempFilesIt.value();
        for (const auto &file : files) {
            QFile::remove(file);
        }
        m_tempFilesToDelete.erase(tempFilesIt);
    }

#ifdef WITH_DBUS
    // emit token signal to unblock remove blocking instances
    adaptor->emitDocumentClosed(QString::number(reinterpret_cast<qptrdiff>(document)));
#endif
}

KTextEditor::Cursor KateApp::cursorFromArgs()
{
    bool hasLine = m_args.isSet(QStringLiteral("line"));
    bool hasColumn = m_args.isSet(QStringLiteral("column"));

    if (!hasLine && !hasColumn) {
        return KTextEditor::Cursor::invalid();
    }

    int line = qMax(m_args.value(QStringLiteral("line")).toInt() - 1, 0);
    int column = qMax(m_args.value(QStringLiteral("column")).toInt() - 1, 0);

    return {line, column};
}

KTextEditor::Cursor KateApp::cursorFromQueryString(const QUrl &url)
{
    if (!url.hasQuery()) {
        return KTextEditor::Cursor::invalid();
    }

    QUrlQuery urlQuery(url);
    QString lineStr = urlQuery.queryItemValue(QStringLiteral("line"));
    QString columnStr = urlQuery.queryItemValue(QStringLiteral("column"));

    if (lineStr.isEmpty() && columnStr.isEmpty()) {
        return KTextEditor::Cursor::invalid();
    }

    int line = qMax(urlQuery.queryItemValue(QStringLiteral("line")).toInt() - 1, 0);
    int column = qMax(urlQuery.queryItemValue(QStringLiteral("column")).toInt() - 1, 0);

    return {line, column};
}

bool KateApp::setCursor(int line, int column)
{
    KateMainWindow *mainWindow = activeKateMainWindow();

    if (!mainWindow) {
        return false;
    }

    if (auto v = mainWindow->viewManager()->activeView()) {
        v->removeSelection();
        v->setCursorPosition(KTextEditor::Cursor(line, column));
    }

    return true;
}

bool KateApp::hasCursorInArgs()
{
    return m_args.isSet(QStringLiteral("line")) || m_args.isSet(QStringLiteral("column"));
}

bool KateApp::openInput(const QString &text, const QString &encoding)
{
    activeKateMainWindow()->viewManager()->openUrl(QUrl(), encoding, true);

    if (!activeKateMainWindow()->viewManager()->activeView()) {
        return false;
    }

    KTextEditor::Document *doc = activeKateMainWindow()->viewManager()->activeView()->document();

    if (!doc) {
        return false;
    }

    return doc->setText(text);
}

KTextEditor::MainWindow *KateApp::activeMainWindow()
{
    // either return wrapper or nullptr
    if (KateMainWindow *a = activeKateMainWindow()) {
        return a->wrapper();
    }
    return nullptr;
}

QList<KTextEditor::MainWindow *> KateApp::mainWindows()
{
    // assemble right list
    QList<KTextEditor::MainWindow *> windows;
    windows.reserve(m_mainWindows.size());

    for (const auto mainWindow : std::as_const(m_mainWindows)) {
        windows.push_back(mainWindow->wrapper());
    }
    return windows;
}

KateMainWindow *KateApp::newMainWindow(KConfig *sconfig_, const QString &sgroup_, bool userTriggered)
{
    KConfig *sconfig = sconfig_ ? sconfig_ : KSharedConfig::openConfig().data();
    QString sgroup = !sgroup_.isEmpty() ? sgroup_ : QStringLiteral("MainWindow0");

    QElapsedTimer t;
    t.start();
    auto *mainWindow = new KateMainWindow(sconfig, sgroup, userTriggered);
    qCDebug(LibKateTime, "Created KateMainWindow in %lld ms", t.elapsed());
    mainWindow->show();

    return mainWindow;
}

void KateApp::addMainWindow(KateMainWindow *mainWindow)
{
    m_mainWindows.push_back(mainWindow);
}

void KateApp::removeMainWindow(KateMainWindow *mainWindow)
{
    m_mainWindows.removeAll(mainWindow);
}

KateMainWindow *KateApp::activeKateMainWindow()
{
    if (m_mainWindows.isEmpty()) {
        return nullptr;
    }

    int n = m_mainWindows.indexOf(qApp->activeWindow());

    if (n < 0) {
        n = 0;
    }

    return m_mainWindows[n];
}

int KateApp::mainWindowsCount() const
{
    return m_mainWindows.size();
}

int KateApp::mainWindowID(KateMainWindow *window)
{
    return m_mainWindows.indexOf(window);
}

KateMainWindow *KateApp::mainWindow(int n)
{
    if (n < m_mainWindows.size()) {
        return m_mainWindows[n];
    }

    return nullptr;
}

bool KateApp::closeDocuments(const QList<KTextEditor::Document *> &documents)
{
    bool shutdownKate =
        KateApp::self()->activeKateMainWindow()->modCloseAfterLast() && KateApp::self()->documentManager()->documentList().size() == documents.size();
    bool success = m_docManager.closeDocumentList(documents, KateApp::self()->activeKateMainWindow());

    if (success && shutdownKate) {
        QTimer::singleShot(0, this, []() {
            KateApp::self()->shutdownKate(KateApp::self()->activeKateMainWindow());
        });
        return true;
    }

    return success;
}

KTextEditor::Plugin *KateApp::plugin(const QString &name)
{
    return m_pluginManager.plugin(name);
}

bool KateApp::eventFilter(QObject *obj, QEvent *event)
{
    // keep track when we got activated last time
    if (event->type() == QEvent::ActivationChange) {
        m_lastActivationChange = QDateTime::currentMSecsSinceEpoch();
    }

    /**
     * handle mac os like file open
     */
    else if (event->type() == QEvent::FileOpen) {
        /**
         * try to open and activate the new document, like we would do for stuff
         * opened via dbus
         */
        auto *foe = static_cast<QFileOpenEvent *>(event);
        KTextEditor::Document *doc = openDocUrl(foe->url(), QString(), false);
        if (doc && activeKateMainWindow()) {
            activeKateMainWindow()->viewManager()->activateView(doc);
        }
        return true;
    }

    /**
     * else: pass over to default implementation
     */
    return QObject::eventFilter(obj, event);
}

void KateApp::remoteMessageReceived(quint32, QByteArray message)
{
    /**
     * try to parse message, ignore if no object
     */
    const QJsonDocument jsonMessage = QJsonDocument::fromJson(message);
    if (!jsonMessage.isObject()) {
        return;
    }

    KTextEditor::Document *doc = nullptr;
    /**
     * open all passed urls
     */
    const QJsonArray urls = jsonMessage.object().value(QLatin1String("urls")).toArray();
    for (const auto &urlObject : urls) {
        /**
         * get url meta data
         */
        const QUrl url = urlObject.toObject().value(QLatin1String("url")).toVariant().toUrl();
        const int line = urlObject.toObject().value(QLatin1String("line")).toVariant().toInt();
        const int column = urlObject.toObject().value(QLatin1String("column")).toVariant().toInt();

        /**
         * open file + save line/column if requested
         */
        doc = openDocUrl(url, QString(), false, /*activateView=*/false, KTextEditor::Cursor{line, column});
    }

    // try to activate current window
    activate(jsonMessage.object().value(QLatin1String("activationToken")).toString());
    if (doc && activeMainWindow()) {
        activeMainWindow()->activateView(doc);
    }
}

bool KateApp::documentVisibleInOtherWindows(KTextEditor::Document *doc, KateMainWindow *window) const
{
    for (auto win : m_mainWindows) {
        if (win != window && win->viewManager()->viewspaceCountForDoc(doc) > 0) {
            return true;
        }
    }
    return false;
}

bool KateApp::isInsideTerminal()
{
    return insideTerminal;
}

void KateApp::setIgnoreSignals()
{
    ignoreUnixSignals = true;
}

qint64 KateApp::lastActivationChange() const
{
#if HAVE_X11
    // we invert the result if we are on the wrong virtual desktop
    // this allows for easy filtering in the client process, bug 486066
    if (KWindowSystem::isPlatformX11()) {
        for (auto win : m_mainWindows) {
            KWindowInfo info(win->winId(), NET::WMDesktop);
            if (info.valid() && info.isOnCurrentDesktop()) {
                return m_lastActivationChange;
            }
        }
        return -m_lastActivationChange;
    }
#endif

    return m_lastActivationChange;
}

void KateApp::activate(const QString &token)
{
    KateMainWindow *win = activeKateMainWindow();
    if (win) {
        win->activate(token);
    }
}

#include "moc_kateapp.cpp"
