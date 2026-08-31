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
    annotateDocument(document);

    connect(document, &KTextEditor::Document::documentUrlChanged, this, &GitDecorationsPlugin::annotateDocument);
    connect(document, &KTextEditor::Document::documentSavedOrUploaded, this, &GitDecorationsPlugin::annotateDocument);
    connect(document, &KTextEditor::Document::aboutToClose, this, [this](KTextEditor::Document *closingDocument) {
        if (auto process = m_processes.take(closingDocument)) {
            if (process->state() != QProcess::NotRunning) {
                process->kill();
            }
        }
    });
}

void GitDecorationsPlugin::annotateDocument(KTextEditor::Document *document)
{
    if (!document || !document->url().isLocalFile()) {
        return;
    }

    if (auto process = m_processes.take(document)) {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
    }

    // TODO: Avoid running get getRepoBasePath every time

    const QString filePath = document->url().toLocalFile();
    const QString directory = QFileInfo(filePath).absolutePath();
    const auto repoBasePath = getRepoBasePath(directory);

    if (repoBasePath) {
        const QString relativePath = QDir(*repoBasePath).relativeFilePath(filePath);
        auto *process = new QProcess(document);

        if (!setupGitProcess(*process, *repoBasePath, {QStringLiteral("diff"), QStringLiteral("HEAD"), QStringLiteral("--"), relativePath})) {
            process->deleteLater();
            return;
        }

        m_processes.insert(document, process);
        QPointer<KTextEditor::Document> targetDocument = document;
        const QUrl targetUrl = document->url();

        connect(process, &QProcess::finished, this, [this, process, targetDocument, targetUrl](int exitCode, QProcess::ExitStatus exitStatus) {
            const QByteArray output = process->readAllStandardOutput();

            if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                process->deleteLater();
                return;
            }

            if (!targetDocument || targetDocument->url() != targetUrl) { // Avoid applying stale results
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
