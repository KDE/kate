/* This file is part of the KDE project
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

#include "session_test.h"
#include "katesession.h"

#include <KConfig>
#include <KConfigGroup>

#include <QtTestWidgets>
#include <QTemporaryFile>
#include <QFileInfo>

QTEST_MAIN(KateSessionTest)

void KateSessionTest::initTestCase() {}

void KateSessionTest::cleanupTestCase() {}

void KateSessionTest::init()
{
    m_tmpfile = new QTemporaryFile;
    QVERIFY(m_tmpfile->open());
}

void KateSessionTest::cleanup()
{
    delete m_tmpfile;
}

void KateSessionTest::create()
{
    const QString name = QString::fromLatin1("session name");
    KateSession::Ptr s = KateSession::create(m_tmpfile->fileName(), name);
    QCOMPARE(s->name(), name);
    QCOMPARE((int)s->documents(), 0);
    QCOMPARE(s->isAnonymous(), false);
    QCOMPARE(s->config()->name(), m_tmpfile->fileName());
}

void KateSessionTest::createAnonymous()
{
    KateSession::Ptr s = KateSession::createAnonymous(m_tmpfile->fileName());
    QCOMPARE(s->name(), QString());
    QCOMPARE((int)s->documents(), 0);
    QCOMPARE(s->isAnonymous(), true);
    QCOMPARE(s->config()->name(), m_tmpfile->fileName());
}

void KateSessionTest::createAnonymousFrom()
{
    // Regular
    KateSession::Ptr s = KateSession::create(m_tmpfile->fileName(), QLatin1String("session name"));

    const QString groupName = QString::fromLatin1("test group");
    QTemporaryFile newFile;
    newFile.open();
    KateSession::Ptr ns;

    s->config()->group(groupName).writeEntry("foo", "bar");

    // Create Anon from Other
    ns = KateSession::createAnonymousFrom(s, newFile.fileName());
    QCOMPARE(ns->name(), QString());
    QCOMPARE((int)ns->documents(), 0);
    QCOMPARE(ns->isAnonymous(), true);
    QCOMPARE(ns->config()->name(), newFile.fileName());
    QCOMPARE(ns->config()->group(groupName).readEntry("foo"), QLatin1String("bar"));
}

void KateSessionTest::createFrom()
{
    KateSession::Ptr s = KateSession::create(m_tmpfile->fileName(), QLatin1String("session name"));

    const QString newName = QString::fromLatin1("new session name");
    const QString groupName = QString::fromLatin1("test group");

    QTemporaryFile newFile;
    newFile.open();
    KateSession::Ptr ns;

    s->config()->group(groupName).writeEntry("foo", "bar");

    ns = KateSession::createFrom(s, newFile.fileName(), newName);
    QCOMPARE(ns->name(), newName);
    QCOMPARE((int)ns->documents(), 0);
    QCOMPARE(ns->isAnonymous(), false);
    QCOMPARE(ns->config()->name(), newFile.fileName());
    QCOMPARE(ns->config()->group(groupName).readEntry("foo"), QLatin1String("bar"));
}

void KateSessionTest::documents()
{
    KateSession::Ptr s = KateSession::create(m_tmpfile->fileName(), QLatin1String("session name"));

    s->setDocuments(42);
    QCOMPARE((int)s->documents(), 42);

    s->config()->sync();
    KConfig c(m_tmpfile->fileName());
    QCOMPARE(c.group("Open Documents").readEntry<int>("Count", 0), 42);
}

void KateSessionTest::setFile()
{
    KateSession::Ptr s = KateSession::create(m_tmpfile->fileName(), QLatin1String("session name"));
    s->config()->group("test").writeEntry("foo", "bar");

    QTemporaryFile file2;
    file2.open();

    s->setFile(file2.fileName());
    QCOMPARE(s->config()->name(), file2.fileName());
    QCOMPARE(s->config()->group("test").readEntry("foo"), QLatin1String("bar"));
}

void KateSessionTest::timestamp()
{
    QFileInfo i(m_tmpfile->fileName());
    KateSession::Ptr s = KateSession::create(m_tmpfile->fileName(), QLatin1String("session name"));

    QCOMPARE(s->timestamp(), i.lastModified());
}

void KateSessionTest::setName()
{
    KateSession::Ptr s = KateSession::create(m_tmpfile->fileName(), QLatin1String("session name"));
    const QString newName = QString::fromLatin1("bar");
    s->setName(newName);
    QCOMPARE(s->name(), newName);
    QCOMPARE(s->file(), m_tmpfile->fileName()); // on purpose, orthogonal
}


