/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

class CEPlugin : public KTextEditor::Plugin
{
public:
    explicit CEPlugin(QObject *parent);

    ~CEPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

/**
 * Plugin view to merge the actions into the UI
 */
class CEPluginView : public QObject, public KXMLGUIClient
{
public:
    /**
     * Construct plugin view
     * @param plugin our plugin
     * @param mainwindows the mainwindow for this view
     */
    explicit CEPluginView(CEPlugin *plugin, KTextEditor::MainWindow *mainwindow);

    /**
     * Our Destructor
     */
    ~CEPluginView() override;

private:
    void openANewTab();

private:
    /**
     * the main window we belong to
     */
    KTextEditor::MainWindow *m_mainWindow;
    class CEWidget *m_mainWidget = nullptr;
};
