#include "gitdecorationsplugin.h"
#include "gitannotationdelegate.h"
#include "gitannotationmodel.h"
#include "gitdiff.h"
#include "gitprocess.h"

#include <KPluginFactory>
#include <QPointer>

#include <KLocalizedString>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON(GitDecorationsPluginFactory, "gitdecorationsplugin.json", registerPlugin<GitDecorationsPlugin>();)

GitDecorationsPlugin::GitDecorationsPlugin(QObject *parent)
    : KTextEditor::Plugin(parent)
{
    auto app = KTextEditor::Editor::instance()->application();
    connect(app, &KTextEditor::Application::documentCreated, this, &GitDecorationsPlugin::registerDocument);
}

void GitDecorationsPlugin::registerDocument(KTextEditor::Document *document)
{
    trackDocument(document);

    // TODO: Annotate document again when HEAD changes, eg: document file is committed

    connect(document, &KTextEditor::Document::documentUrlChanged, this, &GitDecorationsPlugin::trackDocument);
    connect(document, &KTextEditor::Document::aboutToClose, this, &GitDecorationsPlugin::untrackDocument);
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
        m_trackedDocuments.insert(document, DocumentContext{.repoBasePath = *repoBasePath, .process = nullptr});
        connect(document, &KTextEditor::Document::documentSavedOrUploaded, this, &GitDecorationsPlugin::annotateDocument);
        connect(document, &KTextEditor::Document::modifiedOnDisk, this, &GitDecorationsPlugin::annotateDocument);
        annotateDocument(document);
    }
}

void GitDecorationsPlugin::untrackDocument(KTextEditor::Document *document)
{
    if (!document) {
        return;
    }

    const auto it = m_trackedDocuments.find(document);
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

    const QString filePath = document->url().toLocalFile();
    const QString relativePath = QDir(context.repoBasePath).relativeFilePath(filePath);

    auto *process = new QProcess(document);
    if (!setupGitProcess(*process, context.repoBasePath, {QStringLiteral("diff"), QStringLiteral("HEAD"), QStringLiteral("--"), relativePath})) {
        process->deleteLater();
        return;
    }

    it->process = process;
    QPointer<KTextEditor::Document> targetDocument = document;
    const QUrl targetUrl = document->url();

    connect(process, &QProcess::finished, this, [this, process, targetDocument, targetUrl](int exitCode, QProcess::ExitStatus exitStatus) {
        const QByteArray output = process->readAllStandardOutput();

        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            process->deleteLater();
            return;
        }

        // Avoid using stale state
        if (!targetDocument || targetDocument->url() != targetUrl) {
            process->deleteLater();
            return;
        }
        const auto it = m_trackedDocuments.constFind(targetDocument);
        if (it == m_trackedDocuments.cend() || it->process != process) {
            process->deleteLater();
            return;
        }

        VcsDiff diff;
        diff.setDiff(QString::fromUtf8(output));

        auto *model = new GitAnnotationModel(targetDocument);
        model->setDiff(diff);

        targetDocument->setAnnotationModel(model);
        for (auto view : targetDocument->views()) {
            annotateView(view);
        }

        process->deleteLater();
    });

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

    if (dynamic_cast<GitAnnotationModel *>(view->document()->annotationModel())) {
        view->setAnnotationBorderVisible(true);
        if (!dynamic_cast<GitAnnotationDelegate *>(view->annotationItemDelegate())) {
            view->setAnnotationItemDelegate(new GitAnnotationDelegate(view));
        }
    }
}

GitDecorationsPluginView::GitDecorationsPluginView(GitDecorationsPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
    , m_mainWindow(mainwindow)
{
    connect(mainwindow, &KTextEditor::MainWindow::viewChanged, plugin, &GitDecorationsPlugin::annotateView);
}

#include "gitdecorationsplugin.moc"
