/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QObject>
#include <memory>

class LSPClientPlugin;
class LSPClientServerManager;

namespace KTextEditor
{
class MainWindow;
}

class LSPClientPluginView
{
public:
    // only needs a factory; no other public interface
    static QObject *new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin, std::shared_ptr<LSPClientServerManager> manager);
};
