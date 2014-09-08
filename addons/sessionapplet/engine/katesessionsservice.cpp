/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Joseph Wenninger <jowenn@kde.org>
based on clipboard engine:
Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "katesessionsengine.h"
#include "katesessionsjob.h"
#include "katesessionsservice.h"
#include <QDebug>

KateSessionsService::KateSessionsService(KateSessionsEngine *engine, const QString &uuid)
    : Plasma::Service()
    , m_engine(engine)
    , m_uuid(uuid)
{
    setName(QStringLiteral("org.kde.plasma.katesessions"));
}

Plasma::ServiceJob *KateSessionsService::createJob(const QString &operation, QVariantMap &parameters)
{
    qDebug()<<"creating KateSessionsJob";
    return new KateSessionsJob(m_engine, m_uuid, operation, parameters, this);
}
