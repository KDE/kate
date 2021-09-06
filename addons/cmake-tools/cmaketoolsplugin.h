/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KATE_CMAKE_TOOLS_PLUGIN_H
#define KATE_CMAKE_TOOLS_PLUGIN_H

#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QVariant>

#include "cmakecompletion.h"

/**
 * Plugin
 */
class CMakeToolsPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit CMakeToolsPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    ~CMakeToolsPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

/**
 * Plugin view
 */
class CMakeToolsPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    CMakeToolsPluginView(CMakeToolsPlugin *plugin, KTextEditor::MainWindow *mainwindow);

    ~CMakeToolsPluginView() override;

private Q_SLOTS:
    void onViewCreated(KTextEditor::View *v);

private:
    KTextEditor::MainWindow *m_mainWindow;
    CMakeCompletion m_completion;
};

#endif
