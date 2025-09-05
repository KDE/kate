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
public:
    explicit ESLintPlugin(QObject *parent);

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

struct DiagnosticWithFix {
    Diagnostic diag;
    struct Fix {
        int rangeStart = 0;
        int rangeEnd = 0;
        QString text;
    } fix;
};

class ESLintPluginView final : public QObject, public KXMLGUIClient
{
public:
    explicit ESLintPluginView(KTextEditor::MainWindow *mainwindow);
    ~ESLintPluginView();

private:
    void onActiveViewChanged(KTextEditor::View *);
    void onSaved(KTextEditor::Document *d);
    void onReadyRead();
    void onError();
    void onFixesRequested(const QUrl &, const Diagnostic &, const QVariant &);
    void fixDiagnostic(const QUrl &url, const DiagnosticWithFix::Fix &fix);

    QPointer<KTextEditor::Document> m_activeDoc;
    KTextEditor::MainWindow *m_mainWindow;
    DiagnosticsProvider m_provider;
    QProcess m_eslintProcess;
    std::vector<DiagnosticWithFix> m_diagsWithFix;
};
