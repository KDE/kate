/**
 * Description: Common interface for GDB and DAP backends
 *
 *  SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "backendinterface.h"

BackendInterface::BackendInterface(QObject *parent)
    : QObject(parent)
{
}

#include "moc_backendinterface.cpp"
