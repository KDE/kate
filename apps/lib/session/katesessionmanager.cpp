/*
    SPDX-FileCopyrightText: 2005 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katesessionmanager.h"

#include "katesessionmanagedialog.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "katepluginmanager.h"

#ifdef WITH_DBUS
#include "katerunninginstanceinfo.h"
#endif

#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>
#include <KMessageBox>
#include <KService>
#include <KSharedConfig>
#include <KShell>

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QInputDialog>
#include <QTimer>
#include <QUrl>

#ifndef Q_OS_WIN
#include <unistd.h>
#else
#include <io.h>
#endif

namespace
{
QStringList getAllSessionsInDir(const QString &dirPath)
{
    // Let's get a list of all session we have atm
    QDir dir(dirPath, QStringLiteral("*.katesession"), QDir::Time);
    QStringList sessionList;
    sessionList.reserve(dir.count());
    for (unsigned int i = 0; i < dir.count(); ++i) {
        QString name = dir[i];
        name.chop(12); // .katesession
        sessionList << QUrl::fromPercentEncoding(name.toLatin1());
    }
    return sessionList;
}
}

// BEGIN KateSessionManager

KateSessionManager::KateSessionManager(QObject *parent, const QString &sessionsDir)
    : QObject(parent)
    , m_sessionsDir(sessionsDir)
{
    // create dir if needed
    Q_ASSERT(!m_sessionsDir.isEmpty());
    QDir().mkpath(m_sessionsDir);

    // monitor our session directory for outside changes
    m_dirWatch.addPath(m_sessionsDir);
    connect(&m_dirWatch, &QFileSystemWatcher::directoryChanged, this, &KateSessionManager::updateSessionList);

    // initial creation of the session list from disk files
    updateSessionList();

    // init the session auto save
    m_sessionSaveTimer.setInterval(5000);
    m_sessionSaveTimer.setSingleShot(true);
    auto startTimer = [this] {
        if (m_sessionSaveTimerBlocked == 0 && !m_sessionSaveTimer.isActive()) {
            m_sessionSaveTimer.start();
        }
    };
    auto dm = KateApp::self()->documentManager();
    connect(dm, &KateDocManager::documentCreated, &m_sessionSaveTimer, startTimer);
    connect(dm, &KateDocManager::documentDeleted, &m_sessionSaveTimer, startTimer);
    m_sessionSaveTimer.callOnTimeout(this, [this] {
        if (m_sessionSaveTimerBlocked == 0) {
            saveActiveSession(true, /*isAutoSave=*/true);
        }
    });
}

KateSessionManager::~KateSessionManager()
{
    // write jump list actions to disk in the kate.desktop file
    updateJumpListActions();
}

