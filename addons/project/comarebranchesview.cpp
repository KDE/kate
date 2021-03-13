#include "comarebranchesview.h"
#include "kateprojectpluginview.h"
#include "kateprojectworker.h"

#include <QVBoxLayout>

#include <KLocalizedString>
#include <QProcess>

static QVariantMap createMap(const QStringList &files)
{
    QVariantMap cnf, filesMap;
    filesMap[QStringLiteral("list")] = (files);
    cnf[QStringLiteral("name")] = QStringLiteral("ComparingBranches");
    cnf[QStringLiteral("files")] = (QVariantList() << filesMap);
    return cnf;
}

CompareBranchesView::CompareBranchesView(QWidget *parent, const QString &gitPath, const QString fromB, const QString &toBr, QStringList files)
    : QWidget(parent)
    , m_files(std::move(files))
    , m_gitDir(gitPath)
    , m_fromBr(fromB)
    , m_toBr(toBr)
{
    setLayout(new QVBoxLayout);

    KateProjectWorker *w = new KateProjectWorker(m_gitDir, QString(), createMap(m_files), true);
    connect(w, &KateProjectWorker::loadDone, this, &CompareBranchesView::loadFilesDone, Qt::QueuedConnection);
    w->run();

    m_backBtn.setText(i18n("Back"));
    m_backBtn.setIcon(QIcon::fromTheme(QStringLiteral("draw-arrow-back.svg")));
    connect(&m_backBtn, &QPushButton::clicked, this, &CompareBranchesView::backClicked);
    layout()->addWidget(&m_backBtn);

    m_tree.setModel(&m_model);
    layout()->addWidget(&m_tree);

    m_tree.setHeaderHidden(true);

    connect(&m_tree, &QTreeView::clicked, this, &CompareBranchesView::showDiff);
}

void CompareBranchesView::loadFilesDone(const KateProjectSharedQStandardItem &topLevel, KateProjectSharedQHashStringItem)
{
    m_model.clear();
    m_model.invisibleRootItem()->appendColumn(topLevel->takeColumn(0));
    m_tree.expandAll();
}

void CompareBranchesView::showDiff(const QModelIndex &idx)
{
    auto file = idx.data(Qt::UserRole).toString().remove(m_gitDir + QLatin1Char('/'));
    QProcess git;
    git.setWorkingDirectory(m_gitDir);
    QStringList args{QStringLiteral("diff"), QStringLiteral("%1...%2").arg(m_fromBr).arg(m_toBr), QStringLiteral("--"), file};
    git.start(QStringLiteral("git"), args, QProcess::ReadOnly);

    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return;
        }
    }
    m_pluginView->showDiffInFixedView(git.readAllStandardOutput());
}
