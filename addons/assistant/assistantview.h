/*
 *  SPDX-FileCopyrightText: 2025 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <KTextEditor/MainWindow>
#include <KXMLGUIClient>

class Assistant;

class AssistantView : public QObject, public KXMLGUIClient
{
public:
    explicit AssistantView(Assistant *plugin, KTextEditor::MainWindow *mainwindow);
    ~AssistantView();

    void assistantInvoke();

private:
    Assistant *const m_assistant;
    KTextEditor::MainWindow *m_mainWindow = nullptr;
};
