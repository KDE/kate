/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "kate_view_mgmt_tests.h"
#include "katemainwindow.h"
#include "kateviewspace.h"
#include "ktexteditor_utils.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KTextEditor/Editor>

#include <QCommandLineParser>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

QTEST_MAIN(KateViewManagementTests)

static bool viewspaceContainsView(KateViewSpace *vs, KTextEditor::View *v)
{
    return vs->hasDocument(v->document());
}

static int tabIdxForDoc(QTabBar *t, KTextEditor::Document *d)
{
    Q_ASSERT(t);
    for (int i = 0; i < t->count(); ++i) {
        if (t->tabData(i).value<DocOrWidget>().doc() == d)
            return i;
    }
    return -1;
}

KateViewManagementTests::KateViewManagementTests(QObject *)
{
    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));
}

void KateViewManagementTests::init()
{
    m_tempdir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(m_tempdir->path() + QStringLiteral("/testconfigfilerc"));

    static QCommandLineParser parser;
    app.reset(); // needed as KateApp mimics a singleton
    app = std::make_unique<KateApp>(parser, KateApp::ApplicationKWrite, m_tempdir->path());
}

void KateViewManagementTests::testSingleViewspaceDoesntCloseWhenLastViewClosed()
{
    app->sessionManager()->sessionNew();
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
    app->sessionManager()->sessionNew();
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
    app->sessionManager()->sessionNew();
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
    auto *leftVS = vm->m_viewSpaceList[0];
    auto *rightVS = vm->m_viewSpaceList[1];
    QCOMPARE(rightVS, vm->activeViewSpace());

    // add a widget
    QPointer<QWidget> widget = new QWidget;
    app->activeMainWindow()->addWidget(widget);
    // active viewspace remains the same
    QCOMPARE(vm->m_viewSpaceList[1], vm->activeViewSpace());
    // the widget should be active in activeViewSpace
    QCOMPARE(vm->activeViewSpace()->currentWidget(), widget);
    // activeView() should be nullptr
    QVERIFY(!vm->activeView());

    // there should still be 2 views
    // widget is not counted in views
    QCOMPARE(vm->m_views.size(), 2);

    const auto sortedViews = vm->views();
    QCOMPARE(sortedViews.size(), 2);

    // ensure we know where both of the views are and
    // we close the right one
    QVERIFY(viewspaceContainsView(rightVS, sortedViews.at(0)));
    QVERIFY(viewspaceContainsView(leftVS, sortedViews.at(1)));

    // Make the KTE::view in right viewspace active
    vm->activateView(sortedViews.at(0));
    QVERIFY(vm->activeView() == sortedViews.at(0));

    // close active view
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
    app->sessionManager()->sessionNew();
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
    qApp->processEvents();

    // after moving we should have 2 views but only one viewspace left
    QCOMPARE(vm->m_viewSpaceList.size(), 1);
    QCOMPARE(vm->m_views.size(), 2);
}

void KateViewManagementTests::testTwoMainWindowsCloseInitialDocument1()
{
    app->sessionManager()->sessionNew();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // get first main window
    KateMainWindow *first = app->activeKateMainWindow();
    QVERIFY(first);

    // create a second one
    std::unique_ptr<KateMainWindow> second(app->newMainWindow());
    QVERIFY(second);

    // close the initial document
    QVERIFY(app->closeDocument(first->viewManager()->activeView()->document()));

    // create a new document, this did crash due to empty view space
    second->viewManager()->slotDocumentNew();
}

void KateViewManagementTests::testTwoMainWindowsCloseInitialDocument2()
{
    app->sessionManager()->sessionNew();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // get first main window
    KateMainWindow *first = app->activeKateMainWindow();
    QVERIFY(first);

    // create a second one
    std::unique_ptr<KateMainWindow> second(app->newMainWindow());
    QVERIFY(second);

    // close the initial document tab in second window
    second->viewManager()->activeViewSpace()->closeDocument(first->viewManager()->activeView()->document());

    // create a new document, this did crash due to empty view space
    second->viewManager()->slotDocumentNew();
}

void KateViewManagementTests::testTwoMainWindowsCloseInitialDocument3()
{
    app->sessionManager()->sessionNew();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // get first main window
    KateMainWindow *first = app->activeKateMainWindow();
    QVERIFY(first);

    // create a second one
    std::unique_ptr<KateMainWindow> second(app->newMainWindow());
    QVERIFY(second);

    // close the initial document tab in second window
    second->viewManager()->closeView(second->viewManager()->activeView());

    // create a new document, this did crash due to empty view space
    second->viewManager()->slotDocumentNew();
}

