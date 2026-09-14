/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QFileSystemWatcher>
#include <QHash>
#include <QPointer>
#include <QProcess>
#include <QSharedPointer>
#include <QString>
#include <QTimer>

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
    void invalidateAnnotations(KTextEditor::Document *document);
    void refreshAnnotations(KTextEditor::Document *document, KTextEditor::View *view = nullptr);
    void trackRepository(const QString &repoBasePath);
    void untrackRepository(const QString &repoBasePath);
    void refreshRepositoryHead(const QString &repoBasePath);

private:
    struct DocumentContext {
        QString repoBasePath;
        QPointer<QProcess> diffProcess;
        bool annotationNeedsUpdate;
    };

    struct RepositoryContext {
        QString repoBasePath;
        QString headCommit;
        QPointer<QProcess> headProcess;
        QFileSystemWatcher watcher;
        QTimer watcherTimer;
    };

    QHash<QString, QSharedPointer<RepositoryContext>> m_trackedRepositories;
    QHash<KTextEditor::Document *, DocumentContext> m_trackedDocuments;
};

class GitDecorationsPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
    explicit GitDecorationsPluginView(GitDecorationsPlugin *plugin, KTextEditor::MainWindow *mainwindow);
};
