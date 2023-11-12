// SPDX-FileCopyrightText: 2023 Rémi Peuchot <kde.remi@proton.me>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QObject>

class TestGdbVariableParser : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parseVariable();
    void parseVariable_data();
    void fuzzyTest();
};
