/*
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

KateRunningInstanceInfo::KateRunningInstanceInfo(const QString &serviceName_)
    : serviceName(serviceName_)
    , dbus_if(new QDBusInterface(serviceName_,
                                 QStringLiteral("/MainApplication"),
                                 QString(), // I don't know why it does not work if I specify org.kde.Kate.Application here
                                 QDBusConnection::sessionBus()))
{
    // get the last used timestamp, if that is not valid, we can ignore the rest
    const QVariant a_l = dbus_if->property("lastActivationChange");
    if (!a_l.isValid()) {
        return;
    }
    lastActivationChange = a_l.toLongLong();

    // get the active session
    const QVariant a_s = dbus_if->property("activeSession");
    if (a_s.isValid()) {
        sessionName = a_s.toString();
    }
}

std::vector<KateRunningInstanceInfo> fillinRunningKateAppInstances()
{
    // if we have no dbus, nothing to do
    std::vector<KateRunningInstanceInfo> instances;
    QDBusConnectionInterface *i = QDBusConnection::sessionBus().interface();
    if (!i) {
        return instances;
    }

    // get all instances
    const QDBusReply<QStringList> servicesReply = i->registeredServiceNames();
    if (!servicesReply.isValid()) {
        return instances;
    }

    // try to filter out the current instance
    const bool inSandbox = QFileInfo::exists(QStringLiteral("/flatpak-info"));
    const QString my_pid = inSandbox ? QDBusConnection::sessionBus().baseService().replace(QRegularExpression(QStringLiteral("[\\.:]")), QStringLiteral("_"))
                                     : QString::number(QCoreApplication::applicationPid());
    const auto services = servicesReply.value();
    for (const QString &s : services) {
        if (s.startsWith(QLatin1String("org.kde.kate")) && !s.endsWith(my_pid)) {
            // ignore instancer we not even can query the lastActivationChange
            KateRunningInstanceInfo instance(s);
            if (instance.lastActivationChange != 0) {
                instances.push_back(std::move(instance));
            }
        }
    }
    return instances;
}
