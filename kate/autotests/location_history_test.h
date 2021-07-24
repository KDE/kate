/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include <QObject>

class KateApp;
class KateViewSpace;
class KateViewManager;

class LocationHistoryTest : public QObject
{
    Q_OBJECT

public:
    LocationHistoryTest(QObject *parent = nullptr);
    ~LocationHistoryTest();

    KateViewSpace *viewSpace();
    KateViewManager *viewManager();

private Q_SLOTS:
    void init();
    void cleanup();

    void test_addLocationInvalidUrl();
    void test_addLocation();
    void test_addMaxLocations();
    void test_goBackForward();
    void test_goBackForward2();
    void test_addOnlyIfViewLineCountAwayFromCurrentPos();
    void test_signalEmission();

private:
    KateApp *app;
};
