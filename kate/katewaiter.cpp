/*  SPDX-License-Identifier: LGPL-2.0-or-later

    Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
    Copyright (C) 2002, 2003 Joseph Wenninger <jowenn@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "katewaiter.h"

KateWaiter::KateWaiter(const QString &service, const QStringList &tokens)
    : QObject(QCoreApplication::instance())
    , m_tokens(tokens)
    , m_watcher(service, QDBusConnection::sessionBus())
{
    connect(&m_watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &KateWaiter::serviceOwnerChanged);
}

void KateWaiter::exiting()
{
    QCoreApplication::instance()->quit();
}

void KateWaiter::documentClosed(const QString &token)
{
    m_tokens.removeAll(token);
    if (m_tokens.count() == 0) {
        QCoreApplication::instance()->quit();
    }
}

void KateWaiter::serviceOwnerChanged(const QString &, const QString &, const QString &)
{
    QCoreApplication::instance()->quit();
}
