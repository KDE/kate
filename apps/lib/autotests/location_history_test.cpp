/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "location_history_test.h"
#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "kateviewspace.h"

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(LocationHistoryTest)

LocationHistoryTest::LocationHistoryTest(QObject *parent)
    : QObject(parent)
{
    app = new KateApp(QCommandLineParser());
    app->sessionManager()->activateAnonymousSession();
}

LocationHistoryTest::~LocationHistoryTest()
{
    delete app;
}

void LocationHistoryTest::init()
{
}

void LocationHistoryTest::cleanup()
{
    auto vs = viewSpace();
    Q_ASSERT(vs);
    vs->locationHistoryBuffer().clear();
    vs->currentLoc() = 0;
}

KateViewSpace *LocationHistoryTest::viewSpace()
{
    Q_ASSERT(viewManager());
    return viewManager()->activeViewSpace();
}

KateViewManager *LocationHistoryTest::viewManager()
{
    auto mw = app->activeKateMainWindow();
    Q_ASSERT(mw);
    return mw->viewManager();
}

static int viewLineCount(KTextEditor::View *v)
{
    Q_ASSERT(v);
    return v->lastDisplayedLine() - v->firstDisplayedLine();
}

void LocationHistoryTest::test_addLocationInvalidUrl()
{
    auto vs = viewSpace();
    Q_ASSERT(vs);

    // location should be zero
    QCOMPARE(vs->currentLoc(), 0);

    // invalid url will have no effect

    vs->addPositionToHistory(QUrl(), {1, 1});
    QCOMPARE(vs->currentLoc(), 0);
    QCOMPARE(vs->locationHistoryBuffer().size(), 0);
}

void LocationHistoryTest::test_addLocation()
{
    auto vs = viewSpace();
    auto vm = viewManager();
    Q_ASSERT(vs && vm);

    const auto currentFileUrl = QUrl::fromLocalFile(QString::fromLatin1(__FILE__));

    vm->openUrl(currentFileUrl, QString());

    QCOMPARE(vs->currentLoc(), 0);
    QCOMPARE(vs->locationHistoryBuffer().size(), 0);

    // Add first position

    vm->addPositionToHistory(currentFileUrl, {0, 0});

    QCOMPARE(vs->currentLoc(), 0);
    QCOMPARE(vs->locationHistoryBuffer().size(), 1);
    QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(0, 0));

    // Adding same position again will have no affect

    vm->addPositionToHistory(currentFileUrl, {0, 0});

    QCOMPARE(vs->currentLoc(), 0);
    QCOMPARE(vs->locationHistoryBuffer().size(), 1);
    QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(0, 0));

    // Adding position on same line with different column will update the existing cursor
    // But no new positions will be added to the buffer
    vm->addPositionToHistory(currentFileUrl, {0, 2});

    QCOMPARE(vs->currentLoc(), 0);
    QCOMPARE(vs->locationHistoryBuffer().size(), 1);
    QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(0, 2));

    // Adding position, but via internal call i.e., cursorPositionChanged will only have an affect if
    // new position is at least "viewLineCount" away
    vs->addPositionToHistory(currentFileUrl, {0, 3});
    QCOMPARE(vs->currentLoc(), 0);
    QCOMPARE(vs->locationHistoryBuffer().size(), 1);
    QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(0, 3));

    const int viewLineCnt = viewLineCount(vm->activeView());
    vs->addPositionToHistory(currentFileUrl, {0 + viewLineCnt + 1, 0});
    QCOMPARE(vs->currentLoc(), 1);
    QCOMPARE(vs->locationHistoryBuffer().size(), 2);
    QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(0 + viewLineCnt + 1, 0));
}

void LocationHistoryTest::test_addMaxLocations()
{
    auto vs = viewSpace();
    auto vm = viewManager();
    Q_ASSERT(vs && vm);

    const auto currentFileUrl = QUrl::fromLocalFile(QString::fromLatin1(__FILE__));

    for (int i = 0; i < 100; ++i) {
        vm->addPositionToHistory(currentFileUrl, {i, 0});
    }

    QCOMPARE(vs->locationHistoryBuffer().size(), 50);
    QCOMPARE(vs->currentLoc(), 49);
    QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(99, 0));
}

void LocationHistoryTest::test_goBackForward()
{
    auto vs = viewSpace();
    auto vm = viewManager();
    Q_ASSERT(vs && vm);

    const auto currentFileUrl = QUrl::fromLocalFile(QString::fromLatin1(__FILE__));

    for (int i = 0; i <= 49; ++i) {
        vm->addPositionToHistory(currentFileUrl, {i, 0});
    }
    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), false);

    QCOMPARE(vs->locationHistoryBuffer().size(), 50);
    QCOMPARE(vs->currentLoc(), 49);

    for (int i = 49; i >= 0; --i) {
        QCOMPARE(vs->currentLoc(), i);
        QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(i, 0));
        vs->goBack();
    }
    QCOMPARE(vs->isHistoryBackEnabled(), false);
    QCOMPARE(vs->isHistoryForwardEnabled(), true);

    for (int i = 0; i < 49; ++i) {
        QCOMPARE(vs->currentLoc(), i);
        QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(i, 0));
        vs->goForward();
    }
    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), false);
}

