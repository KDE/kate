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

#include <QPointer>
#include <QVariant>
#include <QJSEngine>

class RBQLPlugin final : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit RBQLPlugin(QObject *parent = nullptr, const QVariantList & = QVariantList());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class RBQLPluginView final : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit RBQLPluginView(RBQLPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~RBQLPluginView();

private:
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<QWidget> m_toolview;
};
