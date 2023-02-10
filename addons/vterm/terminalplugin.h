/*
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Cursor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QPointer>
#include <QVariant>
#include <QWidget>

class TerminalPlugin final : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit TerminalPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class TerminalPluginView final : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit TerminalPluginView(TerminalPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~TerminalPluginView();

private:
    void handleEsc(QEvent *event);
    bool eventFilter(QObject *o, QEvent *e) override;

    QPointer<KTextEditor::Document> m_activeDoc;
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<QWidget> m_toolview;
    class KateTerminalWidget *m_termWidget = nullptr;
};
