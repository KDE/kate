/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef DAP_BUS_SELECTOR_H
#define DAP_BUS_SELECTOR_H

class QJsonObject;

namespace dap
{
class Bus;

namespace settings
{
struct BusSettings;
}

Bus *createBus(const settings::BusSettings &configuration);
}

#endif
