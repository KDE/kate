/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kate_view_mgmt_tests.h"
#include "katemainwindow.h"
#include "kateviewspace.h"
#include "ktexteditor_utils.h"

#include <QCommandLineParser>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(KateViewManagementTests)

KateViewManagementTests::KateViewManagementTests(QObject *)
{
    m_tempdir = new QTemporaryDir;
    QVERIFY(m_tempdir->isValid());

    // create KWrite variant to avoid plugin loading!
    static QCommandLineParser parser;
    app = std::make_unique<KateApp>(parser, KateApp::ApplicationKWrite, m_tempdir->path());
}

void KateViewManagementTests::testSingleViewspaceDoesntCloseWhenLastViewClosed()
{
    app->sessionManager()->activateAnonymousSession();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // Test that if we have 1 viewspaces then
    // closing the last view doesn't close the viewspace

    auto mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();

    // Initially we have one viewspace and one view
    QCOMPARE(vm->m_views.size(), 1);
    QCOMPARE(vm->m_viewSpaceList.size(), 1);

    // close active view
    vm->closeView(vm->activeView());

    // still same
    QCOMPARE(vm->m_views.size(), 0);
    QCOMPARE(vm->m_viewSpaceList.size(), 1);
}

void KateViewManagementTests::testViewspaceClosesWhenLastViewClosed()
{
    app->sessionManager()->activateAnonymousSession();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // Test that if we have greater than 1 viewspaces then
    // closing the last view in a viewspace closes that view
    // space

    auto mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();

    // Initially we have one viewspace
    QCOMPARE(vm->m_viewSpaceList.size(), 1);

    vm->slotSplitViewSpaceVert();

    // Now we have two
    QCOMPARE(vm->m_viewSpaceList.size(), 2);
    // and two views, one in each viewspace
    QCOMPARE(vm->m_views.size(), 2);

    // close active view
    vm->closeView(vm->activeView());

    QCOMPARE(vm->m_views.size(), 1);
    QCOMPARE(vm->m_viewSpaceList.size(), 1);
}

void KateViewManagementTests::testViewspaceClosesWhenThereIsWidget()
{
    app->sessionManager()->activateAnonymousSession();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // Test that if we have greater than 1 viewspaces then
    // closing the last view in a viewspace closes that view
    // space

    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    QCOMPARE(vm->m_viewSpaceList.size(), 1);

    vm->slotSplitViewSpaceVert();
    // Now we have two viewspaces
    QCOMPARE(vm->m_viewSpaceList.size(), 2);

    // add a widget
    QPointer<QWidget> widget = new QWidget;
    Utils::addWidget(widget, app->activeMainWindow());

    // there should still be 2 views
    // widget is not counted in views
    QCOMPARE(vm->m_views.size(), 2);

    // Widget should be active
    QVERIFY(vm->activeViewSpace()->currentWidget());

    // close active view
    // active view still points to the last active view
    // even though it is not in focus by user
    vm->closeView(vm->activeView());

    // one view left, but two viewspaces
    QCOMPARE(vm->m_views.size(), 1);
    QCOMPARE(vm->m_viewSpaceList.size(), 2);

    // close the widget!
    QVERIFY(vm->removeWidget(widget));
    QTest::qWait(100);
    QVERIFY(!widget); // widget should be gone

    // only one viewspace should be left now
    QCOMPARE(vm->m_views.size(), 1);
    QCOMPARE(vm->m_viewSpaceList.size(), 1);
}

void KateViewManagementTests::testMoveViewBetweenViewspaces()
{
    app->sessionManager()->activateAnonymousSession();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    vm->slotSplitViewSpaceVert();

    // we have two viewspaces with 2 views
    QCOMPARE(vm->m_viewSpaceList.size(), 2);
    QCOMPARE(vm->m_views.size(), 2);

    auto src = vm->activeViewSpace();
    auto dest = vm->m_viewSpaceList.front();
    QVERIFY(src != dest);
    vm->moveViewToViewSpace(dest, src, vm->activeView()->document());
    QTest::qWait(100);

    // after moving we should have 2 views but only one viewspace left
    QCOMPARE(vm->m_viewSpaceList.size(), 1);
    QCOMPARE(vm->m_views.size(), 2);
}

void KateViewManagementTests::testTwoMainWindowsCloseInitialDocument1()
{
    app->sessionManager()->activateAnonymousSession();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // get first main window
    KateMainWindow *first = app->activeKateMainWindow();
    QVERIFY(first);

    // create a second one
    KateMainWindow *second = app->newMainWindow();
    QVERIFY(second);

    // close the initial document
    QVERIFY(app->closeDocument(first->viewManager()->activeView()->document()));

    // create a new document, this did crash due to empty view space
    second->viewManager()->slotDocumentNew();
}

void KateViewManagementTests::testTwoMainWindowsCloseInitialDocument2()
{
    app->sessionManager()->activateAnonymousSession();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // get first main window
    KateMainWindow *first = app->activeKateMainWindow();
    QVERIFY(first);

    // create a second one
    KateMainWindow *second = app->newMainWindow();
    QVERIFY(second);

    // close the initial document tab in second window
    second->viewManager()->activeViewSpace()->closeDocument(first->viewManager()->activeView()->document());

    // create a new document, this did crash due to empty view space
    second->viewManager()->slotDocumentNew();
}

void KateViewManagementTests::testTwoMainWindowsCloseInitialDocument3()
{
    app->sessionManager()->activateAnonymousSession();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // get first main window
    KateMainWindow *first = app->activeKateMainWindow();
    QVERIFY(first);

    // create a second one
    KateMainWindow *second = app->newMainWindow();
    QVERIFY(second);

    // close the initial document tab in second window
    second->viewManager()->closeView(second->viewManager()->activeView());

    // create a new document, this did crash due to empty view space
    second->viewManager()->slotDocumentNew();
}
