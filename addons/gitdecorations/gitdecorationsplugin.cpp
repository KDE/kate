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
}

void GitDecorationsPlugin::annotateView(KTextEditor::View *view)
{
    if (!view || !view->document()->url().isLocalFile()) {
        return;
    }

    const QString filePath = view->document()->url().toLocalFile();
    const QString directory = QFileInfo(filePath).absolutePath();
    const auto repoBasePath = getRepoBasePath(directory);

    if (repoBasePath) {
        const QString relativePath = QDir(*repoBasePath).relativeFilePath(filePath);
        auto *process = new QProcess(view);

        if (!setupGitProcess(*process, *repoBasePath, {QStringLiteral("diff"), QStringLiteral("HEAD"), QStringLiteral("--"), relativePath})) {
            process->deleteLater();
            return;
        }

        QPointer<KTextEditor::View> targetView = view;
        const QUrl targetUrl = view->document()->url();

        connect(process, &QProcess::finished, this, [process, targetView, targetUrl](int exitCode, QProcess::ExitStatus exitStatus) {
            const QByteArray output = process->readAllStandardOutput();

            if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                process->deleteLater();
                return;
            }

            if (!targetView || targetView->document()->url() != targetUrl) {
                process->deleteLater();
                return;
            }

            VcsDiff diff;
            diff.setDiff(QString::fromUtf8(output));

            auto *model = new GitAnnotationModel(targetView);
            model->setDiff(diff);

            targetView->setAnnotationModel(model);
            targetView->setAnnotationBorderVisible(true);
            auto *delegate = new GitAnnotationDelegate(targetView);
            targetView->setAnnotationItemDelegate(delegate);

            process->deleteLater();
        });

        process->start();
    }
}

QObject *GitDecorationsPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new GitDecorationsPluginView(this, mainWindow);
}

GitDecorationsPluginView::GitDecorationsPluginView(GitDecorationsPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
    , m_mainWindow(mainwindow)
{
    connect(mainwindow, &KTextEditor::MainWindow::viewChanged, plugin, &GitDecorationsPlugin::annotateView);
}

#include "gitdecorationsplugin.moc"
