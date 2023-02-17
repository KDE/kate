/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateapp.h"

#include <QObject>
#include <QTemporaryDir>

#include <memory>

class KateApp;
class KateViewSpace;
class KateViewManager;

class KateViewManagementTests : public QObject
{
    Q_OBJECT

public:
    KateViewManagementTests(QObject *parent = nullptr);

private Q_SLOTS:
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

private:
    class QTemporaryDir *m_tempdir;
    std::unique_ptr<KateApp> app;
};