void KateViewManagementTests::testTabLRUWithWidgets()
{
    app->sessionManager()->sessionNew();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();

    // get first main window
    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    auto vs = vm->activeViewSpace();

    auto view1 = vm->createView(nullptr);
    auto view2 = vm->createView(nullptr);

    QCOMPARE(vs->m_registeredDocuments.size(), 3);
    // view2 should be active
    QCOMPARE(vm->activeView(), view2);

    // Add a widget
    auto *widget = new QWidget;
    widget->setObjectName(QStringLiteral("widget"));
    app->activeMainWindow()->addWidget(widget);

    QCOMPARE(vs->currentWidget(), widget);
    // There should be no activeView
    QVERIFY(!vm->activeView());

    QCOMPARE(vs->m_registeredDocuments.size(), 4);
    // activate view1
    vm->activateView(view1->document());
    QVERIFY(vm->activeView());

    // activate widget again
    QCOMPARE(vs->currentWidget(), nullptr);
    vm->activateWidget(widget);
    QCOMPARE(vs->currentWidget(), widget);
    QVERIFY(!vm->activeView());

    // close "widget"
    vm->slotDocumentClose();

    // on closing the widget we should fallback to view1
    // as it was the last visited view
    QCOMPARE(vs->m_registeredDocuments.size(), 3);
    QCOMPARE(vm->activeView(), view1);
    vm->slotDocumentClose();
    // and view2 after closing view1
    QCOMPARE(vs->m_registeredDocuments.size(), 2);
    QCOMPARE(vm->activeView(), view2);
}

void KateViewManagementTests::testViewChangedEmittedOnAddWidget()
{
    app->sessionManager()->sessionNew();
    auto kmw = app->activeMainWindow();
    QSignalSpy spy(kmw, &KTextEditor::MainWindow::viewChanged);
    kmw->addWidget(new QWidget);
    spy.wait();
    QVERIFY(spy.count() == 1);
}

void KateViewManagementTests::testWidgetAddedEmittedOnAddWidget()
{
    app->sessionManager()->sessionNew();
    QSignalSpy spy(app->activeMainWindow(), &KTextEditor::MainWindow::widgetAdded);
    app->activeMainWindow()->addWidget(new QWidget);
    spy.wait();
    QVERIFY(spy.count() == 1);
}

void KateViewManagementTests::testWidgetRemovedEmittedOnRemoveWidget()
{
    app->sessionManager()->sessionNew();
    auto mw = app->activeMainWindow();
    QSignalSpy spy(mw, &KTextEditor::MainWindow::widgetRemoved);
    auto w = new QWidget;
    mw->addWidget(w);
    mw->removeWidget(w);
    spy.wait();
    QVERIFY(spy.count() == 1);
}

void KateViewManagementTests::testActivateNotAddedWidget()
{
    app->sessionManager()->sessionNew();
    auto kmw = app->activeMainWindow();
    QSignalSpy spy(kmw, &KTextEditor::MainWindow::widgetAdded);
    QSignalSpy spy1(kmw, &KTextEditor::MainWindow::viewChanged);
    auto w = new QWidget;
    kmw->activateWidget(w);
    spy.wait();
    spy1.wait();
    QVERIFY(spy.count() == 1);
    QVERIFY(spy1.count() == 1);
}

void KateViewManagementTests::testBug460613()
{
    // See the bug for details
    // This test basically splits into two viewspaces
    // Both viewspaces have 1 view
    // Adds a new doc to first viewsace and activates
    // and then adds the same doc to second viewspace
    // without activation
    // TEST: closing the doc should only close it
    // in the first viewspace, not second as well!
    // TEST: closing the doc without view should work

    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    vm->createView(nullptr);

    vm->slotSplitViewSpaceVert();
    QCOMPARE(vm->m_viewSpaceList.size(), 2);

    auto vs1 = *vm->m_viewSpaceList.begin();
    auto vs2 = *(vm->m_viewSpaceList.begin() + 1);

    vm->setActiveSpace(vs1);
    vs1->createNewDocument();
    QCOMPARE(vm->activeView(), vs1->currentView());

    KTextEditor::Document *doc = vm->activeView()->document();
    vs2->registerDocument(doc); // registered, but has no view yet

    vm->slotDocumentClose();

    // The doc should still be there in the second viewspace
    QVERIFY(vs2->m_registeredDocuments.contains(doc));

    // Try to close the doc in second viewspace
    vs2->closeDocument(doc);
    QVERIFY(!vs2->hasDocument(doc));
}

