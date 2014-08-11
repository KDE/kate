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

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateviewmanager.h"
#include "katesessionmanager.h"
#include "katemainwindow.h"
#include "katedebug.h"

#include <KConfig>
#include <KSharedConfig>
#include <KMessageBox>
#include <KStartupInfo>
#include <KLocalizedString>
#include <KConfigGui>
#include <KConfigGroup>

#include <QCommandLineParser>
#include <QFileInfo>
#include <QTextCodec>
#include <QApplication>

#include "kateappadaptor.h"

KateApp *KateApp::s_self = 0;

Q_LOGGING_CATEGORY(LOG_KATE, "kate")

KateApp::KateApp(const QCommandLineParser &args)
    : m_args(args)
    , m_wrapper(new KTextEditor::Application(this))
{
    s_self = this;

    // doc man
    m_docManager = new KateDocManager(this);

    // init all normal plugins
    m_pluginManager = new KatePluginManager(this);

    // session manager up
    m_sessionManager = new KateSessionManager(this);

    // dbus
    m_adaptor = new KateAppAdaptor(this);

    /**
     * re-route some signals to application wrapper
     */
    connect(m_docManager, SIGNAL(documentCreated(KTextEditor::Document*)), m_wrapper, SIGNAL(documentCreated(KTextEditor::Document*)));
    connect(m_docManager, SIGNAL(documentWillBeDeleted(KTextEditor::Document*)), m_wrapper, SIGNAL(documentWillBeDeleted(KTextEditor::Document*)));
    connect(m_docManager, SIGNAL(documentDeleted(KTextEditor::Document*)), m_wrapper, SIGNAL(documentDeleted(KTextEditor::Document*)));
    connect(m_docManager, SIGNAL(aboutToCreateDocuments()), m_wrapper, SIGNAL(aboutToCreateDocuments()));
    connect(m_docManager, SIGNAL(documentsCreated(QList<KTextEditor::Document*>)), m_wrapper, SIGNAL(documentsCreated(QList<KTextEditor::Document*>)));
}

KateApp::~KateApp()
{
    // unregister...
    m_adaptor->emitExiting();
    QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/MainApplication"));
    delete m_adaptor;

    // cu session manager
    delete m_sessionManager;

    // cu plugin manager
    delete m_pluginManager;

    // delete this now, or we crash
    delete m_docManager;
}

KateApp *KateApp::self()
{
    return s_self;
}

