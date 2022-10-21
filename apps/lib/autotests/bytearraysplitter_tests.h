/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QObject>

class KateApp;
class KateViewSpace;
class KateViewManager;

class ByteArraySplitterTests : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

private Q_SLOTS:
    void test_data();
    void test();
};
