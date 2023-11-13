/*
 *  SPDX-FileCopyrightText: 2023 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QObject>

class TestGdbBackend : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parseBreakpoint();
    void parseBreakpoint_data();
};