void KateViewManagementTests::testWindowsClosesDocuments()
{
    app->sessionManager()->sessionNew();
    app->activeKateMainWindow()->viewManager()->slotDocumentNew();
    QCOMPARE(app->documentManager()->documentList().size(), 1);

    // get first main window
    KateMainWindow *first = app->activeKateMainWindow();
    QVERIFY(first);

    // create a second one, shall not create a new document
    KateMainWindow *second = app->newMainWindow();
    QCOMPARE(app->documentManager()->documentList().size(), 1);
    QVERIFY(second);

    // second window shall have a new document
    second->viewManager()->slotDocumentNew();
    QCOMPARE(app->documentManager()->documentList().size(), 2);

    // if we close the second window, the second document shall be gone
    delete second;
    QCOMPARE(app->documentManager()->documentList().size(), 1);
}

void KateViewManagementTests::testTabBarHidesShows()
{
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    clearAllDocs(mw);
    auto vm = mw->viewManager();
    auto vs = vm->activeViewSpace();
    QTabBar *tabs = vs->m_tabBar;
    vs->m_autoHideTabBar = true;
    vs->tabBarToggled();

    vm->slotDocumentClose();

    app->activeKateMainWindow()->viewManager()->slotDocumentNew();
    QCOMPARE(tabs->count(), 1);
    QVERIFY(tabs->isHidden()); // only 1 => hide

    app->activeKateMainWindow()->viewManager()->slotDocumentNew();
    QCOMPARE(tabs->count(), 2);
    QVERIFY(!tabs->isHidden()); // 2 => show

    vm->slotDocumentClose();
    QCOMPARE(tabs->count(), 1);
    QVERIFY(!vs->m_tabBar->isVisible()); // 1 -> hide

    app->activeMainWindow()->addWidget(new QWidget);
    QVERIFY(vs->m_tabBar->isVisible()); // 1 + widget => show

    vm->slotDocumentClose();
    QVERIFY(!vs->m_tabBar->isVisible()); // 1 -> hide

    vm->splitViewSpace();
    auto *secondVS = vm->activeViewSpace();
    QVERIFY(secondVS != vs);

    // if one viewspace has more than 1 tab and its
    // tabbar is visible, all other viewspaces shall
    // also have visible tabbars

    // make first vs active and create a doc in it
    // Expect: both viewspaces have tabbar visible
    vm->setActiveSpace(vs);
    vm->slotDocumentNew();
    QCOMPARE(vs->m_tabBar->count(), 2);
    QVERIFY(!vs->m_tabBar->isHidden());
    QCOMPARE(secondVS->m_tabBar->count(), 1);
    QVERIFY(!secondVS->m_tabBar->isHidden());

    // Expect both have tabbar hidden
    vm->slotDocumentClose();
    QVERIFY(vs->m_tabBar->isHidden());
    QVERIFY(secondVS->m_tabBar->isHidden());
}

void KateViewManagementTests::testNewWindowHasSameGlobalOptions()
{
    /**
     * - One mainwindow open with tabbar visible
     * - User hides tabbar
     * - Then creates a new window
     * => TEST: new window should have the tabbar hidden
     */
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    mw->openUrl(QUrl());
    mw->openUrl(QUrl());
    QAction *act = mw->action(QStringLiteral("settings_show_tab_bar"));
    QVERIFY(act && act->isCheckable());
    const bool state = act->isChecked();
    act->setChecked(!state);
    QVERIFY(mw->viewManager()->activeViewSpace()->m_tabBar->isVisible() == act->isChecked());

    // create new window
    std::unique_ptr<KateMainWindow> w2(app->newMainWindow(KateApp::self()->sessionManager()->activeSession()->config()));
    w2->openUrl(QUrl());
    w2->openUrl(QUrl());
    QCOMPARE(w2->viewManager()->activeViewSpace()->m_tabBar->isVisible(), mw->viewManager()->activeViewSpace()->m_tabBar->isVisible());
}

