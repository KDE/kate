// Description: Advanced settings dialog for gdb
//
//
// Copyright (c) 2012 Kåre Särs <kare.sars@iki.fi>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#ifndef ADVANCED_SETTINGS_H
#define ADVANCED_SETTINGS_H

#include "ui_advanced_settings.h"
#include <QStringList>
#include <QDialog>

class AdvancedGDBSettings : public QDialog, public Ui::AdvancedGDBSettings
{
Q_OBJECT
public:
    enum CustomStringOrder {
        GDBIndex = 0,
        LocalRemoteIndex,
        RemoteBaudIndex,
        SoAbsoluteIndex,
        SoRelativeIndex,
        SrcPathsIndex,
        CustomStartIndex
    };

    AdvancedGDBSettings(QWidget *parent = nullptr);
    ~AdvancedGDBSettings() override;

    const QStringList configs() const;

    void setConfigs(const QStringList &cfgs);

private:
    void setComboText(QComboBox *combo, const QString &str);

private Q_SLOTS:
    void slotBrowseGDB();

    void slotSetSoPrefix();

    void slotAddSoPath();
    void slotDelSoPath();

    void slotAddSrcPath();
    void slotDelSrcPath();

    void slotLocalRemoteChanged();
};

#endif
