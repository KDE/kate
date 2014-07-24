/* This file is part of the KDE libraries
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

#ifndef __KATE_WAITER_H__
#define __KATE_WAITER_H__

#include <QCoreApplication>
#include <QObject>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

class KateWaiter : public QObject
{
    Q_OBJECT

private:
    QCoreApplication *m_app;
    QString m_service;
    QStringList m_tokens;
public:
    KateWaiter(QCoreApplication *app, const QString &service, const QStringList &tokens)
        : QObject(app), m_app(app), m_service(service), m_tokens(tokens) {
        connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString, QString, QString))
                , this, SLOT(serviceOwnerChanged(QString, QString, QString)));
    }

public Q_SLOTS:
    void exiting() {
        m_app->quit();
    }

    void documentClosed(const QString &token) {
        m_tokens.removeAll(token);
        if (m_tokens.count() == 0) {
            m_app->quit();
        }
    }

    void serviceOwnerChanged(const QString &name, const QString &, const QString &) {
        if (name != m_service) {
            return;
        }

        m_app->quit();
    }
};

#endif
