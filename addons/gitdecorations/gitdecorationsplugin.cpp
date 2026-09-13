/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitdecorationsplugin.h"
#include "gitannotationdelegate.h"
#include "gitannotationmodel.h"
#include "gitdiff.h"
#include "gitprocess.h"

#include <KPluginFactory>
#include <QLoggingCategory>
#include <QPointer>

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(GitDecorationsPluginFactory, "gitdecorationsplugin.json", registerPlugin<GitDecorationsPlugin>();)
Q_LOGGING_CATEGORY(gitDecorationsLog, "gitdecorations", QtWarningMsg)

GitDecorationsPlugin::GitDecorationsPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
    auto app = KTextEditor::Editor::instance()->application();
    connect(app, &KTextEditor::Application::documentCreated, this, &GitDecorationsPlugin::registerDocument);
}

void GitDecorationsPlugin::registerDocument(KTextEditor::Document *document)
{
    trackDocument(document);

    connect(document, &KTextEditor::Document::documentUrlChanged, this, &GitDecorationsPlugin::trackDocument);
    connect(document, &KTextEditor::Document::aboutToClose, this, &GitDecorationsPlugin::untrackDocument);
}

void GitDecorationsPlugin::invalidateAnnotations(KTextEditor::Document *document)
{
    if (!document) {
        return;
    }

    const auto it = m_trackedDocuments.find(document);
    if (it == m_trackedDocuments.end()) {
        return;
    }

    it->annotationNeedsUpdate = true;
    refreshAnnotations(document);
}

void GitDecorationsPlugin::refreshAnnotations(KTextEditor::Document *document, KTextEditor::View *view)
{
    const auto it = m_trackedDocuments.find(document);
    if (it == m_trackedDocuments.end()) {
        return;
    }

    if (!it->annotationNeedsUpdate || it->process) {
        return;
    }

    const bool hasViewToAnnotate = view != nullptr || std::any_of(document->views().cbegin(), document->views().cend(), [](KTextEditor::View *view) {
                                       return view->isVisible();
                                   });

    if (!hasViewToAnnotate) {
        return;
    }

    annotateDocument(document);
}

void GitDecorationsPlugin::trackDocument(KTextEditor::Document *document)
{
    if (!document) {
        return;
    }

    untrackDocument(document);

    if (!document->url().isLocalFile()) {
        return;
    }

    const QString filePath = document->url().toLocalFile();
    const QString fileDir = QFileInfo(filePath).absolutePath();
    const auto repoBasePath = getRepoBasePath(fileDir);

    if (repoBasePath) {
        trackRepository(*repoBasePath);
        m_trackedDocuments.insert(document, DocumentContext{.repoBasePath = *repoBasePath, .process = nullptr, .annotationNeedsUpdate = true});
        connect(document, &KTextEditor::Document::documentSavedOrUploaded, this, &GitDecorationsPlugin::invalidateAnnotations);
        connect(document, &KTextEditor::Document::modifiedOnDisk, this, &GitDecorationsPlugin::invalidateAnnotations);
        refreshAnnotations(document);
    }
}

void GitDecorationsPlugin::untrackDocument(KTextEditor::Document *document)
{
    if (!document) {
        return;
    }

    const auto it = m_trackedDocuments.constFind(document);
    if (it == m_trackedDocuments.end()) {
        return;
    }

    auto context = it.value();
    m_trackedDocuments.erase(it);
    disconnect(document, &KTextEditor::Document::documentSavedOrUploaded, this, &GitDecorationsPlugin::annotateDocument);
    disconnect(document, &KTextEditor::Document::modifiedOnDisk, this, &GitDecorationsPlugin::annotateDocument);
    if (context.process && context.process->state() != QProcess::NotRunning) {
        context.process->kill();
    }

    QString repoBasePath = context.repoBasePath;
    const bool repoHasTrackedDocuments = std::any_of(m_trackedDocuments.cbegin(), m_trackedDocuments.cend(), [repoBasePath](const DocumentContext &context) {
        return context.repoBasePath == repoBasePath;
    });

    if (!repoHasTrackedDocuments) {
        untrackRepository(repoBasePath);
    }
}

void GitDecorationsPlugin::trackRepository(const QString &repoBasePath)
{
    if (m_trackedRepositories.contains(repoBasePath)) {
        return;
    }

    auto repoContext = QSharedPointer<RepositoryContext>::create();
    repoContext->repoBasePath = repoBasePath;
    repoContext->watcherTimer.setSingleShot(true);
    repoContext->watcherTimer.setInterval(300);

    QString gitPath = QDir(repoBasePath).filePath(QStringLiteral(".git"));
    if (!repoContext->watcher.addPath(gitPath)) {
        qCWarning(gitDecorationsLog) << "Cannot add path to watcher: " << gitPath;
        return;
    }

    connect(&repoContext->watcher, &QFileSystemWatcher::directoryChanged, this, [this, repoBasePath]() {
        const auto it = m_trackedRepositories.constFind(repoBasePath);
        if (it != m_trackedRepositories.cend()) {
            it.value()->watcherTimer.start();
        }
    });

    connect(&repoContext->watcherTimer, &QTimer::timeout, this, [this, repoBasePath]() {
        refreshRepositoryHead(repoBasePath);
    });

    m_trackedRepositories.insert(repoBasePath, repoContext);
    repoContext->watcherTimer.start();
}

