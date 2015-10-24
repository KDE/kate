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
#include <QStandardPaths>

#include <KSharedConfig>
#include <KConfigGroup>

/**
 * If we have some local breeze icon resource, prefer it
 */
static void setupIconTheme()
{
    /**
     * let QStandardPaths handle this, it will look for app local stuff
     * this means e.g. for mac: "<APPDIR>/../Resources" and for win: "<APPDIR>/data"
     */
    const QString breezeIcons = QStandardPaths::locate(QStandardPaths::DataLocation, QStringLiteral("breeze.rcc"));
    if (QFile::exists(breezeIcons) && QResource::registerResource(breezeIcons)) {
        // tell qt about the theme
        QIcon::setThemeSearchPaths(QStringList() << QStringLiteral(":/icons"));
        QIcon::setThemeName(QStringLiteral("breeze"));

        // tell KIconLoader an co. about the theme
        KConfigGroup cg(KSharedConfig::openConfig(), "Icons");
        cg.writeEntry("Theme", "breeze");
        cg.sync();
    }
}
