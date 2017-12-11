/* This file is part of the KDE project
 *
 *  Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "katesessionmanager.h"

#include "katesessionchooser.h"
#include "katesessionmanagedialog.h"
#include "katesessionopendialog.h"

#include "kateapp.h"
#include "katepluginmanager.h"
#include "katerunninginstanceinfo.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>
#include <KMessageBox>
#include <KIO/CopyJob>
#include <KDesktopFile>
#include <KDirWatch>
#include <KService>
#include <KShell>

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QInputDialog>
#include <QScopedPointer>
#include <QUrl>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

//BEGIN KateSessionManager

KateSessionManager::KateSessionManager(QObject *parent, const QString &sessionsDir)
    : QObject(parent)
{
    if (sessionsDir.isEmpty()) {
        m_sessionsDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kate/sessions");
    } else {
        m_sessionsDir = sessionsDir;
    }

    // create dir if needed
    QDir().mkpath(m_sessionsDir);

    m_dirWatch = new KDirWatch(this);
    m_dirWatch->addDir(m_sessionsDir);
    connect(m_dirWatch, SIGNAL(dirty(QString)), this, SLOT(updateSessionList()));

    updateSessionList();

    m_activeSession = KateSession::createAnonymous(anonymousSessionFile());
}

KateSessionManager::~KateSessionManager()
{
    delete m_dirWatch;
}

void KateSessionManager::updateSessionList()
{
    QStringList list;

    // Let's get a list of all session we have atm
    QDir dir(m_sessionsDir, QStringLiteral("*.katesession"));

    for (unsigned int i = 0; i < dir.count(); ++i) {
        QString name = dir[i];
        name.chop(12); // .katesession
        list << QUrl::fromPercentEncoding(name.toLatin1());
    }

    // write jump list actions to disk in the kate.desktop file
    updateJumpListActions(list);

    // delete old items;
    QMutableHashIterator<QString, KateSession::Ptr> i(m_sessions);

    while (i.hasNext()) {
        i.next();
        const int idx = list.indexOf(i.key());
        if (idx == -1) { // the key is invalid, remove it from m_session
            if (i.value() != m_activeSession) { // if active, ignore missing config
                i.remove();
            }
        } else { // remove it from scan list
            list.removeAt(idx);
        }
    }

    // load the new ones
    foreach(const QString & newName, list) {
        const QString file = sessionFileForName(newName);
        m_sessions[newName] = KateSession::create(file, newName);
    }
}

bool KateSessionManager::activateSession(KateSession::Ptr session,
        const bool closeAndSaveLast,
        const bool loadNew)
{
    if (m_activeSession == session) {
        return true;
    }

    if (!session->isAnonymous()) {
        //check if the requested session is already open in another instance
        KateRunningInstanceMap instances;
        if (!fillinRunningKateAppInstances(&instances)) {
            KMessageBox::error(nullptr, i18n("Internal error: there is more than one instance open for a given session."));
            return false;
        }

        if (instances.contains(session->name())) {
            if (KMessageBox::questionYesNo(nullptr, i18n("Session '%1' is already opened in another kate instance, change there instead of reopening?", session->name()),
                                           QString(), KStandardGuiItem::yes(), KStandardGuiItem::no(), QStringLiteral("katesessionmanager_switch_instance")) == KMessageBox::Yes) {
                instances[session->name()]->dbus_if->call(QStringLiteral("activate"));
                cleanupRunningKateAppInstanceMap(&instances);
                return false;
            }
        }

        cleanupRunningKateAppInstanceMap(&instances);
    }
    // try to close and save last session
    if (closeAndSaveLast) {
        if (KateApp::self()->activeKateMainWindow()) {
            if (!KateApp::self()->activeKateMainWindow()->queryClose_internal()) {
                return true;
            }
        }

        // save last session or not?
        saveActiveSession();

        // really close last
        KateApp::self()->documentManager()->closeAllDocuments();
    }

    // set the new session
    m_activeSession = session;

    // there is one case in which we don't want the restoration and that is
    // when restoring session from session manager.
    // In that case the restore is handled by the caller
    if (loadNew) {
        loadSession(session);
    }

    emit sessionChanged();
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
    KConfigGroup c(sharedConfig, "General");

    if (c.readEntry("Restore Window Configuration", true)) {
        KConfig *cfg = sc;
        bool delete_cfg = false;
        // a new, named session, read settings of the default session.
        if (! sc->hasGroup("Open MainWindows")) {
            delete_cfg = true;
            cfg = new KConfig(anonymousSessionFile(), KConfig::SimpleConfig);
        }

        int wCount = cfg->group("Open MainWindows").readEntry("Count", 1);

        for (int i = 0; i < wCount; ++i) {
            if (i >= KateApp::self()->mainWindowsCount()) {
                KateApp::self()->newMainWindow(cfg, QString::fromLatin1("MainWindow%1").arg(i));
            } else {
                KateApp::self()->mainWindow(i)->readProperties(KConfigGroup(cfg, QString::fromLatin1("MainWindow%1").arg(i)));
            }

            KateApp::self()->mainWindow(i)->restoreWindowConfig(KConfigGroup(cfg, QString::fromLatin1("MainWindow%1 Settings").arg(i)));
        }

        if (delete_cfg) {
            delete cfg;
        }

        // remove mainwindows we need no longer...
        if (wCount > 0) {
            while (wCount < KateApp::self()->mainWindowsCount()) {
                delete KateApp::self()->mainWindow(KateApp::self()->mainWindowsCount() - 1);
            }
        }
    }
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
    saveSessionTo(s->config());
    m_sessions[name] = s;
    return s;
}

void KateSessionManager::deleteSession(KateSession::Ptr session)
{
    QFile::remove(session->file());
    if (session != activeSession()) {
        m_sessions.remove(session->name());
    }
}

bool KateSessionManager::renameSession(KateSession::Ptr session, const QString &newName)
{
    Q_ASSERT(!newName.isEmpty());

    if (session->name() == newName) {
        return true;
    }

    const QString newFile = sessionFileForName(newName);

    if (QFile::exists(newFile)) {
        KMessageBox::sorry(QApplication::activeWindow(),
                           i18n("The session could not be renamed to \"%1\", there already exists another session with the same name", newName),
                           i18n("Session Renaming"));
        return false;
    }

    session->config()->sync();

    const QUrl srcUrl = QUrl::fromLocalFile(session->file());
    const QUrl dstUrl = QUrl::fromLocalFile(newFile);
    KIO::CopyJob *job = KIO::move(srcUrl, dstUrl, KIO::HideProgressInfo);

    if (!job->exec()) {
        KMessageBox::sorry(QApplication::activeWindow(),
                           i18n("The session could not be renamed to \"%1\". Failed to write to \"%2\"", newName, newFile),
                           i18n("Session Renaming"));
        return false;
    }

    m_sessions[newName] = m_sessions.take(session->name());
    session->setName(newName);
    session->setFile(newFile);

    if (session == activeSession()) {
        emit sessionChanged();
    }

    return true;
}

void KateSessionManager::saveSessionTo(KConfig *sc) const
{
    // save plugin configs and which plugins to load
    KateApp::self()->pluginManager()->writeConfig(sc);

    // save document configs + which documents to load
    KateApp::self()->documentManager()->saveDocumentList(sc);

    sc->group("Open MainWindows").writeEntry("Count", KateApp::self()->mainWindowsCount());

    // save config for all windows around ;)
    bool saveWindowConfig = KConfigGroup(KSharedConfig::openConfig(), "General").readEntry("Restore Window Configuration", true);
    for (int i = 0; i < KateApp::self()->mainWindowsCount(); ++i) {
        KConfigGroup cg(sc, QString::fromLatin1("MainWindow%1").arg(i));
        KateApp::self()->mainWindow(i)->saveProperties(cg);
        if (saveWindowConfig) {
            KateApp::self()->mainWindow(i)->saveWindowConfig(KConfigGroup(sc, QString::fromLatin1("MainWindow%1 Settings").arg(i)));
        }
    }

    sc->sync();

    /**
     * try to sync file to disk
     */
    QFile fileToSync(sc->name());
    if (fileToSync.open(QIODevice::ReadOnly)) {
#ifndef Q_OS_WIN
        // ensure that the file is written to disk
#ifdef HAVE_FDATASYNC
        fdatasync(fileToSync.handle());
#else
        fsync(fileToSync.handle());
#endif
#endif
    }
}

