/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "bus.h"

namespace dap
{
Bus::Bus(QObject *parent)
    : QObject(parent)
    , m_state(State::None)
{
}

Bus::State Bus::state() const
{
    return m_state;
}

void Bus::setState(State state)
{
    if (state == m_state)
        return;
    m_state = state;

    Q_EMIT stateChanged(state);

    switch (state) {
    case State::Running:
        Q_EMIT running();
        break;
    case State::Closed:
        Q_EMIT closed();
        break;
    default:;
    }
}

}
