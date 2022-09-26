/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "bus.h"
#include "processbus.h"
#include "socketbus.h"
#include "socketprocessbus.h"

#include "settings.h"

#include "bus_selector.h"

namespace dap
{
Bus *createBus(const settings::BusSettings &configuration)
{
    const bool has_command = configuration.hasCommand();
    const bool has_connection = configuration.hasConnection();

    if (has_command && has_connection) {
        return new SocketProcessBus();
    }

    if (has_command) {
        return new ProcessBus();
    }

    if (has_connection) {
        return new SocketBus();
    }

    return nullptr;
}

}
