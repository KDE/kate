/*************************************************************************************
 * This file is part of KDevPlatform                                                 *
 * SPDX-FileCopyrightText: 2016 Zhigalin Alexander <alexander@zhigalin.tk>                         *
 *                                                                                   *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *************************************************************************************/

#ifndef COLORSCHEMECHOOSER_H
#define COLORSCHEMECHOOSER_H

#include <QAction>
#include <QApplication>
#include <QObject>
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
    KateColorSchemeChooser(QObject *parent);

    QString currentSchemeName() const;
private Q_SLOTS:
    static void slotSchemeChanged(QAction *triggeredAction);

private:
    static QString loadCurrentScheme();
    static void saveCurrentScheme(const QString &name);
    static QString currentDesktopDefaultScheme();
};

#endif // COLORSCHEMECHOOSER_H
