#include "gitwidget.h"
#include "gitstatusmodel.h"
#include "kateproject.h"

#include <QDebug>
#include <QProcess>
#include <QPushButton>
#include <QStringListModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QtConcurrentRun>

GitWidget::GitWidget(KateProject *project, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
{
    m_menuBtn = new QPushButton(this);
    m_commitBtn = new QPushButton(this);
    m_treeView = new QTreeView(this);

    m_menuBtn->setIcon(QIcon::fromTheme(QStringLiteral("application-menu")));
    m_commitBtn->setIcon(QIcon(QStringLiteral(":/kxmlgui5/kateproject/git-commit-dark.svg")));

    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *btnsLayout = new QHBoxLayout;

    btnsLayout->addWidget(m_commitBtn);
    btnsLayout->addWidget(m_menuBtn);

    layout->addLayout(btnsLayout);
    layout->addWidget(m_treeView);

    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);

    m_model = new GitStatusModel(this);
    m_treeView->setModel(m_model);

    setLayout(layout);

    connect(&m_gitStatusWatcher, &QFutureWatcher<GitWidget::GitParsedStatus>::finished, this, &GitWidget::parseStatusReady);

    getStatus(m_project->baseDir());
}

void GitWidget::getStatus(const QString &repo, bool submodules)
{
    connect(&git, &QProcess::readyRead, this, &GitWidget::gitStatusReady);

    auto args = QStringList{QStringLiteral("status"), QStringLiteral("-z"), QStringLiteral("-u")};
    if (!submodules) {
        args.append(QStringLiteral("--ignore-submodules"));
    }
    git.setArguments(args);
    git.setProgram(QStringLiteral("git"));
    git.setWorkingDirectory(repo);
    git.start();
}

void GitWidget::gitStatusReady()
{
    disconnect(&git, &QProcess::readyRead, this, &GitWidget::gitStatusReady);
    QByteArray s = git.readAllStandardOutput();
    auto future = QtConcurrent::run(this, &GitWidget::parseStatus, s);
    m_gitStatusWatcher.setFuture(future);

    auto l = s.split(0x00);
    QStringList out;
    for (const auto &m : l) {
        out.append(QString::fromLocal8Bit(m));
    }
}

GitWidget::GitParsedStatus GitWidget::parseStatus(const QByteArray &raw)
{
    QVector<GitUtils::StatusItem> untracked;
    QVector<GitUtils::StatusItem> unmerge;
    QVector<GitUtils::StatusItem> staged;
    QVector<GitUtils::StatusItem> changed;

    QList<QByteArray> rawList = raw.split(0x00);
    for (const auto &r : rawList) {
        if (r.isEmpty() || r.length() < 3) {
            continue;
        }

        char x = r.at(0);
        char y = r.at(1);
        uint16_t xy = (((uint16_t)x) << 8) | y;
        using namespace GitUtils;

        const char *file = r.data() + 3;
        const int size = r.size() - 3;

        switch (xy) {
        case StatusXY::QQ:
            untracked.append({QString::fromUtf8(file, size), GitStatus::Untracked});
            break;
        case StatusXY::II:
            untracked.append({QString::fromUtf8(file, size), GitStatus::Ignored});
            break;

        case StatusXY::DD:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothDeleted});
            break;
        case StatusXY::AU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_AddedByUs});
            break;
        case StatusXY::UD:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_DeletedByThem});
            break;
        case StatusXY::UA:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_AddedByThem});
            break;
        case StatusXY::DU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_DeletedByUs});
            break;
        case StatusXY::AA:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothAdded});
            break;
        case StatusXY::UU:
            unmerge.append({QString::fromUtf8(file, size), GitStatus::Unmerge_BothModified});
            break;

        case StatusXY::M_:
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Modified});
            break;
        case StatusXY::A_:
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Added});
            break;
        case StatusXY::D_:
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Deleted});
            break;
        case StatusXY::R_:
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Renamed});
            break;
        case StatusXY::C_:
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Copied});
            break;

        case StatusXY::_M:
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Modified});
            break;
        case StatusXY::_D:
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Deleted});
            break;
        case StatusXY::_A:
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_IntentToAdd});
            break;
        }
    }

    return {untracked, unmerge, staged, changed};
}

void GitWidget::hideEmptyTreeNodes()
{
    const auto emptyRows = m_model->emptyRows();
    for (const int row : emptyRows) {
        m_treeView->setRowHidden(row, QModelIndex(), true);
    }
}

void GitWidget::parseStatusReady()
{
    GitParsedStatus s = m_gitStatusWatcher.result();
    m_model->addItems(s.staged, s.changed, s.unmerge, s.untracked);

    hideEmptyTreeNodes();
}
