/*************************************************************************************
 * This file is part of KDevPlatform                                                 *
 * SPDX-FileCopyrightText: 2016 Zhigalin Alexander <alexander@zhigalin.tk>                         *
 *                                                                                   *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 *************************************************************************************/

#include "katecolorschemechooser.h"

#include <QActionGroup>
#include <QDirIterator>
#include <QFileInfo>
#include <QMenu>
#include <QModelIndex>
#include <QStandardPaths>
#include <QStringList>
#include <QtGlobal>

#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <kconfigwidgets_version.h>

#include "katedebug.h"
#include "katemainwindow.h"

KateColorSchemeChooser::KateColorSchemeChooser(QObject *parent)
    : QAction(parent)
{
    auto manager = new KColorSchemeManager(parent);

    const auto scheme(currentSchemeName());
    qCDebug(LOG_KATE) << "Color scheme : " << scheme;

    auto selectionMenu = manager->createSchemeSelectionMenu(scheme, this);

    connect(selectionMenu->menu(), &QMenu::triggered, this, &KateColorSchemeChooser::slotSchemeChanged);

    manager->activateScheme(manager->indexForScheme(scheme));

    setMenu(selectionMenu->menu());
    menu()->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-color")));
    menu()->setTitle(i18n("&Color Scheme"));
}

QString KateColorSchemeChooser::loadCurrentScheme() const
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup cg(config, "UiSettings");
    return cg.readEntry("ColorScheme", currentDesktopDefaultScheme());
}

void KateColorSchemeChooser::saveCurrentScheme(const QString &name)
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup cg(config, "UiSettings");
    cg.writeEntry("ColorScheme", name);
    cg.sync();
}

QString KateColorSchemeChooser::currentDesktopDefaultScheme() const
{
#if KCONFIGWIDGETS_VERSION < QT_VERSION_CHECK(5, 67, 0)
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup group(config, "General");
    const QString scheme = group.readEntry("ColorScheme", QStringLiteral("Breeze"));
    const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(scheme));
    KSharedConfigPtr schemeFile = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
    const QString name = KConfigGroup(schemeFile, "General").readEntry("Name", scheme);
    return name;
#else
    return QString();
#endif
}

QString KateColorSchemeChooser::currentSchemeName() const
{
    if (!menu()) {
        return loadCurrentScheme();
    }

    QAction *const action = menu()->activeAction();

    if (action) {
        return KLocalizedString::removeAcceleratorMarker(action->text());
    }
    return currentDesktopDefaultScheme();
}

void KateColorSchemeChooser::slotSchemeChanged(QAction *triggeredAction)
{
    saveCurrentScheme(KLocalizedString::removeAcceleratorMarker(triggeredAction->text()));
}