bool KateApp::init()
{

    qCDebug(LOG_KATE) << "Setting KATE_PID: '" << QCoreApplication::applicationPid() << "'";
    qputenv("KATE_PID", QString::fromLatin1("%1").arg(QCoreApplication::applicationPid()).toLatin1().constData());

    // handle restore different
    if (qApp->isSessionRestored()) {
        restoreKate();
    } else {
        // let us handle our command line args and co ;)
        // we can exit here if session chooser decides
        if (!startupKate()) {
            qCDebug(LOG_KATE) << "startupKate returned false";
            return false;
        }
    }

    // application dbus interface
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/MainApplication"), this);
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
    m_docManager->restoreDocumentList(sessionConfig);

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
    // user specified session to open
    if (m_args.isSet(QStringLiteral("start"))) {
        sessionManager()->activateSession(m_args.value(QStringLiteral("start")), false);
    } else if (m_args.isSet(QStringLiteral("startanon"))) {
        sessionManager()->activateAnonymousSession();
    } else if (!m_args.isSet(QStringLiteral("stdin")) && (m_args.positionalArguments().count() == 0)) { // only start session if no files specified
        // let the user choose session if possible
        if (!sessionManager()->chooseSession()) {
            qCDebug(LOG_KATE) << "chooseSession returned false, exiting";
            // we will exit kate now, notify the rest of the world we are done
#ifdef Q_WS_X11
            KStartupInfo::appStarted(startupId());
#endif
            return false;
        }
    } else {
        sessionManager()->activateAnonymousSession();
    }

    // oh, no mainwindow, create one, should not happen, but make sure ;)
    if (mainWindowsCount() == 0) {
        newMainWindow();
    }

    // notify about start
#ifdef Q_WS_X11
    KStartupInfo::setNewStartupId(activeKateMainWindow(), startupId());
#endif

    QTextCodec *codec = m_args.isSet(QStringLiteral("encoding")) ? QTextCodec::codecForName(m_args.value(QStringLiteral("encoding")).toUtf8()) : 0;
    bool tempfileSet = m_args.isSet(QStringLiteral("tempfile"));

    KTextEditor::Document *doc = 0;
    const QString codec_name = codec ? QString::fromLatin1(codec->name()) : QString();

    QList<QUrl> urls;
    Q_FOREACH(const QString positionalArgument, m_args.positionalArguments()) {
        QUrl url;

        // convert to an url
        QRegExp withProtocol(QStringLiteral("^[a-zA-Z]+:")); // TODO: remove after Qt supports this on its own
        if (withProtocol.indexIn(positionalArgument) == 0) {
            url = QUrl::fromUserInput(positionalArgument);
        } else {
            const QString path = QDir::current().absoluteFilePath(positionalArgument);
            url = QUrl::fromLocalFile(path);
        }

        // this file is no local dir, open it, else warn
        bool noDir = !url.isLocalFile() || !QFileInfo(url.toLocalFile()).isDir();

        if (noDir) {
            urls << url;
        } else {
            KMessageBox::sorry(activeKateMainWindow(),
                               i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", url.toString()));
        }
    }

    doc = activeKateMainWindow()->viewManager()->openUrls(urls, codec_name, tempfileSet);

    // handle stdin input
    if (m_args.isSet(QStringLiteral("stdin"))) {
        QTextStream input(stdin, QIODevice::ReadOnly);

        // set chosen codec
        if (codec) {
            input.setCodec(codec);
        }

        QString line;
        QString text;

        do {
            line = input.readLine();
            text.append(line + QLatin1Char('\n'));
        } while (!line.isNull());

        openInput(text);
    } else if (doc) {
        activeKateMainWindow()->viewManager()->activateView(doc);
    }

    int line = 0;
    int column = 0;
    bool nav = false;

    if (m_args.isSet(QStringLiteral("line"))) {
        line = m_args.value(QStringLiteral("line")).toInt() - 1;
        nav = true;
    }

    if (m_args.isSet(QStringLiteral("column"))) {
        column = m_args.value(QStringLiteral("column")).toInt() - 1;
        nav = true;
    }

    if (nav && activeKateMainWindow()->viewManager()->activeView()) {
        activeKateMainWindow()->viewManager()->activeView()->setCursorPosition(KTextEditor::Cursor(line, column));
    }

    activeKateMainWindow()->setAutoSaveSettings();

    qCDebug(LOG_KATE) << "KateApplication::init finished successful";
    return true;
}

void KateApp::shutdownKate(KateMainWindow *win)
{
    if (!win->queryClose_internal()) {
        return;
    }

    sessionManager()->saveActiveSession(true);

    // cu main windows
    while (!m_mainWindows.isEmpty()) {
        // mainwindow itself calls KateApp::removeMainWindow(this)
        delete m_mainWindows[0];
    }

    QApplication::quit();
}

KatePluginManager *KateApp::pluginManager()
{
    return m_pluginManager;
}

KateDocManager *KateApp::documentManager()
{
    return m_docManager;
}

KateSessionManager *KateApp::sessionManager()
{
    return m_sessionManager;
}

bool KateApp::openUrl(const QUrl &url, const QString &encoding, bool isTempFile)
{
    return openDocUrl(url, encoding, isTempFile);
}

KTextEditor::Document *KateApp::openDocUrl(const QUrl &url, const QString &encoding, bool isTempFile)
{
    KateMainWindow *mainWindow = activeKateMainWindow();

    if (!mainWindow) {
        return 0;
    }

    QTextCodec *codec = encoding.isEmpty() ? 0 : QTextCodec::codecForName(encoding.toLatin1());

    // this file is no local dir, open it, else warn
    bool noDir = !url.isLocalFile() || !QFileInfo(url.toLocalFile()).isDir();

    KTextEditor::Document *doc = 0;

    if (noDir) {
        // open a normal file
        if (codec) {
            doc = mainWindow->viewManager()->openUrl(url, QString::fromLatin1(codec->name()), true, isTempFile);
        } else {
            doc = mainWindow->viewManager()->openUrl(url, QString(), true, isTempFile);
        }
    } else
        KMessageBox::sorry(mainWindow,
                           i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", url.url()));

    return doc;
}

bool KateApp::setCursor(int line, int column)
{
    KateMainWindow *mainWindow = activeKateMainWindow();

    if (!mainWindow) {
        return false;
    }

    if (mainWindow->viewManager()->activeView()) {
        mainWindow->viewManager()->activeView()->setCursorPosition(KTextEditor::Cursor(line, column));
    }

    return true;
}

bool KateApp::openInput(const QString &text)
{
    activeKateMainWindow()->viewManager()->openUrl(QUrl(), QString(), true);

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
        return 0;
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
    for (int i = 0; i < m_mainWindows.size(); i++)
        if (window == m_mainWindows[i]) {
            return i;
        }
    return -1;
}

KateMainWindow *KateApp::mainWindow(int n)
{
    if (n < m_mainWindows.size()) {
        return m_mainWindows[n];
    }

    return 0;
}

void KateApp::emitDocumentClosed(const QString &token)
{
    m_adaptor->emitDocumentClosed(token);
}

KTextEditor::Plugin *KateApp::plugin(const QString &name)
{
    return m_pluginManager->plugin(name);
}

