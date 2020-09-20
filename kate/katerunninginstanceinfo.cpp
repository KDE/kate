/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2009 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katerunninginstanceinfo.h"
#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

int KateRunningInstanceInfo::dummy_session = 0;

bool fillinRunningKateAppInstances(KateRunningInstanceMap *map)
{
    QDBusConnectionInterface *i = QDBusConnection::sessionBus().interface();
    if (!i) {
        return true; // we do not know about any others...
    }

    // look up all running kate instances and there sessions
    QDBusReply<QStringList> servicesReply = i->registeredServiceNames();
    QStringList services;
    if (servicesReply.isValid()) {
        services = servicesReply.value();
    }

    const bool inSandbox = QFileInfo::exists(QStringLiteral("/flatpak-info"));
    const QString my_pid = inSandbox ? QDBusConnection::sessionBus().baseService().replace(QRegularExpression(QStringLiteral("[\\.:]")), QStringLiteral("_")) : QString::number(QCoreApplication::applicationPid());

    for (const QString &s : qAsConst(services)) {
        if (s.startsWith(QLatin1String("org.kde.kate")) && !s.endsWith(my_pid)) {
            KateRunningInstanceInfo *rii = new KateRunningInstanceInfo(s);
            if (rii->valid) {
                if (map->contains(rii->sessionName)) {
                    return false; // ERROR no two instances may have the same session name
                }
                map->insert(rii->sessionName, rii);
                // std::cerr<<qPrintable(s)<<"running instance:"<< rii->sessionName.toUtf8().data()<<std::endl;
            } else {
                delete rii;
            }
        }
    }
    return true;
}

void cleanupRunningKateAppInstanceMap(KateRunningInstanceMap *map)
{
    for (KateRunningInstanceMap::const_iterator it = map->constBegin(); it != map->constEnd(); ++it) {
        delete it.value();
    }
    map->clear();
}
