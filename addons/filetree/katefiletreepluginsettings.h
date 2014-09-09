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

#ifndef KATE_FILETREE_PLUGIN_SETTINGS_H
#define KATE_FILETREE_PLUGIN_SETTINGS_H

#include <KConfigGroup>
#include <QColor>

// Geh, should make this into a proper class, setters and getters.
class KateFileTreePluginSettings
{
public:
    KateFileTreePluginSettings();

    void save();

    bool shadingEnabled() const;
    void setShadingEnabled(bool);

    const QColor &viewShade() const;
    void setViewShade(const QColor &);

    const QColor &editShade() const;
    void setEditShade(const QColor &);

    bool listMode() const;
    void setListMode(bool);

    int sortRole() const;
    void setSortRole(int);

    bool showFullPathOnRoots() const;
    void setShowFullPathOnRoots(bool);

private:
    KConfigGroup m_group;

    bool m_shadingEnabled;
    QColor m_viewShade;
    QColor m_editShade;

    bool m_listMode;
    int m_sortRole;

    bool m_showFullPathOnRoots;
};

#endif //KATE_FILETREE_PLUGIN_H

