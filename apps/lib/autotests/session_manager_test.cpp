/*
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "session_manager_test.h"
#include "kateapp.h"
#include "katesessionmanager.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QCommandLineParser>
#include <QInputDialog>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTestWidgets>

QTEST_MAIN(KateSessionManagerTest)

KateSessionManagerTest::KateSessionManagerTest()
{
    // ensure we are in test mode, for the part, too
    QStandardPaths::setTestModeEnabled(true);

    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));

    m_tempdir = new QTemporaryDir;
    QVERIFY(m_tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(m_tempdir->path() + QStringLiteral("/testconfigfilerc"));

    // create KWrite variant to avoid plugin loading!
    static QCommandLineParser parser;
    m_app = new KateApp(parser, KateApp::ApplicationKWrite, m_tempdir->path());
    m_app->sessionManager()->activateAnonymousSession();
    m_manager = m_app->sessionManager();
}

KateSessionManagerTest::~KateSessionManagerTest()
{
    delete m_app;
    delete m_tempdir;
}

void KateSessionManagerTest::basic()
{
    QCOMPARE(m_manager->sessionsDir(), m_tempdir->path());
    QCOMPARE(m_manager->sessionList().size(), 0);
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->activeSession());
}

void KateSessionManagerTest::activateNewNamedSession()
{
    const QString sessionName = QStringLiteral("hello_world");

    QVERIFY(m_manager->activateSession(sessionName, false, false));
    QCOMPARE(m_manager->sessionList().size(), 1);

    KateSession::Ptr s = m_manager->activeSession();
    QCOMPARE(s->name(), sessionName);
    QCOMPARE(s->isAnonymous(), false);

    const QString sessionFile = m_tempdir->path() + u'/' + sessionName + QLatin1String(".katesession");
    QCOMPARE(s->config()->name(), sessionFile);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionManagerTest::anonymousSessionFile()
{
    const QString anonfile = QDir().cleanPath(m_tempdir->path() + QLatin1String("/../anonymous.katesession"));
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->activeSession()->isAnonymous());
    QCOMPARE(m_manager->activeSession()->config()->name(), anonfile);
}

void KateSessionManagerTest::urlizeSessionFile()
{
    const QString sessionName = QStringLiteral("hello world/#");

    m_manager->activateSession(sessionName, false, false);
    KateSession::Ptr s = m_manager->activeSession();

    const QString sessionFile = m_tempdir->path() + QLatin1String("/hello%20world%2F%23.katesession");
    QCOMPARE(s->config()->name(), sessionFile);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

void KateSessionManagerTest::renameSession()
{
    m_manager->activateSession(QStringLiteral("foo"));
    KateSession::Ptr s = m_manager->activeSession();

    QCOMPARE(m_manager->sessionList().size(), 1);

    const QString newName = QStringLiteral("bar");
    m_manager->renameSession(s, newName); // non-collision path
    QCOMPARE(s->name(), newName);
    QCOMPARE(m_manager->sessionList().size(), 1);
    QCOMPARE(m_manager->sessionList().constFirst(), s);

    // cleanup again for next test
    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QCOMPARE(m_manager->sessionList().size(), 0);
}

// Renaming the session stored as "Last Session" must update that entry. Else the next
// start with "Startup Session=last" would create a new empty session with the old name.
void KateSessionManagerTest::renameSessionUpdatesLastSession()
{
    QVERIFY(m_manager->activateSession(QStringLiteral("last"), false, false));
    KateSession::Ptr s = m_manager->activeSession();
    QVERIFY(m_manager->activateAnonymousSession());

    KConfigGroup c(KSharedConfig::openConfig(), QStringLiteral("General"));
    c.writeEntry("Last Session", QStringLiteral("last"));
    c.sync();
    auto cleanup = qScopeGuard([this, s, &c] {
        c.writeEntry("Last Session", QString());
        m_manager->deleteSession(s);
    });

    QCOMPARE(m_manager->renameSession(s, QStringLiteral("renamed")), QStringLiteral("renamed"));
    QCOMPARE(c.readEntry("Last Session"), QStringLiteral("renamed"));
}

#include "moc_session_manager_test.cpp"

void KateSessionManagerTest::deleteSessionRemovesStash()
{
    m_manager->activateSession(QStringLiteral("stashed"), false, false);
    KateSession::Ptr s = m_manager->activeSession();

    // the stash of the session, see KateStashManager::stashDocuments
    const QString stashPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/stash/") + QFileInfo(s->file()).fileName();
    QVERIFY(QDir().mkpath(stashPath));
    QFile stashFile(stashPath + QStringLiteral("/Document 0"));
    QVERIFY(stashFile.open(QIODevice::WriteOnly));
    stashFile.write("unsaved\n");
    stashFile.close();

    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QVERIFY(!QDir(stashPath).exists());
}

void KateSessionManagerTest::renameSessionMovesStash()
{
    m_manager->activateSession(QStringLiteral("stashBeforeRename"), false, false);
    KateSession::Ptr s = m_manager->activeSession();

    // the stash of the session, see KateStashManager::stashDocuments
    const auto stashPath = [](const KateSession::Ptr &session) {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/stash/") + QFileInfo(session->file()).fileName();
    };
    const QString oldStashPath = stashPath(s);
    QVERIFY(QDir().mkpath(oldStashPath));
    QFile stashFile(oldStashPath + QStringLiteral("/Document 0"));
    QVERIFY(stashFile.open(QIODevice::WriteOnly));
    stashFile.write("unsaved\n");
    stashFile.close();
    KConfigGroup(s->config(), QStringLiteral("Document 0")).writeEntry("stashedFile", stashFile.fileName());
    s->config()->sync();

    QCOMPARE(m_manager->renameSession(s, QStringLiteral("stashAfterRename")), QStringLiteral("stashAfterRename"));

    // the stash moved with the session, and the session refers to the moved files
    const QString newStashPath = stashPath(s);
    QVERIFY(newStashPath != oldStashPath);
    QVERIFY(!QDir(oldStashPath).exists());
    QVERIFY(QFile::exists(newStashPath + QStringLiteral("/Document 0")));
    QCOMPARE(KConfigGroup(s->config(), QStringLiteral("Document 0")).readEntry("stashedFile"), newStashPath + QStringLiteral("/Document 0"));

    QVERIFY(m_manager->activateAnonymousSession());
    QVERIFY(m_manager->deleteSession(s));
    QVERIFY(!QDir(newStashPath).exists());
}

// A copied session gets its own stash. Opening the copy removes the stash files it pops,
// so a shared stash would lose the unsaved content of the original session.
void KateSessionManagerTest::copySessionCopiesStash()
{
    m_manager->activateSession(QStringLiteral("copySource"), false, false);
    KateSession::Ptr s = m_manager->activeSession();

    // the stash of the session, see KateStashManager::stashDocuments
    const auto stashPath = [](const QString &sessionFile) {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/stash/") + QFileInfo(sessionFile).fileName();
    };
    const QString sourceStashPath = stashPath(s->file());
    QVERIFY(QDir().mkpath(sourceStashPath));
    QFile stashFile(sourceStashPath + QStringLiteral("/Document 0"));
    QVERIFY(stashFile.open(QIODevice::WriteOnly));
    stashFile.write("unsaved\n");
    stashFile.close();
    KConfigGroup(s->config(), QStringLiteral("Document 0")).writeEntry("stashedFile", stashFile.fileName());
    s->config()->sync();
    QVERIFY(m_manager->activateAnonymousSession());

    QCOMPARE(m_manager->copySession(s, QStringLiteral("copyTarget")), QStringLiteral("copyTarget"));
    const auto copies = m_manager->sessionList();
    const auto it = std::find_if(copies.cbegin(), copies.cend(), [](const KateSession::Ptr &session) {
        return session->name() == QStringLiteral("copyTarget");
    });
    QVERIFY(it != copies.cend());
    KateSession::Ptr copy = *it;
    auto cleanup = qScopeGuard([this, s, copy] {
        m_manager->deleteSession(s);
        m_manager->deleteSession(copy);
    });

    // both sessions have their own stash file with the content
    const QString copyStashPath = stashPath(copy->file());
    QVERIFY(copyStashPath != sourceStashPath);
    QVERIFY(QFile::exists(sourceStashPath + QStringLiteral("/Document 0")));
    QFile copiedFile(copyStashPath + QStringLiteral("/Document 0"));
    QVERIFY(copiedFile.open(QIODevice::ReadOnly));
    QCOMPARE(copiedFile.readAll(), QByteArrayLiteral("unsaved\n"));

    QCOMPARE(KConfigGroup(s->config(), QStringLiteral("Document 0")).readEntry("stashedFile"), sourceStashPath + QStringLiteral("/Document 0"));
    QCOMPARE(KConfigGroup(copy->config(), QStringLiteral("Document 0")).readEntry("stashedFile"), copyStashPath + QStringLiteral("/Document 0"));
    // the copy is written with the rewritten entry
    KConfig copyOnDisk(copy->file(), KConfig::SimpleConfig);
    QCOMPARE(KConfigGroup(&copyOnDisk, QStringLiteral("Document 0")).readEntry("stashedFile"), copyStashPath + QStringLiteral("/Document 0"));
}

// Renaming onto the name of an existing session asks for another name. The session must
// get that name, and the existing session must stay in the session list.
void KateSessionManagerTest::renameSessionOntoExistingNameKeepsBothSessions()
{
    QVERIFY(m_manager->activateSession(QStringLiteral("A"), false, false));
    KateSession::Ptr a = m_manager->activeSession();
    QVERIFY(m_manager->activateSession(QStringLiteral("B"), false, false));
    KateSession::Ptr b = m_manager->activeSession();
    QVERIFY(m_manager->activateAnonymousSession());
    auto cleanup = qScopeGuard([this, a, b] {
        m_manager->deleteSession(a);
        m_manager->deleteSession(b);
    });

    // accept the suggested name in the collision prompt
    QString suggested;
    QMetaObject::invokeMethod(
        this,
        [&suggested] {
            auto *dlg = qobject_cast<QInputDialog *>(qApp->activeModalWidget());
            QVERIFY(dlg);
            suggested = dlg->textValue();
            dlg->accept();
        },
        Qt::QueuedConnection);

    const QString name = m_manager->renameSession(b, QStringLiteral("A"));
    QCOMPARE(suggested, QStringLiteral("A (2)"));
    QCOMPARE(name, QStringLiteral("A (2)"));
    QCOMPARE(b->name(), name);
    QCOMPARE(a->name(), QStringLiteral("A"));

    const auto sessions = m_manager->sessionList();
    QCOMPARE(sessions.size(), 2);
    QVERIFY(sessions.contains(a));
    QVERIFY(sessions.contains(b));
    QVERIFY(QFile::exists(a->file()));
    QVERIFY(QFile::exists(b->file()));
}
