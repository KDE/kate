/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QJSEngine>
#include <QPointer>
#include <QVariant>

class RBQLPlugin final : public KTextEditor::Plugin
{
public:
    explicit RBQLPlugin(QObject *parent);

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class RBQLPluginView final : public QObject, public KXMLGUIClient
{
public:
    explicit RBQLPluginView(RBQLPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~RBQLPluginView();

private:
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<QWidget> m_toolview;
};
