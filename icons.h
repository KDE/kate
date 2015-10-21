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

/**
 * Setup icons, if no icons found globally, fallback to local bundled icons
 */
static void setupIconTheme()
{
    /**
     * magic icon path search: if we have no global icons, search for local bundled ones!
     */
    if (QIcon::fromTheme(QStringLiteral("document-new")).isNull()) {
        /**
         * fallback theme is breeze ATM
         */
        QIcon::setThemeName(QStringLiteral("breeze"));

        /**
         * probe multiple local paths relative to application binary
         */
        QStringList localPathSuffixes;
        localPathSuffixes << QStringLiteral("/../icons") << QStringLiteral("/../share/icons") << QStringLiteral("/../Resources/icons");
        Q_FOREACH (const QString &localPathSuffix, localPathSuffixes) {
            /**
             * try new path, break if icons found
             */
            QIcon::setThemeSearchPaths(QStringList() << QCoreApplication::applicationDirPath() + localPathSuffix);

            /**
             * icons there?
             */
            if (!QIcon::fromTheme(QStringLiteral("document-new")).isNull()) {
                break;
            }
        }
    }
}
