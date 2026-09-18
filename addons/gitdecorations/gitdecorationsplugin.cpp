/*
    SPDX-FileCopyrightText: 2026 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitdecorationsplugin.h"
#include "gitannotationdelegate.h"
#include "gitannotationmodel.h"
#include "gitdiff.h"
#include "gitprocess.h"
#include "hostprocess.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QPointer>

#include <KLocalizedString>
#include <KPluginFactory>
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
    const auto app = KTextEditor::Editor::instance()->application();
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

    if (!it->annotationNeedsUpdate || it->diffProcess) {
        return;
    }

    bool hasViewToAnnotate = view != nullptr;
    if (!hasViewToAnnotate) {
        const auto views = document->views();
        hasViewToAnnotate = std::any_of(views.cbegin(), views.cend(), [](KTextEditor::View *v) {
            return v->isVisible();
        });
    }

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
        m_trackedDocuments.insert(document, DocumentContext{.repoBasePath = *repoBasePath, .diffProcess = nullptr, .annotationNeedsUpdate = true});
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
    if (it == m_trackedDocuments.cend()) {
        return;
    }

    auto context = it.value();
    m_trackedDocuments.erase(it);
    disconnect(document, &KTextEditor::Document::documentSavedOrUploaded, this, &GitDecorationsPlugin::invalidateAnnotations);
    disconnect(document, &KTextEditor::Document::modifiedOnDisk, this, &GitDecorationsPlugin::invalidateAnnotations);
    if (context.diffProcess && context.diffProcess->state() != QProcess::NotRunning) {
        context.diffProcess->kill();
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
        qCWarning(gitDecorationsLog, "Cannot add path to watcher: %ls", qUtf16Printable(gitPath));
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
    if (it == m_trackedRepositories.cend()) {
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
    if (it == m_trackedRepositories.cend()) {
        return;
    }

    const auto context = it.value();
    if (context->headProcess && context->headProcess->state() != QProcess::NotRunning) {
        context->headProcess->kill();
    }

    auto *headProcess = new QProcess(this);
    if (!setupGitProcess(*headProcess, repoBasePath, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")})) {
        headProcess->deleteLater();
        return;
    }

    context->headProcess = headProcess;
    connect(headProcess, &QProcess::finished, this, [this, headProcess, repoBasePath](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qCWarning(gitDecorationsLog,
                      "Git rev-parse process failed for %ls with exit code %d. %ls",
                      qUtf16Printable(repoBasePath),
                      exitCode,
                      qUtf16Printable(headProcess->errorString()));
            headProcess->deleteLater();
            return;
        }

        // Avoid using stale state
        auto it = m_trackedRepositories.constFind(repoBasePath);
        if (it == m_trackedRepositories.constEnd()) {
            return;
        }

        const auto context = it.value();
        if (context->headProcess != headProcess) {
            headProcess->deleteLater();
            return;
        }

        QString headCommit = QString::fromUtf8(headProcess->readAllStandardOutput());
        if (!headCommit.isEmpty() && headCommit != context->headCommit) {
            bool isFirstUpdate = context->headCommit.isEmpty();
            context->headCommit = headCommit;
            if (!isFirstUpdate) {
                for (auto it = m_trackedDocuments.cbegin(); it != m_trackedDocuments.cend(); ++it) {
                    if (it.value().repoBasePath == context->repoBasePath) {
                        invalidateAnnotations(it.key());
                    }
                }
            }
        }

        headProcess->deleteLater();
    });

    qCDebug(gitDecorationsLog, "Starting git rev-parse HEAD process for: %ls", qUtf16Printable(repoBasePath));
    startHostProcess(*headProcess);
}

void GitDecorationsPlugin::annotateDocument(KTextEditor::Document *document)
{
    const auto it = m_trackedDocuments.find(document);
    if (it == m_trackedDocuments.end()) {
        return;
    }

    const auto context = it.value();
    if (context.diffProcess && context.diffProcess->state() != QProcess::NotRunning) {
        context.diffProcess->kill();
    }

    auto *diffProcess = new QProcess(document);
    const QString relativePath = QDir(context.repoBasePath).relativeFilePath(document->url().toLocalFile());
    if (!setupGitProcess(*diffProcess, context.repoBasePath, {QStringLiteral("diff"), QStringLiteral("HEAD"), QStringLiteral("--"), relativePath})) {
        diffProcess->deleteLater();
        return;
    }

    it->diffProcess = diffProcess;
    QPointer<KTextEditor::Document> targetDocument = document;
    const QUrl targetUrl = document->url();

    connect(diffProcess, &QProcess::finished, this, [this, diffProcess, targetDocument, targetUrl](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qCWarning(gitDecorationsLog,
                      "Git diff process failed for %ls with exit code %d. %ls",
                      qUtf16Printable(targetUrl.toLocalFile()),
                      exitCode,
                      qUtf16Printable(diffProcess->errorString()));
            diffProcess->deleteLater();
            return;
        }

        // Avoid using stale state
        if (!targetDocument || targetDocument->url() != targetUrl) {
            diffProcess->deleteLater();
            return;
        }

        const auto it = m_trackedDocuments.find(targetDocument);
        if (it == m_trackedDocuments.end() || it->diffProcess != diffProcess) {
            diffProcess->deleteLater();
            return;
        }

        VcsDiff diff;
        diff.setDiff(QString::fromUtf8(diffProcess->readAllStandardOutput()));

        auto *model = new GitAnnotationModel(targetDocument);
        model->setDiff(diff);

        it->annotationNeedsUpdate = false;
        targetDocument->setAnnotationModel(model);

        const auto views = targetDocument->views();
        for (auto *view : views) {
            annotateView(view);
        }

        diffProcess->deleteLater();
    });

    qCDebug(gitDecorationsLog, "Starting git diff HEAD process for: %ls", qUtf16Printable(document->url().toLocalFile()));
    startHostProcess(*diffProcess);
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