bool KateSessionManager::saveActiveSession(bool rememberAsLast)
{
    KConfig *sc = activeSession()->config();

    saveSessionTo(sc);

    if (rememberAsLast) {
        KSharedConfigPtr c = KSharedConfig::openConfig();
        c->group("General").writeEntry("Last Session", activeSession()->name());
        c->sync();
    }
    return true;
}

bool KateSessionManager::chooseSession()
{
    const KConfigGroup c(KSharedConfig::openConfig(), "General");

    // get last used session, default to default session
    const QString lastSession(c.readEntry("Last Session", QString()));
    const QString sesStart(c.readEntry("Startup Session", "manual"));

    // uhh, just open last used session, show no chooser
    if (sesStart == QStringLiteral("last")) {
        activateSession(lastSession, false);
        return true;
    }

    // start with empty new session or in case no sessions exist
    if (sesStart == QStringLiteral("new") || sessionList().size() == 0) {
        activateAnonymousSession();
        return true;
    }

    QScopedPointer<KateSessionChooser> chooser(new KateSessionChooser(nullptr, lastSession));
    const int res = chooser->exec();
    bool success = true;

    switch (res) {
    case KateSessionChooser::resultOpen: {
        KateSession::Ptr s = chooser->selectedSession(); // dialog guarantees this to be valid
        success = activateSession(s, false);
        break;
    }

    case KateSessionChooser::resultCopy: {
        KateSession::Ptr s = chooser->selectedSession(); // dialog guarantees this to be valid
        KateSession::Ptr ns = KateSession::createAnonymousFrom(s, anonymousSessionFile());
        activateSession(ns, false);
        break;
    }

    // exit the app lateron
    case KateSessionChooser::resultQuit:
        return false;

    case KateSessionChooser::resultNew:
    default:
        activateAnonymousSession();
        break;
    }

    // write back our nice boolean :)
    if (success && chooser->reopenLastSession()) {
        KConfigGroup generalConfig(KSharedConfig::openConfig(), QStringLiteral("General"));

        if (res == KateSessionChooser::resultOpen) {
            generalConfig.writeEntry("Startup Session", "last");
        } else if (res == KateSessionChooser::resultNew) {
            generalConfig.writeEntry("Startup Session", "new");
        }

        generalConfig.sync();
    }

    return success;
}

