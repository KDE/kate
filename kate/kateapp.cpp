/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2002 Joseph Wenninger <jowenn@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kateapp.h"

#include "kateviewmanager.h"

#include <kcoreaddons_version.h>

#include <KAboutData>
#include <KConfigGroup>
#include <KConfigGui>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNetworkMounts>
#include <KSharedConfig>
#include <KStartupInfo>
#include <KWindowInfo>

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
#include <QDBusConnection>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTextCodec>
#include <QUrlQuery>

#include <signal_watcher.h>
#include <urlinfo.h>

/**
 * singleton instance pointer
 */
static KateApp *appSelf = Q_NULLPTR;

Q_LOGGING_CATEGORY(LOG_KATE, "kate", QtWarningMsg)

KateApp::KateApp(const QCommandLineParser &args, const ApplicationMode mode)
    : m_args(args)
    , m_mode(mode)
    , m_wrapper(appSelf = this)
    , m_docManager(this)
    , m_adaptor(this)
    , m_pluginManager(this)
    , m_sessionManager(this,
                       QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                           + (isKate() ? QStringLiteral("/kate/sessions") : QStringLiteral("/kwrite/sessions")))
    , m_stashManager(this)
{
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
    m_userFeedbackProvider.setProductIdentifier(isKate() ? QStringLiteral("org.kde.kate") : QStringLiteral("org.kde.kwrite"));
    m_userFeedbackProvider.setFeedbackServer(QUrl(QStringLiteral("https://telemetry.kde.org/")));
    m_userFeedbackProvider.setSubmissionInterval(7);
    m_userFeedbackProvider.setApplicationStartsUntilEncouragement(5);
    m_userFeedbackProvider.setEncouragementDelay(30);

    /**
     * add some feedback providers
     */

    // software version info
    m_userFeedbackProvider.addDataSource(new KUserFeedback::ApplicationVersionSource);
    m_userFeedbackProvider.addDataSource(new KUserFeedback::QtVersionSource);

    // info about the machine
    m_userFeedbackProvider.addDataSource(new KUserFeedback::PlatformInfoSource);
    m_userFeedbackProvider.addDataSource(new KUserFeedback::ScreenInfoSource);

    // usage info
    m_userFeedbackProvider.addDataSource(new KUserFeedback::StartCountSource);
    m_userFeedbackProvider.addDataSource(new KUserFeedback::UsageTimeSource);
#endif
}

