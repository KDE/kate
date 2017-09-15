/* This file is part of the KDE project

   Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>

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

#ifndef _KATE_RUNNING_INSTANCE_INFO_
#define _KATE_RUNNING_INSTANCE_INFO_

#include <QMap>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QVariant>
#include <iostream>

class KateRunningInstanceInfo: public QObject
{
    Q_OBJECT

public:
    KateRunningInstanceInfo(const QString &serviceName_):
        QObject(), valid(false),
        serviceName(serviceName_),
        dbus_if(new QDBusInterface(serviceName_, QStringLiteral("/MainApplication"),
                                   QString(), //I don't know why it does not work if I specify org.kde.Kate.Application here
                                   QDBusConnection::sessionBus(), this)) {
        if (!dbus_if->isValid()) {
            std::cerr << qPrintable(QDBusConnection::sessionBus().lastError().message()) << std::endl;
        }
        QVariant a_s = dbus_if->property("activeSession");
        /*      std::cerr<<a_s.isValid()<<std::endl;
              std::cerr<<"property:"<<qPrintable(a_s.toString())<<std::endl;
              std::cerr<<qPrintable(QDBusConnection::sessionBus().lastError().message())<<std::endl;*/
        if (!a_s.isValid()) {
            sessionName = QString::fromLatin1("___NO_SESSION_OPENED__%1").arg(dummy_session++);
            valid = false;
        } else {
            if (a_s.toString().isEmpty()) {
                sessionName = QString::fromLatin1("___DEFAULT_CONSTRUCTED_SESSION__%1").arg(dummy_session++);
            } else {
                sessionName = a_s.toString();
            }
            valid = true;
        }
    }
    ~KateRunningInstanceInfo() override {
        delete dbus_if;
    }
    bool valid;
    const QString serviceName;
    QDBusInterface *dbus_if;
    QString sessionName;

private:
    static int dummy_session;
};

typedef QMap<QString, KateRunningInstanceInfo *> KateRunningInstanceMap;

Q_DECL_EXPORT bool fillinRunningKateAppInstances(KateRunningInstanceMap *map);
Q_DECL_EXPORT void cleanupRunningKateAppInstanceMap(KateRunningInstanceMap *map);

#endif

