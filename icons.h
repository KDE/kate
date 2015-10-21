/* This file is part of the KDE project
   Copyright (C) 2015 Christoph Cullmann <cullmann@kde.org>

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

#pragma once

#include <QIcon>
#include <QResource>

/**
 * If we have some local breeze icon resource, prefer it
 */
static void setupIconTheme()
{
    /**
     * Search in local "share" and "Resources"
     */
    QStringList localPathSuffixes;
    localPathSuffixes << QStringLiteral("/../share/breeze.rcc") << QStringLiteral("/../Resources/breeze.rcc");
    Q_FOREACH (const QString &localPathSuffix, localPathSuffixes) {
        const QString localIconsResource = QCoreApplication::applicationDirPath() + localPathSuffix;
        if (QFile::exists(localIconsResource) && QResource::registerResource(localIconsResource)) {
            QIcon::setThemeName(QStringLiteral("breeze"));
            break;
        }
    }
}