void KateViewManagementTests::testBug465811()
{
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    auto v1 = vm->createView(nullptr);
    auto v2 = vm->createView(nullptr);

    vm->slotSplitViewSpaceVert();
    QCOMPARE(vm->m_viewSpaceList.size(), 2);

    auto vs1 = *vm->m_viewSpaceList.begin();
    auto vs2 = *(vm->m_viewSpaceList.begin() + 1);

    // on creation there is already a view copied from first viewspace
    QCOMPARE(vs1->m_docToView.size(), 2);
    QCOMPARE(vs2->m_docToView.size(), 1);
    QCOMPARE(vs2->m_docToView.begin()->second->document(), v2->document());

    // Add an unactivated doc to the second viewspace meaning
    // there is a doc but no view for it yet
    KTextEditor::Document *doc = v1->document();
    QVERIFY(doc != v2->document());
    vs2->registerDocument(doc);

    // Activate first viewspace
    vm->setActiveSpace(vs1);

    // Precondition checks
    QCOMPARE(vm->activeViewSpace(), vs1);
    QCOMPARE(vs1->m_docToView.size(), 2);
    QCOMPARE(vs2->m_docToView.size(), 1); // 1 view
    QCOMPARE(vs2->m_registeredDocuments.size(), 2); // 2 docs available

    // Close the active tab of second viewspace(which is inactive)
    vs2->closeTabRequest(0);
    // The second document's view should be created, instead of it being created in the
    // active viewspace
    QCOMPARE(vs2->m_docToView.size(), 1);
}

void KateViewManagementTests::testBug465807()
{
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    auto v1 = vm->createView(nullptr);
    auto v2 = vm->createView(nullptr);

    vm->slotSplitViewSpaceVert();
    QCOMPARE(vm->m_viewSpaceList.size(), 2);

    auto vs1 = *vm->m_viewSpaceList.begin();
    auto vs2 = *(vm->m_viewSpaceList.begin() + 1);

    // on creation there is already a view copied from first viewspace
    QCOMPARE(vs1->m_docToView.size(), 2);
    QCOMPARE(vs2->m_docToView.size(), 1);
    QCOMPARE(vs2->m_docToView.begin()->second->document(), v2->document());

    vm->setActiveSpace(vs1);
    vm->activateView(v1);

    QCOMPARE(vs1->m_tabBar->tabData(0).value<DocOrWidget>().doc(), v1->document());
    vs1->closeTabRequest(0);

    QCOMPARE(vm->activeViewSpace(), vs1);
    QCOMPARE(vm->activeView(), v2);
}

void KateViewManagementTests::testBug465808()
{
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    auto v1 = vm->createView(nullptr);
    auto v2 = vm->createView(nullptr);

    vm->slotSplitViewSpaceVert();
    QCOMPARE(vm->m_viewSpaceList.size(), 2);

    auto vs1 = *vm->m_viewSpaceList.begin();
    auto vs2 = *(vm->m_viewSpaceList.begin() + 1);

    // on creation there is already a view copied from first viewspace
    QCOMPARE(vs1->m_docToView.size(), 2);
    QCOMPARE(vs2->m_docToView.size(), 1);
    QCOMPARE(vs2->m_docToView.begin()->second->document(), v2->document());

    // Switch back to v1
    vm->setActiveSpace(vs1);
    vm->activateView(v1);

    // Switch to second viewspace and create a view there
    vm->setActiveSpace(vs2);
    auto v3 = vm->createView();
    QCOMPARE(vs2->currentView(), v3);

    // Now vs1 has v1 active and vs2 has v3 active,
    // v2 with same doc exists in both viewspaces

    // switch to first viewspace
    vm->setActiveSpace(vs1);
    QCOMPARE(vs1->currentView(), v1);

    // while vs1 is active, close v3 of vs2
    int idx = tabIdxForDoc(vs2->m_tabBar, v3->document());
    QVERIFY(idx != -1);
    vs2->closeTabRequest(idx);

    // Closing the tab should not change the view in vs1
    QCOMPARE(vs1->currentView(), v1);
}

void KateViewManagementTests::testNewViewCreatedIfViewNotinViewspace()
{
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    auto vm = mw->viewManager();
    vm->createView(nullptr);

    vm->slotSplitViewSpaceVert();
    QCOMPARE(vm->m_viewSpaceList.size(), 2);

    auto vs1 = *vm->m_viewSpaceList.begin();
    auto vs2 = *(vm->m_viewSpaceList.begin() + 1);

    QVERIFY(vs2->isActiveSpace());

    auto v2 = vm->createView();
    QCOMPARE(vs2->m_docToView.size(), 2);

    v2->insertText(QStringLiteral("Line1\nLine2\nLine3"));
    v2->setCursorPosition({2, 2});

    // activate first space
    vm->setActiveSpace(vs1);
    // Try to activate the doc for which there is no view
    // in first space
    vm->activateView(v2->document());
    // first space should now have 2 views i.e., a view
    // should be created for v2->document()
    QCOMPARE(vs1->m_docToView.size(), 2);
    QCOMPARE(vs1->m_registeredDocuments, vs2->m_registeredDocuments);
    // Cursor position is maintained in the new view
    QCOMPARE(vs1->m_docToView[v2->document()]->cursorPosition(), v2->cursorPosition());

    v2->document()->clear();
    v2->document()->setModified(false);
}

