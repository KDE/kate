/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewspace.h"

#include <QObject>
#include <QTemporaryDir>

#include <memory>

class KateApp;
class KateViewSpace;
class KateViewManager;

inline void clearAllDocs(KateMainWindow *mw)
{
    auto vm = mw->viewManager();
    vm->slotCloseOtherViews();
    auto vs = vm->activeViewSpace();
    // close everything
    for (int i = 0; i < vs->numberOfRegisteredDocuments(); ++i) {
        vm->slotDocumentClose();
    }
}

class KateViewManagementTests : public QObject
{
    Q_OBJECT

public:
    KateViewManagementTests(QObject *parent = nullptr);

private Q_SLOTS:
    void init();
    void testSingleViewspaceDoesntCloseWhenLastViewClosed();
    void testViewspaceClosesWhenLastViewClosed();
    void testViewspaceClosesWhenThereIsWidget();
    void testMoveViewBetweenViewspaces();
    void testTwoMainWindowsCloseInitialDocument1();
    void testTwoMainWindowsCloseInitialDocument2();
    void testTwoMainWindowsCloseInitialDocument3();
    void testTabLRUWithWidgets();
    void testViewChangedEmittedOnAddWidget();
    void testWidgetAddedEmittedOnAddWidget();
    void testWidgetRemovedEmittedOnRemoveWidget();
    void testActivateNotAddedWidget();
    void testBug460613();
    void testWindowsClosesDocuments();
    void testTabBarHidesShows();
    void testNewWindowHasSameGlobalOptions();
    void testBug465811();
    void testBug465807();
    void testBug465808();
    void testNewViewCreatedIfViewNotinViewspace();
    void testNewSessionClearsWindowWidgets();
    void testViewspaceWithWidgetDoesntCrashOnClose();
    void testDetachDoc();
    void testKwriteInSDIModeWithOpenMultipleUrls();
    void testTabbarContextMenu();

private:
    std::unique_ptr<QTemporaryDir> m_tempdir;
    std::unique_ptr<KateApp> app;
};
