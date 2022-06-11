/**
 * Description: Common interface for GDB and DAP clients
 *
 *  SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "debugview_iface.h"

DebugViewInterface::DebugViewInterface(QObject *parent)
    : QObject(parent)
{
}