KateApp::~KateApp()
{
    /**
     * unregister from dbus before we get unusable...
     */
    if (QDBusConnection::sessionBus().interface()) {
        m_adaptor.emitExiting();
        QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/MainApplication"));
    }

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

void KateApp::fillAuthorsAndCredits(KAboutData &aboutData)
{
    aboutData.addAuthor(i18n("Christoph Cullmann"), i18n("Maintainer"), QStringLiteral("cullmann@kde.org"), QStringLiteral("https://cullmann.io"));
    aboutData.addAuthor(i18n("Dominik Haumann"), i18n("Core Developer"), QStringLiteral("dhaumann@kde.org"));
    aboutData.addAuthor(i18n("Sven Brauch"), i18n("Developer"), QStringLiteral("mail@svenbrauch.de"));
    aboutData.addAuthor(i18n("Kåre Särs"), i18n("Developer"), QStringLiteral("kare.sars@iki.fi"));
    aboutData.addAuthor(i18n("Anders Lund"), i18n("Core Developer"), QStringLiteral("anders@alweb.dk"), QStringLiteral("https://www.alweb.dk"));
    aboutData.addAuthor(i18n("Joseph Wenninger"),
                        i18n("Core Developer"),
                        QStringLiteral("jowenn@kde.org"),
                        QStringLiteral("http://stud3.tuwien.ac.at/~e9925371"));
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
    aboutData.addAuthor(i18n("Gerald Senarclens de Grancy"),
                        i18n("QA and Scripting"),
                        QStringLiteral("oss@senarclens.eu"),
                        QStringLiteral("http://find-santa.eu/"));

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
    // set KATE_PID for use in child processes
    if (isKate()) {
        qputenv("KATE_PID", QStringLiteral("%1").arg(QCoreApplication::applicationPid()).toLatin1().constData());
    }

#ifdef Q_OS_UNIX
    /**
     * Set up signal handler for SIGINT and SIGTERM
     */
    auto sigWatcher = new SignalWatcher(this);
    connect(sigWatcher, &SignalWatcher::unixSignal, this, [this](SignalWatcher::Signal) {
        printf("Shutting down...\n");
        quit();
    });
#endif

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

    return true;
}

void KateApp::restoreKate()
{
    KConfig *sessionConfig = KConfigGui::sessionConfig();

    // activate again correct session!!!
    QString lastSession(sessionConfig->group("General").readEntry("Last Session", QString()));
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
    // KWrite is session less
    if (isKWrite()) {
        sessionManager()->activateAnonymousSession();
    } else {
        // user specified session to open
        if (m_args.isSet(QStringLiteral("start"))) {
            sessionManager()->activateSession(m_args.value(QStringLiteral("start")), false);
        } else if (m_args.isSet(QStringLiteral("startanon"))) {
            sessionManager()->activateAnonymousSession();
        } else if (!m_args.isSet(QStringLiteral("stdin")) && (m_args.positionalArguments().count() == 0)) { // only start session if no files specified
            // let the user choose session if possible
            if (!sessionManager()->chooseSession()) {
                // we will exit kate now, notify the rest of the world we are done
                KStartupInfo::appStarted(KStartupInfo::startupId());
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

    // notify about start
    QWidget *win = activeKateMainWindow();
    win->setAttribute(Qt::WA_NativeWindow, true);
    KStartupInfo::setNewStartupId(win->windowHandle(), KStartupInfo::startupId());

    QTextCodec *codec = m_args.isSet(QStringLiteral("encoding")) ? QTextCodec::codecForName(m_args.value(QStringLiteral("encoding")).toUtf8()) : nullptr;
    bool tempfileSet = m_args.isSet(QStringLiteral("tempfile"));

    KTextEditor::Document *doc = nullptr;
    const QString codec_name = codec ? QString::fromLatin1(codec->name()) : QString();

    const auto args = m_args.positionalArguments();

    for (const auto &positionalArgument : args) {
        UrlInfo info(positionalArgument);

        // this file is no local dir, open it, else warn
        bool noDir = !info.url.isLocalFile()
            || KNetworkMounts::self()->isOptionEnabledForPath(info.url.toLocalFile(), KNetworkMounts::LowSideEffectsOptimizations)
            || !QFileInfo(info.url.toLocalFile()).isDir();

        if (noDir) {
            if (!info.cursor.isValid()) {
                if (hasCursorInArgs()) {
                    info.cursor = cursorFromArgs();
                } else if (info.url.hasQuery()) {
                    info.cursor = cursorFromQueryString(info.url);
                }
            }
            doc = openDocUrl(info.url, codec_name, tempfileSet, /*activateView=*/false, info.cursor);
        } else if (!KateApp::self()->pluginManager()->plugin(QStringLiteral("kateprojectplugin"))) {
            KMessageBox::sorry(activeKateMainWindow(), i18n("Folders can only be opened when the projects plugin is enabled"));
        }
    }

    // handle stdin input
    if (m_args.isSet(QStringLiteral("stdin"))) {
        QFile input;
        input.open(stdin, QIODevice::ReadOnly);
        QString text = codec ? codec->toUnicode(input.readAll()) : QString::fromLocal8Bit(input.readAll());

        // normalize line endings, to e.g. catch issues with \r\n on Windows
        text.replace(QRegularExpression(QStringLiteral("\r\n?")), QStringLiteral("\n"));

        openInput(text, codec_name);
    } else if (doc) {
        activeKateMainWindow()->viewManager()->activateView(doc);
    }

    return true;
}

void KateApp::shutdownKate(KateMainWindow *win)
{
    if (!win->queryClose_internal()) {
        return;
    }

    sessionManager()->saveActiveSession(true);
    stashManager()->stashDocuments(sessionManager()->activeSession()->config(), documentManager()->documentList());

    /**
     * all main windows will be cleaned up
     * in the KateApp destructor after the event
     * loop is left
     */
    QApplication::quit();
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

bool KateApp::openUrl(const QUrl &url, const QString &encoding, bool isTempFile)
{
    return openDocUrl(url, encoding, isTempFile);
}

bool KateApp::isOnActivity(const QString &activity)
{
    for (const auto window : qAsConst(m_mainWindows)) {
        const KWindowInfo info(window->winId(), {}, NET::WM2Activities);
        const auto activities = info.activities();
        // handle special case of "on all activities"
        if (activities.isEmpty() || activities.contains(activity)) {
            return true;
        }
    }

    return false;
}

KTextEditor::Document *KateApp::openDocUrl(const QUrl &url, const QString &encoding, bool isTempFile, bool activateView, KTextEditor::Cursor c)
{
    KateMainWindow *mainWindow = activeKateMainWindow();

    if (!mainWindow) {
        return nullptr;
    }

    QTextCodec *codec = encoding.isEmpty() ? nullptr : QTextCodec::codecForName(encoding.toLatin1());

    // this file is no local dir, open it, else warn
    bool noDir = !url.isLocalFile()
        || KNetworkMounts::self()->isOptionEnabledForPath(url.toLocalFile(), KNetworkMounts::LowSideEffectsOptimizations)
        || !QFileInfo(url.toLocalFile()).isDir();

    KTextEditor::Document *doc = nullptr;

    if (noDir) {
        KateDocumentInfo docInfo;
        docInfo.startCursor = c;
        // open a normal file
        if (codec) {
            doc = mainWindow->viewManager()->openUrl(url, QString::fromLatin1(codec->name()), activateView, isTempFile, docInfo);
        } else {
            doc = mainWindow->viewManager()->openUrl(url, QString(), activateView, isTempFile, docInfo);
        }
    } else {
        KMessageBox::sorry(mainWindow, i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", url.url()));
    }

    return doc;
}

KTextEditor::Cursor KateApp::cursorFromArgs()
{
    int line = -1;
    int column = -1;

    if (m_args.isSet(QStringLiteral("line"))) {
        line = m_args.value(QStringLiteral("line")).toInt() - 1;
    }

    if (m_args.isSet(QStringLiteral("column"))) {
        column = m_args.value(QStringLiteral("column")).toInt() - 1;
    }

    return {line, column};
}

KTextEditor::Cursor KateApp::cursorFromQueryString(const QUrl &url)
{
    int line = -1;
    int column = -1;

    if (!url.hasQuery()) {
        return {line, column};
    }

    QUrlQuery urlQuery(url);
    QString lineStr = urlQuery.queryItemValue(QStringLiteral("line"));
    QString columnStr = urlQuery.queryItemValue(QStringLiteral("column"));

    if (!lineStr.isEmpty()) {
        line = lineStr.toInt();
        line > 0 && line--;
    }

    if (!columnStr.isEmpty()) {
        column = columnStr.toInt();
        column > 0 && column--;
    }

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

KateMainWindow *KateApp::newMainWindow(KConfig *sconfig_, const QString &sgroup_)
{
    KConfig *sconfig = sconfig_ ? sconfig_ : KSharedConfig::openConfig().data();
    QString sgroup = !sgroup_.isEmpty() ? sgroup_ : QStringLiteral("MainWindow0");

    KateMainWindow *mainWindow = new KateMainWindow(sconfig, sgroup);
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

    int n = m_mainWindows.indexOf(static_cast<KateMainWindow *>((static_cast<QApplication *>(QCoreApplication::instance())->activeWindow())));

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

void KateApp::emitDocumentClosed(const QString &token)
{
    m_adaptor.emitDocumentClosed(token);
}

KTextEditor::Plugin *KateApp::plugin(const QString &name)
{
    return m_pluginManager.plugin(name);
}

bool KateApp::eventFilter(QObject *obj, QEvent *event)
{
    /**
     * handle mac os like file open
     */
    if (event->type() == QEvent::FileOpen) {
        /**
         * try to open and activate the new document, like we would do for stuff
         * opened via dbus
         */
        QFileOpenEvent *foe = static_cast<QFileOpenEvent *>(event);
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

void KateApp::remoteMessageReceived(const QString &message, QObject *)
{
    /**
     * try to parse message, ignore if no object
     */
    const QJsonDocument jsonMessage = QJsonDocument::fromJson(message.toUtf8());
    if (!jsonMessage.isObject()) {
        return;
    }

    KTextEditor::Document *doc = nullptr;
    /**
     * open all passed urls
     */
    const QJsonArray urls = jsonMessage.object().value(QLatin1String("urls")).toArray();
    for (const QJsonValue &urlObject : urls) {
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
    m_adaptor.activate();
    if (doc && activeMainWindow()) {
        activeMainWindow()->activateView(doc);
    }
}
