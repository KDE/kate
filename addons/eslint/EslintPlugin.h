/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Cursor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KXMLGUIClient>

#include <QProcess>
#include <diagnostics/diagnosticview.h>

#include <QPointer>
#include <QVariant>

class ESLintPlugin final : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit ESLintPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class ESLintPluginView final : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit ESLintPluginView(ESLintPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~ESLintPluginView();

private:
    void onActiveViewChanged(KTextEditor::View *);
    void onSaved(KTextEditor::Document *d);
    void onReadyRead();
    void onError();

    QPointer<KTextEditor::Document> m_activeDoc;
    ESLintPlugin *const m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    DiagnosticsProvider m_provider;
    QProcess m_eslintProcess;
};