void KateViewManagementTests::testNewSessionClearsWindowWidgets()
{
    // BUG: 466526
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    clearAllDocs(mw);

    QPointer<QWidget> w1 = new QWidget;
    QPointer<QWidget> w2 = new QWidget;
    app->activeMainWindow()->addWidget(w1);
    app->activeMainWindow()->addWidget(w2);

    QCOMPARE(mw->viewManager()->activeViewSpace()->m_registeredDocuments.size(), 2);

    app->sessionManager()->sessionNew();

    qApp->processEvents();

    // Widgets are gone now
    QCOMPARE(mw->viewManager()->activeViewSpace()->m_registeredDocuments.size(), 1);
    QVERIFY(!w1);
    QVERIFY(!w2);
}

void KateViewManagementTests::testViewspaceWithWidgetDoesntCrashOnClose()
{
    // two viewspaces, one with widget. Closing the space shouldn't crash us
    app->sessionManager()->sessionNew();
    KateMainWindow *mw = app->activeKateMainWindow();
    clearAllDocs(mw);

    auto vm = mw->viewManager();
    vm->createView();
    auto vs1 = vm->activeViewSpace();
    vm->slotSplitViewSpaceVert();
    QVERIFY(vs1 != vm->activeViewSpace());
    QCOMPARE(vm->m_viewSpaceList.size(), 2);
    QPointer<KateViewSpace> vs2 = vm->activeViewSpace();

    QPointer<QWidget> w1 = new QWidget;
    app->activeMainWindow()->addWidget(w1);
    QCOMPARE(vs2->currentWidget(), w1);
    QVERIFY(vm->activeView() == nullptr);

    QSignalSpy spy(app->activeMainWindow(), &KTextEditor::MainWindow::widgetRemoved);

    // close the viewspace
    vm->slotCloseCurrentViewSpace();

    // The widget and viewspace should be gone
    QVERIFY(!vs2);
    QVERIFY(!w1);
    QVERIFY(spy.count() == 1);
}

void KateViewManagementTests::testDetachDoc()
{
    // two viewspaces, one with widget. Closing the space shouldn't crash us
    app->sessionManager()->sessionNew();
    for (auto mw : app->mainWindows()) {
        if (mw != app->activeMainWindow()) {
            delete mw->window();
        }
    }
    KateMainWindow *mw = app->activeKateMainWindow();
    clearAllDocs(mw);
    QCOMPARE(app->mainWindowsCount(), 1);

    auto vm = mw->viewManager();
    auto doc = vm->createView()->document();
    auto action = mw->action(QStringLiteral("detach_active_view_doc"));
    action->trigger();
    QCOMPARE(app->mainWindowsCount(), 2);
    QVERIFY(app->activeKateMainWindow()->viewManager()->activeView()->document() == doc);
}

void KateViewManagementTests::testKwriteInSDIModeWithOpenMultipleUrls()
{
    /**
     * In this test, we simulate opening KWrite from the terminal and passing in 2 docs
     * - KWrite is in SDI Mode
     * - We should get 2 windows for 2 docs
     */

    app->sessionManager()->sessionNew();
    for (auto mw : app->mainWindows()) {
        if (mw != app->activeMainWindow()) {
            delete mw->window();
        }
    }

    {
        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
        cgGeneral.writeEntry("SDI Mode", true);
        app->configurationChanged();
    }

    KateMainWindow *mw = app->activeKateMainWindow();
    QVERIFY(mw->viewManager()->m_sdiMode);
    clearAllDocs(mw);
    QCOMPARE(app->mainWindowsCount(), 1);
    QCOMPARE(mw->views().size(), 0);

    const QDir d = QDir::current();
    const QUrl file1 = QUrl::fromLocalFile(d.absoluteFilePath(QStringLiteral("File1")));
    const QUrl file2 = QUrl::fromLocalFile(d.absoluteFilePath(QStringLiteral("File2")));

    app->openDocUrl(file1, QString(), false, /*activateView=*/false);
    app->openDocUrl(file2, QString(), false, /*activateView=*/false);

    QCOMPARE(app->mainWindowsCount(), 2);

    for (auto mw : app->mainWindows()) {
        QCOMPARE(mw->views().size(), 1);
    }

    // fallback to default
    {
        KSharedConfig::Ptr config = KSharedConfig::openConfig();
        KConfigGroup cgGeneral = KConfigGroup(config, QStringLiteral("General"));
        cgGeneral.deleteEntry("SDI Mode");
        app->configurationChanged();
    }
}

#include "moc_kate_view_mgmt_tests.cpp"
