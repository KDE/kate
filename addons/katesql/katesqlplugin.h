/*
   SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <ktexteditor/application.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/plugin.h>
#include <ktexteditor/view.h>

#include <KPluginFactory>

class KateSQLPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateSQLPlugin(QObject *parent);

    ~KateSQLPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override
    {
        return 1;
    };
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;
    QString configPageName(int number = 0) const;
    QString configPageFullName(int number = 0) const;
    QIcon configPageIcon(int number = 0) const;

Q_SIGNALS:
    void globalSettingsChanged();
};
