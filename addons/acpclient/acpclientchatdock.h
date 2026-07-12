/*
    SPDX-FileCopyrightText: 2026

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QDockWidget>

class ACPClientPlugin;
class ACPClientChatWidget;
namespace KTextEditor
{
class MainWindow;
}

class ACPClientChatDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit ACPClientChatDock(ACPClientPlugin *plugin, KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);
    ~ACPClientChatDock() override;

    // Get the chat widget
    ACPClientChatWidget *chatWidget() const;

    // Show or hide the dock
    void showChat(bool show = true);

Q_SIGNALS:
    void visibilityChanged(bool visible);

private:
    ACPClientPlugin *m_plugin;
    ACPClientChatWidget *m_chatWidget;
};
