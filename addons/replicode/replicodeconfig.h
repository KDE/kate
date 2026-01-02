/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QTabWidget>

class Ui_tabWidget;
class ReplicodeSettings;

class ReplicodeConfig : public QTabWidget
{
public:
    explicit ReplicodeConfig(QWidget *parent = nullptr);
    ~ReplicodeConfig() override;

public Q_SLOTS:
    void reset();
    void save();
    void load();

    ReplicodeSettings *settingsObject()
    {
        save();
        return m_settings;
    }

private:
    Ui_tabWidget *m_ui;
    ReplicodeSettings *m_settings;
};