void KateSessionManager::updateSessionList()
{
    const QStringList list = getAllSessionsInDir(m_sessionsDir);
    bool changed = false;
    // Add new sessions to our list
    for (const QString &session : std::as_const(list)) {
        if (!m_sessions.contains(session)) {
            const QString file = sessionFileForName(session);
            m_sessions.insert(session, KateSession::create(file, session));
            changed = true;
        }
    }

    // Remove gone sessions from our list
    for (auto it = m_sessions.begin(); it != m_sessions.end();) {
        if (list.indexOf(it.key()) < 0 && it.value() != activeSession()) {
            it = m_sessions.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    if (changed) {
        Q_EMIT sessionListChanged();
    }
}

bool KateSessionManager::activateSession(KateSession::Ptr session, const bool closeAndSaveLast, const bool loadNew)
{
    if (activeSession() == session) {
        return true;
    }

    // we want no auto saving during session switches
    AutoSaveBlocker blocker(this);

#ifdef WITH_DBUS
    if (!session->isAnonymous()) {
        // check if the requested session is already open in another instance
        const std::vector<KateRunningInstanceInfo> instances = fillinRunningKateAppInstances();
        for (const auto &instance : instances) {
            if (session->name() == instance.sessionName) {
                if (KMessageBox::questionTwoActions(
                        nullptr,
                        i18n("Session '%1' is already opened in another Kate instance. Switch to that or reopen in this instance?", session->name()),
                        QString(),
                        KGuiItem(i18nc("@action:button", "Switch to Instance"), QStringLiteral("window")),
                        KGuiItem(i18nc("@action:button", "Reopen"), QStringLiteral("document-open")),
                        QStringLiteral("katesessionmanager_switch_instance"))
                    == KMessageBox::PrimaryAction) {
                    instance.dbus_if->call(QStringLiteral("activate"));
                    return false;
                }
                break;
            }
        }
    }
#endif

    // try to close and save last session
    if (closeAndSaveLast) {
        if (KateApp::self()->activeKateMainWindow()) {
            if (!KateApp::self()->activeKateMainWindow()->queryClose_internal()) {
                return false;
            }
        }

        // Remove all open widgets
        const auto mainWindows = KateApp::self()->mainWindowsCount();
        for (int i = 0; i < mainWindows; ++i) {
            if (KateMainWindow *win = KateApp::self()->mainWindow(i)) {
                const auto widgets = win->widgets();
                for (auto w : widgets) {
                    win->removeWidget(w);
                }
            }
        }

        // save last session or not?
        saveActiveSession();

        // really close last
        // dont closeUrl, The queryClose_internal() should already have queried about
        // modified documents correctly. Doing this again leads to one dialog per doc
        // even when we have stashing enabled.
        if (!KateApp::self()->documentManager()->closeAllDocuments(/*closeUrl=*/false)) {
            // can still fail for modified files, bug 466553
            return false;
        }
    }

    // set the new session
    m_activeSession = session;

    // there is one case in which we don't want the restoration and that is
    // when restoring session from session manager.
    // In that case the restore is handled by the caller
    if (loadNew) {
        loadSession(session);
    }

    Q_EMIT sessionChanged();
    return true;
}

void KateSessionManager::loadSession(const KateSession::Ptr &session) const
{
    // open the new session
    KSharedConfigPtr sharedConfig = KSharedConfig::openConfig();
    KConfig *sc = session->config();
    const bool loadDocs = !session->isAnonymous(); // do not load docs for new sessions

    // if we have no session config object, try to load the default
    // (anonymous/unnamed sessions)
    // load plugin config + plugins
    KateApp::self()->pluginManager()->loadConfig(sc);

    if (loadDocs) {
        KateApp::self()->documentManager()->restoreDocumentList(sc);
    }

    // window config
    KConfigGroup c(sharedConfig, QStringLiteral("General"));

    KConfig *cfg = sc;
    bool delete_cfg = false;
    // a new, named session, read settings of the default session.
    if (!sc->hasGroup(QLatin1String("Open MainWindows"))) {
        delete_cfg = true;
        cfg = new KConfig(anonymousSessionFile(), KConfig::SimpleConfig);
    }

    if (c.readEntry("Restore Window Configuration", true)) {
        int wCount = cfg->group(QStringLiteral("Open MainWindows")).readEntry("Count", 1);

        for (int i = 0; i < wCount; ++i) {
            if (i >= KateApp::self()->mainWindowsCount()) {
                KateApp::self()->newMainWindow(cfg, QStringLiteral("MainWindow%1").arg(i));
            } else {
                KateApp::self()->mainWindow(i)->readProperties(KConfigGroup(cfg, QStringLiteral("MainWindow%1").arg(i)));
            }

            KateApp::self()->mainWindow(i)->restoreWindowConfig(KConfigGroup(cfg, QStringLiteral("MainWindow%1 Settings").arg(i)));
        }

        // remove mainwindows we need no longer...
        if (wCount > 0) {
            while (wCount < KateApp::self()->mainWindowsCount()) {
                delete KateApp::self()->mainWindow(KateApp::self()->mainWindowsCount() - 1);
            }
        }
    } else {
        // load recent files for all existing windows, see bug 408499
        for (int i = 0; i < KateApp::self()->mainWindowsCount(); ++i) {
            KateApp::self()->mainWindow(i)->loadOpenRecent(cfg);
        }
    }

    // ensure we have at least one window, always! load recent files for it, too, see bug 408499
    if (KateApp::self()->mainWindowsCount() == 0) {
        auto w = KateApp::self()->newMainWindow();
        w->loadOpenRecent(cfg);
    }

    if (delete_cfg) {
        delete cfg;
    }

    // we shall always have some existing windows here!
    Q_ASSERT(KateApp::self()->mainWindowsCount() > 0);
}

bool KateSessionManager::activateSession(const QString &name, const bool closeAndSaveLast, const bool loadNew)
{
    return activateSession(giveSession(name), closeAndSaveLast, loadNew);
}

bool KateSessionManager::activateAnonymousSession()
{
    return activateSession(QString(), false);
}

KateSession::Ptr KateSessionManager::giveSession(const QString &name)
{
    if (name.isEmpty()) {
        return KateSession::createAnonymous(anonymousSessionFile());
    }

    if (m_sessions.contains(name)) {
        return m_sessions.value(name);
    }

    KateSession::Ptr s = KateSession::create(sessionFileForName(name), name);
    saveSessionTo(s->config(), /*isAutoSave=*/false);
    m_sessions[name] = s;
    // Due to this add to m_sessions will updateSessionList() no signal emit,
    // but it's important to add. Otherwise could it be happen that m_activeSession
    // is not part of m_sessions but a double
    Q_EMIT sessionListChanged();

    return s;
}

bool KateSessionManager::deleteSession(KateSession::Ptr session)
{
    if (sessionIsActive(session->name())) {
        return false;
    }

    KConfigGroup c(KSharedConfig::openConfig(), QStringLiteral("General"));
    if (c.readEntry("Last Session") == session->name()) {
        c.writeEntry("Last Session", QString());
        c.sync();
    }

    QFile::remove(session->file());

    // ensure session list is updated
    m_sessions.remove(session->name());
    Q_EMIT sessionListChanged();
    return true;
}

QString KateSessionManager::copySession(const KateSession::Ptr &session, const QString &newName)
{
    const QString name = askForNewSessionName(session, newName);

    if (name.isEmpty()) {
        return name;
    }

    const QString newFile = sessionFileForName(name);

    KateSession::Ptr ns = KateSession::createFrom(session, newFile, name);
    ns->config()->sync();

    // ensure session list is updated
    m_sessions[ns->name()] = ns;
    Q_EMIT sessionListChanged();
    return name;
}

QString KateSessionManager::renameSession(KateSession::Ptr session, const QString &newName)
{
    const QString name = askForNewSessionName(session, newName);

    if (name.isEmpty()) {
        return name;
    }

    const QString newFile = sessionFileForName(name);

    session->config()->sync();

    if (!QFile::rename(session->file(), newFile)) {
        KMessageBox::error(QApplication::activeWindow(),
                           i18n("The session could not be renamed to \"%1\". Failed to write to \"%2\"", newName, newFile),
                           i18n("Session Renaming"));
        return QString();
    }

    // update session name and sync
    const auto oldName = session->name();
    session->setName(newName);
    session->setFile(newFile);
    session->config()->sync();

    // ensure session list is updated
    m_sessions[session->name()] = m_sessions.take(oldName);
    Q_EMIT sessionListChanged();

    if (session == activeSession()) {
        Q_EMIT sessionChanged();
    }

    return name;
}

void KateSessionManager::saveSessionTo(KConfig *sc, bool isAutoSave)
{
    if (!isAutoSave) {
        // Clear the session file to avoid to accumulate outdated entries
        const QStringList groupList = sc->groupList();
        for (const auto &group : groupList) {
            // Don't delete groups for loaded documents that have
            // ViewSpace config in session but do not have any views.
            if (!isViewLessDocumentViewSpaceGroup(group)) {
                sc->deleteGroup(group);
            }
        }
    }

    // save plugin configs and which plugins to load
    KateApp::self()->pluginManager()->writeConfig(sc);

    // save document configs + which documents to load
    KateApp::self()->documentManager()->saveDocumentList(sc);

    sc->group(QStringLiteral("Open MainWindows")).writeEntry("Count", KateApp::self()->mainWindowsCount());

    // save config for all windows around ;)
    bool saveWindowConfig = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("General")).readEntry("Restore Window Configuration", true);
    for (int i = 0; i < KateApp::self()->mainWindowsCount(); ++i) {
        KConfigGroup cg(sc, QStringLiteral("MainWindow%1").arg(i));
        // saveProperties() handles saving the "open recent" files list
        // don't store splitters and co. for KWrite
        // see bug 461355 and bug 459366 and bug 463139
        KateApp::self()->mainWindow(i)->saveProperties(cg, KateApp::isKate());
        if (saveWindowConfig) {
            KateApp::self()->mainWindow(i)->saveWindowConfig(KConfigGroup(sc, QStringLiteral("MainWindow%1 Settings").arg(i)));
        }
    }

    // Stash docs if the session is not anon
    if (auto active = activeSession()) {
        if (sc == active->config() && !active->isAnonymous()) {
            KateApp::self()->stashManager()->stashDocuments(sc, KateApp::self()->documents());
        }
    }

    sc->sync();

    /**
     * try to sync file to disk
     */
    QFile fileToSync(sc->name());
    // open read-write for _commit, don't use WriteOnly, that will truncate
    if (fileToSync.open(QIODevice::ReadWrite)) {
#ifndef Q_OS_WIN
        // ensure that the file is written to disk
#ifdef HAVE_FDATASYNC
        fdatasync(fileToSync.handle());
#else
        fsync(fileToSync.handle());
#endif
#else
        _commit(fileToSync.handle());
#endif
    }
}

bool KateSessionManager::saveActiveSession(bool rememberAsLast, bool isAutoSave)
{
    if (!activeSession()) {
        return false;
    }

    KConfig *sc = activeSession()->config();

    saveSessionTo(sc, isAutoSave);

    if (rememberAsLast && !activeSession()->isAnonymous()) {
        KSharedConfigPtr c = KSharedConfig::openConfig();
        c->group(QStringLiteral("General")).writeEntry("Last Session", activeSession()->name());
        c->sync();
    }
    return true;
}

bool KateSessionManager::chooseSession()
{
    const KConfigGroup c(KSharedConfig::openConfig(), QStringLiteral("General"));

    // get last used session, default to default session
    const QString lastSession(c.readEntry("Last Session", QString()));
    const QString sesStart(c.readEntry("Startup Session", "manual"));

    // uhh, just open last used session, show no chooser
    if (sesStart == QLatin1String("last")) {
        return activateSession(lastSession, false);
    }

    // start with empty new session or in case no sessions exist
    if (sesStart == QLatin1String("new") || sessionList().empty()) {
        return activateAnonymousSession();
    }

    // else: ask the user
    KateSessionManageDialog dlg(nullptr, lastSession);
    return dlg.exec();
}

void KateSessionManager::sessionNew()
{
    activateSession(giveSession(QString()));
}

void KateSessionManager::sessionSave()
{
    if (activeSession() && activeSession()->isAnonymous()) {
        sessionSaveAs();
    } else {
        saveActiveSession();
    }
}

void KateSessionManager::sessionSaveAs()
{
    const QString newName = askForNewSessionName(activeSession());

    if (newName.isEmpty()) {
        return;
    }

    activeSession()->config()->sync();

    KateSession::Ptr ns = KateSession::createFrom(activeSession(), sessionFileForName(newName), newName);
    m_activeSession = ns;
    saveActiveSession();

    Q_EMIT sessionChanged();
}

QString KateSessionManager::askForNewSessionName(KateSession::Ptr session, const QString &newName)
{
    if (session->name() == newName && !session->isAnonymous()) {
        return QString();
    }

    const QString messagePrompt = i18n("Session name:");
    const KLocalizedString messageExist = ki18n(
        "There is already an existing session with your chosen name: %1\n"
        "Please choose a different one.");
    const QString messageEmpty = i18n("To save a session, you must specify a name.");

    QString messageTotal = messagePrompt;
    QString name = newName;

    while (true) {
        QString preset = name;

        if (name.isEmpty()) {
            preset = suggestNewSessionName(session->name());
            messageTotal = messageEmpty + QLatin1String("\n\n") + messagePrompt;

        } else if (QFile::exists(sessionFileForName(name))) {
            preset = suggestNewSessionName(name);
            if (preset.isEmpty()) {
                // Very unlikely, but as fall back we keep users input
                preset = name;
            }
            messageTotal = messageExist.subs(name).toString() + QLatin1String("\n\n") + messagePrompt;

        } else {
            return name;
        }

        QInputDialog dlg(KateApp::self()->activeKateMainWindow());
        dlg.setInputMode(QInputDialog::TextInput);
        if (session->isAnonymous()) {
            dlg.setWindowTitle(i18n("Specify a name for this session"));
        } else {
            dlg.setWindowTitle(i18n("Specify a new name for session: %1", session->name()));
        }
        dlg.setLabelText(messageTotal);
        dlg.setTextValue(preset);
        dlg.resize(900, 100); // FIXME Calc somehow a proper size
        bool ok = dlg.exec();
        name = dlg.textValue();

        if (!ok) {
            return QString();
        }
    }
}

QString KateSessionManager::suggestNewSessionName(const QString &target)
{
    if (target.isEmpty()) {
        // Here could also a default name set or the current session name used
        return QString();
    }

    const QString mask = QStringLiteral("%1 (%2)");
    QString name;

    for (int i = 2; i < 1000000; i++) { // Should be enough to get an unique name
        name = mask.arg(target).arg(i);

        if (!QFile::exists(sessionFileForName(name))) {
            return name;
        }
    }

    return QString();
}

void KateSessionManager::sessionManage()
{
    KateSessionManageDialog dlg(KateApp::self()->activeKateMainWindow());
    dlg.exec();
}

bool KateSessionManager::sessionIsActive(const QString &session)
{
    // first check the current instance, that is skipped below
    if (activeSession() && activeSession()->name() == session) {
        return true;
    }

#ifdef WITH_DBUS
    // check if the requested session is open in another instance
    const std::vector<KateRunningInstanceInfo> instances = fillinRunningKateAppInstances();
    const auto it = std::find_if(instances.cbegin(), instances.cend(), [&session](const auto &instance) {
        return instance.sessionName == session;
    });
    return it != instances.end();
#else
    return false;
#endif
}

QString KateSessionManager::anonymousSessionFile() const
{
    const QString file = m_sessionsDir + QStringLiteral("/../anonymous.katesession");
    return QDir().cleanPath(file);
}

QString KateSessionManager::sessionFileForName(const QString &name) const
{
    Q_ASSERT(!name.isEmpty());
    const QString sname = QString::fromLatin1(QUrl::toPercentEncoding(name, QByteArray(), QByteArray(".")));
    return m_sessionsDir + QStringLiteral("/") + sname + QStringLiteral(".katesession");
}

KateSessionList KateSessionManager::sessionList()
{
    return m_sessions.values();
}

void KateSessionManager::updateJumpListActions()
{
    // Let's get a list of all session we have atm
    QStringList sessionList = getAllSessionsInDir(m_sessionsDir);
    if (sessionList.isEmpty()) {
        return;
    }

    KService::Ptr service = KService::serviceByStorageId(qApp->desktopFileName());
    if (!service) {
        return;
    }

    std::unique_ptr<KDesktopFile> df(new KDesktopFile(service->entryPath()));

    QStringList newActions = df->readActions();

    // try to keep existing custom actions intact, only remove our "Session" actions and add them back later
    newActions.erase(std::remove_if(newActions.begin(),
                                    newActions.end(),
                                    [](const QString &action) {
                                        return action.startsWith(QLatin1String("Session "));
                                    }),
                     newActions.end());

    // Limit the number of list entries we like to offer
    const int maxEntryCount = std::min<int>(sessionList.count(), 10);

    // sessionList is ordered by time, but we like it alphabetical to avoid even more a needed update
    std::sort(sessionList.begin(), sessionList.begin() + maxEntryCount);

    // we compute the new group names in advance so we can tell whether we changed something
    // and avoid touching the desktop file leading to an expensive ksycoca recreation
    QStringList sessionActions;
    sessionActions.reserve(maxEntryCount);

    for (int i = 0; i < maxEntryCount; ++i) {
        sessionActions
            << QStringLiteral("Session %1").arg(QString::fromLatin1(QCryptographicHash::hash(sessionList.at(i).toUtf8(), QCryptographicHash::Md5).toHex()));
    }

    newActions += sessionActions;

    // nothing to do
    if (df->readActions() == newActions) {
        return;
    }

    const QString &localPath = service->locateLocal();
    if (service->entryPath() != localPath) {
        df.reset(df->copyTo(localPath));
    }

    // remove all Session action groups first to not leave behind any cruft
    const QStringList actions = df->readActions();
    for (const QString &action : actions) {
        if (action.startsWith(QLatin1String("Session "))) {
            // TODO is there no deleteGroup(QLatin1String(KConfigGroup))?
            df->deleteGroup(df->actionGroup(action).name());
        }
    }

    for (int i = 0; i < maxEntryCount; ++i) {
        const QString &action = sessionActions.at(i); // is a transform of sessionSubList, so count and order is identical
        const QString &session = sessionList.at(i);

        KConfigGroup grp = df->actionGroup(action);
        grp.writeEntry("Name", session);
        grp.writeEntry("Exec", QStringLiteral("kate -n -s %1").arg(KShell::quoteArg(session)));
    }

    df->desktopGroup().writeXdgListEntry("Actions", newActions);
}

bool KateSessionManager::isViewLessDocumentViewSpaceGroup(const QString &group)
{
    if (KateApp::self()->sessionManager()->activeSession()->isAnonymous()) {
        return false;
    }

    if (!group.startsWith(QStringLiteral("MainWindow"))) {
        return false;
    }

    static const QRegularExpression re(QStringLiteral("^MainWindow\\d+\\-ViewSpace\\s\\d+\\s(.*)$"));
    QRegularExpressionMatch match = re.match(group);
    if (match.hasMatch()) {
        QUrl url(match.captured(1));
        auto *docMan = KateApp::self()->documentManager();
        auto *doc = docMan->findDocument(url);
        if (doc && doc->views().empty() && docMan->documentList().contains(doc)) {
            return true;
        }
    }
    return false;
}

// END KateSessionManager

#include "moc_katesessionmanager.cpp"
