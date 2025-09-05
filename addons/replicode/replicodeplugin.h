/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "replicodeview.h"
#include <KTextEditor/ConfigPage>
#include <KTextEditor/Plugin>

class ReplicodePlugin : public KTextEditor::Plugin
{
public:
    explicit ReplicodePlugin(QObject *parent);
    ~ReplicodePlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override
    {
        return new ReplicodeView(this, mainWindow);
    }

    // Config interface
    int configPages() const override
    {
        return 1;
    }
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;
};
