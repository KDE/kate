/*
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef LSPCLIENTPLUGINVIEW_H
#define LSPCLIENTPLUGINVIEW_H

#include <QObject>

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
    static QObject *new_(LSPClientPlugin *plugin, KTextEditor::MainWindow *mainWin, QSharedPointer<LSPClientServerManager> manager);
};

#endif
