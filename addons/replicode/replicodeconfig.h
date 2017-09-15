/* This file is part of the KDE project
   Copyright (C) 2014 Martin Sandsmark <martin.sandsmark@kde.org>

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
#ifndef REPLICODECONFIG_H
#define REPLICODECONFIG_H

#include <QVariant>
#include <QTabWidget>

class Ui_tabWidget;
class ReplicodeSettings;

class ReplicodeConfig : public QTabWidget
{
    Q_OBJECT
public:
    explicit ReplicodeConfig(QWidget *parent = nullptr);
    ~ReplicodeConfig() override;

public Q_SLOTS:
    void reset();
    void save();
    void load();

    ReplicodeSettings *settingsObject() { save(); return m_settings; }

private:
    Ui_tabWidget *m_ui;
    ReplicodeSettings *m_settings;
};

#endif//REPLICODECONFIG_H