void GitDecorationsPlugin::untrackRepository(const QString &repoBasePath)
{
    auto it = m_trackedRepositories.constFind(repoBasePath);
    if (it == m_trackedRepositories.end()) {
        return;
    }

    const auto context = it.value();
    context->watcherTimer.stop();
    context->watcher.removePaths(context->watcher.directories());
    if (context->headProcess && context->headProcess->state() != QProcess::NotRunning) {
        context->headProcess->kill();
    }

    m_trackedRepositories.erase(it);
}

void GitDecorationsPlugin::refreshRepositoryHead(const QString &repoBasePath)
{
    auto it = m_trackedRepositories.constFind(repoBasePath);
    if (it == m_trackedRepositories.end()) {
        return;
    }

    const auto context = it.value();
    if (context->headProcess && context->headProcess->state() != QProcess::NotRunning) {
        context->headProcess->kill();
    }

    auto *process = new QProcess(this);
    if (!setupGitProcess(*process, repoBasePath, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")})) {
        process->deleteLater();
        return;
    }

    context->headProcess = process;
    connect(process, &QProcess::finished, this, [this, process, repoBasePath](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            process->deleteLater();
            return;
        }

        // Avoid using stale state
        auto it = m_trackedRepositories.constFind(repoBasePath);
        if (it == m_trackedRepositories.constEnd()) {
            return;
        }

        const auto context = it.value();
        if (context->headProcess != process) {
            process->deleteLater();
            return;
        }

        QString head = QString::fromUtf8(process->readAllStandardOutput());
        if (!head.isEmpty() && head != context->head) {
            bool isFirstRun = context->head.isEmpty();
            context->head = head;
            if (!isFirstRun) {
                for (auto it = m_trackedDocuments.cbegin(); it != m_trackedDocuments.cend(); ++it) {
                    if (it.value().repoBasePath == context->repoBasePath) {
                        invalidateAnnotations(it.key());
                    }
                }
            }
        }

        process->deleteLater();
    });

    qCDebug(gitDecorationsLog) << "Starting git rev-parse HEAD process for: " << repoBasePath;
    process->start();
}

void GitDecorationsPlugin::annotateDocument(KTextEditor::Document *document)
{
    const auto it = m_trackedDocuments.find(document);
    if (it == m_trackedDocuments.end()) {
        return;
    }

    const auto context = it.value();
    if (context.process && context.process->state() != QProcess::NotRunning) {
        context.process->kill();
    }

    auto *process = new QProcess(document);
    const QString relativePath = QDir(context.repoBasePath).relativeFilePath(document->url().toLocalFile());
    if (!setupGitProcess(*process, context.repoBasePath, {QStringLiteral("diff"), QStringLiteral("HEAD"), QStringLiteral("--"), relativePath})) {
        process->deleteLater();
        return;
    }

    it->process = process;
    QPointer<KTextEditor::Document> targetDocument = document;
    const QUrl targetUrl = document->url();

    connect(process, &QProcess::finished, this, [this, process, targetDocument, targetUrl](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            process->deleteLater();
            return;
        }

        // Avoid using stale state
        if (!targetDocument || targetDocument->url() != targetUrl) {
            process->deleteLater();
            return;
        }

        const auto it = m_trackedDocuments.find(targetDocument);
        if (it == m_trackedDocuments.cend() || it->process != process) {
            process->deleteLater();
            return;
        }

        VcsDiff diff;
        diff.setDiff(QString::fromUtf8(process->readAllStandardOutput()));

        auto *model = new GitAnnotationModel(targetDocument);
        model->setDiff(diff);

        it->annotationNeedsUpdate = false;
        targetDocument->setAnnotationModel(model);
        for (auto view : targetDocument->views()) {
            annotateView(view);
        }

        process->deleteLater();
    });

    qCDebug(gitDecorationsLog) << "Starting git diff HEAD process for: " << document->url().toLocalFile();
    process->start();
}

QObject *GitDecorationsPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new GitDecorationsPluginView(this, mainWindow);
}

void GitDecorationsPlugin::annotateView(KTextEditor::View *view)
{
    if (!view || !view->document()) {
        return;
    }

    if (m_trackedDocuments.contains(view->document())) {
        refreshAnnotations(view->document(), view);
        view->setAnnotationBorderVisible(true);
        if (!dynamic_cast<GitAnnotationDelegate *>(view->annotationItemDelegate())) {
            view->setAnnotationItemDelegate(new GitAnnotationDelegate(view));
        }
    }
}

GitDecorationsPluginView::GitDecorationsPluginView(GitDecorationsPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
{
    connect(mainwindow, &KTextEditor::MainWindow::viewChanged, plugin, &GitDecorationsPlugin::annotateView);
}

#include "gitdecorationsplugin.moc"