void LocationHistoryTest::test_goBackForward2()
{
    auto vs = viewSpace();
    auto vm = viewManager();
    Q_ASSERT(vs && vm);

    const auto currentFileUrl = QUrl::fromLocalFile(QString::fromLatin1(__FILE__));

    // add 5 positions
    for (int i = 0; i < 5; ++i) {
        vm->addPositionToHistory(currentFileUrl, {i, 0});
    }
    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), false);

    // go back 3 positions
    for (int i = 4; i >= 2; --i) {
        QCOMPARE(vs->currentLoc(), i);
        QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(i, 0));
        vs->goBack();
    }
    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), true);

    QCOMPARE(vs->locationHistoryBuffer().size(), 5);
    QCOMPARE(vs->currentLoc(), 1);

    // jump to new position
    // should clear forward history
    vm->addPositionToHistory(currentFileUrl, {50, 0});

    QCOMPARE(vs->locationHistoryBuffer().size(), 3);
    QCOMPARE(vs->currentLoc(), 2);

    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), false);
}

void LocationHistoryTest::test_addOnlyIfViewLineCountAwayFromCurrentPos()
{
    auto vs = viewSpace();
    auto vm = viewManager();
    Q_ASSERT(vs && vm);

    const auto currentFileUrl = QUrl::fromLocalFile(QString::fromLatin1(__FILE__));

    vm->addPositionToHistory(currentFileUrl, {0, 0});
    QCOMPARE(vs->locationHistoryBuffer().size(), 1);

    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), false);

    // Changing cursor position will only be added to location
    // history if view line count away
    // ensure the view has some non-null size
    auto view = vm->activeView();
    view->resize(600, 600);
    int vlc = viewLineCount(view);
    view->setCursorPosition({2 * vlc, 0});
    QCOMPARE(vs->locationHistoryBuffer().size(), 2);

    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), false);

    // changing cursor position again but not far enough, should lead
    // to no change
    view->setCursorPosition({view->cursorPosition().line() + 1, 0});
    QCOMPARE(vs->locationHistoryBuffer().size(), 2);

    // go back one position i.e., to {0, 0}
    vs->goBack();

    QCOMPARE(vs->isHistoryBackEnabled(), false);
    QCOMPARE(vs->isHistoryForwardEnabled(), true);

    // changing cursor position now should only add a location if cursor is viewLineCount away
    // from "current" position, which is {0, 0}, and not from the last location in buffer.
    view->setCursorPosition({vlc + 5, 0});

    QCOMPARE(vs->locationHistoryBuffer().size(), 2);
    QCOMPARE(vs->locationHistoryBuffer().at(vs->currentLoc()).cursor, KTextEditor::Cursor(vlc + 5, 0));

    QCOMPARE(vs->isHistoryBackEnabled(), true);
    QCOMPARE(vs->isHistoryForwardEnabled(), false);
}

void LocationHistoryTest::test_signalEmission()
{
    auto vs = viewSpace();
    auto vm = viewManager();
    Q_ASSERT(vs && vm);

    const auto currentFileUrl = QUrl::fromLocalFile(QString::fromLatin1(__FILE__));

    QSignalSpy sigSpy(vm, &KateViewManager::historyBackEnabled);

    vm->addPositionToHistory(currentFileUrl, {0, 0});
    QCOMPARE(vs->locationHistoryBuffer().size(), 1);

    // only one position, no signals for now
    QCOMPARE(sigSpy.count(), 0);

    vm->addPositionToHistory(currentFileUrl, {1, 0});
    QCOMPARE(sigSpy.count(), 1);
    auto args = sigSpy.takeFirst();
    QCOMPARE(args.at(0).toBool(), true);

    QSignalSpy spy2(vm, &KateViewManager::historyForwardEnabled);

    vs->goBack();

    // 2 signals (back disabled, forward enabled)

    QCOMPARE(spy2.count(), 1);
    {
        auto args = spy2.takeFirst();
        QCOMPARE(args.at(0).toBool(), true);
    }
    QCOMPARE(sigSpy.count(), 1);
    {
        auto args = sigSpy.takeFirst();
        QCOMPARE(args.at(0).toBool(), false);
    }

    vs->goForward();
    // 2 signals (back enabled, forward disabled)
    QCOMPARE(spy2.count(), 1);
    {
        auto args = spy2.takeFirst();
        QCOMPARE(args.at(0).toBool(), false);
    }
    QCOMPARE(sigSpy.count(), 1);
    {
        auto args = sigSpy.takeFirst();
        QCOMPARE(args.at(0).toBool(), true);
    }
}
