/*************************************************************************************
 * This file is part of KDevPlatform                                                 *
 * Copyright 2016 Zhigalin Alexander <alexander@zhigalin.tk>                         *
 *                                                                                   *
 * This library is free software; you can redistribute it and/or                     *
 * modify it under the terms of the GNU Lesser General Public                        *
 * License as published by the Free Software Foundation; either                      *
 * version 2.1 of the License, or (at your option) version 3, or any                 *
 * later version accepted by the membership of KDE e.V. (or its                      *
 * successor approved by the membership of KDE e.V.), which shall                    *
 * act as a proxy defined in Section 6 of version 3 of the license.                  *
 *                                                                                   *
 * This library is distributed in the hope that it will be useful,                   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                 *
 * Lesser General Public License for more details.                                   *
 *                                                                                   *
 * You should have received a copy of the GNU Lesser General Public                  *
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.     *
 *************************************************************************************/

#ifndef COLORSCHEMECHOOSER_H
#define COLORSCHEMECHOOSER_H

#include <QObject>
#include <QAction>
#include <QApplication>
#include <QString>

#include <KColorSchemeManager>

class KActionCollection;

/**
* Provides a menu that will offer to change the color scheme
*
* Furthermore, it will save the selection in the user configuration.
*/
class KateColorSchemeChooser : public QAction
{
    public:
        KateColorSchemeChooser(QObject* parent);

        QString currentSchemeName() const;
    private Q_SLOTS:
        void slotSchemeChanged(QAction* triggeredAction);

    private:
        QString loadCurrentScheme() const;
        void    saveCurrentScheme(const QString &name);
        QString currentDesktopDefaultScheme() const;
};

#endif // COLORSCHEMECHOOSER_H
