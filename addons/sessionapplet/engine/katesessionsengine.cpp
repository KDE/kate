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
#include "katesessionsmodel.h"
#include "katesessionsservice.h"
#include <QDebug>

static const QString s_clipboardSourceName = QStringLiteral("katesessions");

KateSessionsEngine::KateSessionsEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    qDebug()<<"JOWENN";
    setData(s_clipboardSourceName,QStringLiteral("test_data"),QLatin1String("This is just for testing"));  
    setModel(s_clipboardSourceName,new KateSessionsModel(this));
}

KateSessionsEngine::~KateSessionsEngine()
{
    
}

Plasma::Service *KateSessionsEngine::serviceForSource(const QString &source)
{
    qDebug()<< "Creating KateSessionService";
    Plasma::Service *service = new KateSessionsService(this, source);
    service->setParent(this);
    return service;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(org.kde.plasma.katesessions, KateSessionsEngine, "plasma-dataengine-katesessions.json")

#include "katesessionsengine.moc"
