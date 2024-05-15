/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QObject>

class FormatApplyTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testFormatApply();
    void testFormatterForName();
};
