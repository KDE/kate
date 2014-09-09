/* This file is part of the KDE project
   Copyright (C) 2010 Thomas Fjellstrom <thomas@fjellstrom.ca>

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

#include "katefiletreepluginsettings.h"
#include <KColorScheme>
#include <KColorUtils>
#include "katefiletreedebug.h"

KateFileTreePluginSettings::KateFileTreePluginSettings()
    : m_group(KSharedConfig::openConfig(), "filetree")
{
    KColorScheme colors(QPalette::Active);
    QColor bg = colors.background().color();
    QColor viewShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::VisitedText).color(), 0.5);
    QColor editShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::ActiveText).color(), 0.5);

    m_shadingEnabled = m_group.readEntry("shadingEnabled", true);
    m_viewShade = m_group.readEntry("viewShade", viewShade);
    m_editShade = m_group.readEntry("editShade", editShade);

    m_listMode = m_group.readEntry("listMode", false);
    m_sortRole = m_group.readEntry("sortRole", int(Qt::DisplayRole));

    m_showFullPathOnRoots = m_group.readEntry("showFullPathOnRoots", false);
}

void KateFileTreePluginSettings::save()
{
    m_group.writeEntry("shadingEnabled", m_shadingEnabled);
    m_group.writeEntry("viewShade", m_viewShade);
    m_group.writeEntry("editShade", m_editShade);
    m_group.writeEntry("listMode", m_listMode);
    m_group.writeEntry("sortRole", m_sortRole);
    m_group.writeEntry("showFullPathOnRoots", m_showFullPathOnRoots);

    m_group.sync();
}

bool KateFileTreePluginSettings::shadingEnabled() const
{
    return m_shadingEnabled;
}

void KateFileTreePluginSettings::setShadingEnabled(bool shadingEnabled)
{
    m_shadingEnabled = shadingEnabled;
}

const QColor &KateFileTreePluginSettings::viewShade() const
{
    return m_viewShade;
}

void KateFileTreePluginSettings::setViewShade(const QColor &viewShade)
{
    m_viewShade = viewShade;
}

const QColor &KateFileTreePluginSettings::editShade() const
{
    return m_editShade;
}

void KateFileTreePluginSettings::setEditShade(const QColor &editShade)
{
    m_editShade = editShade;
}

bool KateFileTreePluginSettings::listMode() const
{
    return m_listMode;
}

void KateFileTreePluginSettings::setListMode(bool listMode)
{
    m_listMode = listMode;
}

int KateFileTreePluginSettings::sortRole() const
{
    return m_sortRole;
}

void KateFileTreePluginSettings::setSortRole(int sortRole)
{
    m_sortRole = sortRole;
}

bool KateFileTreePluginSettings::showFullPathOnRoots() const
{
    return m_showFullPathOnRoots;
}

void KateFileTreePluginSettings::setShowFullPathOnRoots(bool s)
{
    m_showFullPathOnRoots = s;
}

