/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QPointer>
#include <QProcess>

#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIClient>

class GitDecorationsPlugin : public KTextEditor::Plugin
{
    Q_OBJECT
public:
    explicit GitDecorationsPlugin(QObject *parent);
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    void annotateView(KTextEditor::View *view);

private:
    void registerDocument(KTextEditor::Document *document);
    void annotateDocument(KTextEditor::Document *document);
    void trackDocument(KTextEditor::Document *document);
    void untrackDocument(KTextEditor::Document *document);

private:
    struct DocumentContext {
        QString repoBasePath;
        QPointer<QProcess> process;
    };

    QHash<KTextEditor::Document *, DocumentContext> m_trackedDocuments;
};

class GitDecorationsPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit GitDecorationsPluginView(GitDecorationsPlugin *plugin, KTextEditor::MainWindow *mainwindow);

private:
    KTextEditor::MainWindow *m_mainWindow = nullptr;
};
