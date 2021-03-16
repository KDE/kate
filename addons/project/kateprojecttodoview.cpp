#include "kateprojecttodoview.h"

#include <QProcess>
#include <QStandardItemModel>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KLocalizedString>

KateProjectTODOView::KateProjectTODOView(KTextEditor::MainWindow *mainWin, const QString &projectPath)
    : m_projectPath(projectPath)
    , m_mainWindow(mainWin)
{
    refresh();

    m_model = new QStandardItemModel(this);
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(false);

    connect(m_treeView, &QTreeView::clicked, this, [this](const QModelIndex &idx) {
        QModelIndex fileIdx = m_model->index(idx.row(), 0);
        QModelIndex lineIdx = m_model->index(idx.row(), 1);
        if (!fileIdx.isValid() || !lineIdx.isValid() || !m_mainWindow) {
            return;
        }

        QUrl file = QUrl::fromLocalFile(fileIdx.data(Qt::UserRole + 2).toString());
        KTextEditor::View *v = m_mainWindow->openUrl(file);
        if (v) {
            v->setCursorPosition({lineIdx.data(Qt::UserRole + 1).toInt() - 1, 0});
        }
    });

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addWidget(m_treeView);
    setLayout(layout);
}

void KateProjectTODOView::reload(const QByteArray &data)
{
    m_model->clear();
    m_model->setHorizontalHeaderLabels(QStringList() << i18n("File") << i18n("Line") << i18n("Text"));

    const QList<QByteArray> lines = data.split('\n');

    for (const QByteArray &l : lines) {
        int colon = l.indexOf(':');
        if (colon < 0) {
            continue;
        }
        QString file = QString::fromUtf8(l.mid(0, colon));

        ++colon;

        int preCol = colon;
        colon = l.indexOf(':', colon);
        if (colon < 0) {
            continue;
        }

        QByteArray lineBa = l.mid(preCol, colon - preCol);
        QString line = QString::fromUtf8(lineBa);

        bool ok = false;
        int lineNo = lineBa.toInt(&ok);
        if (!ok) {
            continue;
        }

        QString text = QString::fromUtf8(l.mid(++colon));

        auto *lineCol = new QStandardItem();
        lineCol->setText(line);
        lineCol->setData(lineNo, Qt::UserRole + 1);
        auto *fileCol = new QStandardItem();
        fileCol->setText(file);
        fileCol->setData(QString(m_projectPath + QLatin1Char('/') + file), Qt::UserRole + 2);
        auto *textCol = new QStandardItem();
        textCol->setText(text.trimmed());

        m_model->appendRow({fileCol, lineCol, textCol});
    }
    m_treeView->resizeColumnToContents(0);
    m_treeView->resizeColumnToContents(1);
}

void KateProjectTODOView::refresh()
{
    QProcess *git = new QProcess(this);
    git->setWorkingDirectory(m_projectPath);
    git->setArguments({QStringLiteral("grep"),
                       QStringLiteral("-P"), // perl regex
                       QStringLiteral("-i"), // case insensitive
                       QStringLiteral("-n"), // get line numbers
                       QStringLiteral("(TODO|FIXME)\\b")});
    git->setProgram(QStringLiteral("git"));

    connect(git, &QProcess::finished, this, [this, git](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            git->deleteLater();
            return;
        }
        reload(git->readAllStandardOutput());
        git->deleteLater();
    });
    git->start(QProcess::ReadOnly);
}