void KateSessionManager::sessionNew()
{
    activateSession(giveSession(QString()));
}

void KateSessionManager::sessionOpen()
{
    QScopedPointer<KateSessionOpenDialog> chooser(new KateSessionOpenDialog(nullptr));

    const int res = chooser->exec();

    if (res == KateSessionOpenDialog::resultCancel) {
        return;
    }

    KateSession::Ptr s = chooser->selectedSession();

    if (s) {
        activateSession(s);
    }
}

void KateSessionManager::sessionSave()
{
    saveActiveSession(); // this is the optional point to handle saveSessionAs for anonymous session
}

void KateSessionManager::sessionSaveAs()
{
    if (newSessionName()) {
        saveActiveSession();
        emit sessionChanged();
    }
}

bool KateSessionManager::newSessionName()
{
    bool alreadyExists = false;

    do {
        bool ok = false;
        const QString name = QInputDialog::getText(QApplication::activeWindow(),
                             i18n("Specify New Name for Current Session"),
                             alreadyExists ? i18n("There is already an existing session with your chosen name.\nPlease choose a different one\nSession name:") : i18n("Session name:"),
                             QLineEdit::Normal, activeSession()->name(), &ok);

        if (!ok) {
            return false;
        }

        if (name.isEmpty()) {
            KMessageBox::sorry(nullptr, i18n("To save a session, you must specify a name."), i18n("Missing Session Name"));
            continue;
        }

        const QString file = sessionFileForName(name);
        if (QFile::exists(file)) {
            alreadyExists = true;
            continue;
        }

        activeSession()->config()->sync();
        KateSession::Ptr ns = KateSession::createFrom(activeSession(), file, name);
        m_activeSession = ns;

        emit sessionChanged();

        alreadyExists = false;
    } while (alreadyExists);
    return true;
}

void KateSessionManager::sessionManage()
{
    QScopedPointer<KateSessionManageDialog>(new KateSessionManageDialog(nullptr))->exec();
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

void KateSessionManager::updateJumpListActions(const QStringList &sessionList)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    KService::Ptr service = KService::serviceByStorageId(qApp->desktopFileName());
    if (!service) {
        return;
    }

    QScopedPointer<KDesktopFile> df(new KDesktopFile(service->entryPath()));

    QStringList newActions = df->readActions();

    // try to keep existing custom actions intact, only remove our "Session" actions and add them back later
    newActions.erase(std::remove_if(newActions.begin(), newActions.end(), [](const QString &action) {
        return action.startsWith(QLatin1String("Session "));
    }), newActions.end());

    // we compute the new group names in advance so we can tell whether we changed something
    // and avoid touching the desktop file leading to an expensive ksycoca recreation
    QStringList sessionActions;
    sessionActions.reserve(sessionList.count());

    std::transform(sessionList.constBegin(), sessionList.constEnd(), std::back_inserter(sessionActions), [](const QString &session) {
        return QStringLiteral("Session %1").arg(QString::fromLatin1(QCryptographicHash::hash(session.toUtf8(), QCryptographicHash::Md5).toHex()));
    });

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
    for (const QString &action : df->readActions()) {
        if (action.startsWith(QLatin1String("Session "))) {
            // TODO is there no deleteGroup(KConfigGroup)?
            df->deleteGroup(df->actionGroup(action).name());
        }
    }

    const int maxEntryCount = std::min(sessionActions.count(), 10);
    for (int i = 0; i < maxEntryCount; ++i) {
        const QString &action = sessionActions.at(i); // is a transform of sessionList, so count and order is identical
        const QString &session = sessionList.at(i);

        KConfigGroup grp = df->actionGroup(action);
        grp.writeEntry(QStringLiteral("Name"), session);
        grp.writeEntry(QStringLiteral("Exec"), QStringLiteral("kate -s %1").arg(KShell::quoteArg(session))); // TODO proper executable name?
    }

    df->desktopGroup().writeXdgListEntry("Actions", newActions);
#endif
}

//END KateSessionManager

