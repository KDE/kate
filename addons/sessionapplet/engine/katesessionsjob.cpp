/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Joseph Wenninger <jowenn@kde.org>
based on clipboard engine:
Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>
partly based on code:
Copyright (C) 2008 by Montel Laurent <montel@kde.org>    

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
#include "katesessionsjob.h"
#include "katesessionsengine.h"
#include <QDebug>
#include <KToolInvocation>
#include <QUrl>

static const QString s_clipboardSourceName = QStringLiteral("katesessions");

KateSessionsJob::KateSessionsJob(KateSessionsEngine *engine, const QString &destination, const QString &operation, const QVariantMap &parameters, QObject *parent)
    : Plasma::ServiceJob(destination, operation, parameters, parent)
    , m_engine(engine)
{
}

void KateSessionsJob::start()
{
    qDebug()<<"Job started:"<<operationName()<<" "<<destination();
    const QString operation = operationName();
    // first check for operations not needing an item
   if (operation == QLatin1String("newSession")) {
     QString sessionName=parameters().value(QLatin1String("sessionName")).toString();
     if (sessionName.isEmpty()) {
         setResult(false);
         emitResult();
         return;
     }
     //CHECK IF SESSION EXISTS
     QStringList args;
     args <<QLatin1String("-n")<<QLatin1String("--start")<< sessionName;
     KToolInvocation::kdeinitExec(QLatin1String("kate"), args);
     setResult(true);
     emitResult();
     return;
   } else if (operation == QLatin1String("invoke")) {
        QString dest=destination();
        QStringList args;
        if (dest==QLatin1String("_kate_noargs")) {
            //do nothing
        } else if (dest==QLatin1String("_kate_anon_newsession")) {
            args << QLatin1String("--startanon");
        } else if (dest==QLatin1String("_kate_newsession")) {
            args << QLatin1String("--startanon");
            qDebug()<<"This should not be reached";
        } else {
            dest.chop(12); // .katesession
            args <<QLatin1String("-n")<<QLatin1String("--start")<<QUrl::fromPercentEncoding(dest.toLatin1());
            //args <<"-n"<< "--start"<<m_sessions[ id-3 ];
        }
        
        KToolInvocation::kdeinitExec(QLatin1String("kate"), args);
        setResult(true);
        emitResult();
        return;
    } else  if (operation == QLatin1String("remove")) {
        qDebug()<<operation<<destination();
        setResult(true);
        emitResult();
        return;
    }
    setResult(false);
    emitResult();
}
