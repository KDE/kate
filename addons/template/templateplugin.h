// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "template.h"

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIClient>

class TemplatePlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit TemplatePlugin(QObject *parent);

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class TemplatePluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit TemplatePluginView(TemplatePlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~TemplatePluginView();

private:
    void crateNewFromTemplate();

    void templateCrated(const QString &fileToOpen);

    KTextEditor::MainWindow *m_mainWindow = nullptr;
    Template *m_template = nullptr;
};
