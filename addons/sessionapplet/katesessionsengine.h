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
#ifndef KATESESSIONS_ENGINE_H
#define KATESESSIONS_ENGINE_H

#include <Plasma/DataEngine>


class KateSessionsEngine : public Plasma::DataEngine
{
    Q_OBJECT
public:
    KateSessionsEngine(QObject *parent, const QVariantList &args);
    ~KateSessionsEngine() override;

    Plasma::Service *serviceForSource (const QString &source) override;

};

#endif
