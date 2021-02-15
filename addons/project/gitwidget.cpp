#include "gitwidget.h"
#include "gitstatusmodel.h"
#include "kateproject.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QDialog>
#include <QEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStringListModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <KLocalizedString>

#include <KTextEditor/ConfigInterface>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/View>

GitWidget::GitWidget(KateProject *project, QWidget *parent, KTextEditor::MainWindow *mainWindow)
    : QWidget(parent)
    , m_project(project)
    , m_mainWin(mainWindow)
{
    m_menuBtn = new QPushButton(this);
    m_commitBtn = new QPushButton(this);
    m_treeView = new QTreeView(this);

    m_menuBtn->setIcon(QIcon::fromTheme(QStringLiteral("application-menu")));
    m_commitBtn->setIcon(QIcon(QStringLiteral(":/kxmlgui5/kateproject/git-commit-dark.svg")));
    m_commitBtn->setText(i18n("Commit"));

    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *btnsLayout = new QHBoxLayout;

    btnsLayout->addWidget(m_commitBtn);
    btnsLayout->addWidget(m_menuBtn);

    layout->addLayout(btnsLayout);
    layout->addWidget(m_treeView);

    m_model = new GitStatusModel(this);

    m_treeView->setHeaderHidden(true);
    //    m_treeView->setRootIsDecorated(false);
    m_treeView->setModel(m_model);
    m_treeView->installEventFilter(this);

    setLayout(layout);

    connect(&m_gitStatusWatcher, &QFutureWatcher<GitWidget::GitParsedStatus>::finished, this, &GitWidget::parseStatusReady);
    connect(m_commitBtn, &QPushButton::clicked, this, &GitWidget::opencommitChangesDialog);

    getStatus(m_project->baseDir());
}

void GitWidget::sendMessage(const QString &message, bool warn)
{
    KTextEditor::Message *msg = new KTextEditor::Message(message, warn ? KTextEditor::Message::Warning : KTextEditor::Message::Positive);
    msg->setPosition(KTextEditor::Message::TopInView);
    msg->setAutoHide(3000);
    msg->setAutoHideMode(KTextEditor::Message::Immediate);
    msg->setView(m_mainWin->activeView());
    m_mainWin->activeView()->document()->postMessage(msg);
}

void GitWidget::getStatus(const QString &repo, bool untracked, bool submodules)
{
    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    connect(&git, &QProcess::finished, this, &GitWidget::gitStatusReady);

    auto args = QStringList{QStringLiteral("status"), QStringLiteral("-z")};
    if (!untracked) {
        args.append(QStringLiteral("-uno"));
    } else {
        args.append(QStringLiteral("-u"));
    }
    if (!submodules) {
        args.append(QStringLiteral("--ignore-submodules"));
    }
    git.setArguments(args);
    git.setProgram(QStringLiteral("git"));
    git.setWorkingDirectory(repo);
    git.start();
}

void GitWidget::stage(const QString &file, bool untracked)
{
    auto args = QStringList{QStringLiteral("add"), QStringLiteral("-A"), QStringLiteral("--")};

    // all
    if (file.isEmpty()) {
        const QVector<GitUtils::StatusItem> &files = untracked ? m_model->untrackedFiles() : m_model->changedFiles();
        args.reserve(args.size() + files.size());
        for (const auto &file : files) {
            args.append(file.file);
        }
    } else {
        // one file
        args.append(file);
    }

    git.setWorkingDirectory(m_project->baseDir());
    git.setProgram(QStringLiteral("git"));
    git.setArguments(args);
    git.start();

    if (git.waitForStarted() && git.waitForFinished(-1)) {
        getStatus(m_project->baseDir());
    }
}

void GitWidget::unstage(const QString &file)
{
    // git reset -q HEAD --
    auto args = QStringList{QStringLiteral("reset"), QStringLiteral("-q"), QStringLiteral("HEAD"), QStringLiteral("--")};

    // all
    if (file.isEmpty()) {
        const QVector<GitUtils::StatusItem> &files = m_model->stagedFiles();
        args.reserve(args.size() + files.size());
        for (const auto &file : files) {
            args.append(file.file);
        }
    } else {
        // one file
        args.append(file);
    }

    git.setWorkingDirectory(m_project->baseDir());
    git.setProgram(QStringLiteral("git"));
    git.setArguments(args);
    git.start();

    if (git.waitForStarted() && git.waitForFinished(-1)) {
        getStatus(m_project->baseDir());
    }
}

void GitWidget::commitChanges(const QString &msg, const QString &desc)
{
    auto args = QStringList{QStringLiteral("commit"), QStringLiteral("-m"), msg};
    if (!desc.isEmpty()) {
        args.append(QStringLiteral("-m"));
        args.append(desc);
    }

    git.setWorkingDirectory(m_project->baseDir());
    git.setProgram(QStringLiteral("git"));
    git.setArguments(args);
    git.start();

    connect(&git, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
        if (exitCode > 0) {
            sendMessage(i18n("Failed to commit. \n %1", QString::fromUtf8(git.readAllStandardError())), true);
        } else {
            sendMessage(i18n("Changes commited successfully."), false);
            // refresh
            getStatus(m_project->baseDir());
        }
    });
}

void GitWidget::opencommitChangesDialog()
{
    if (m_model->stagedFiles().isEmpty()) {
        return sendMessage(i18n("Nothing to commit. Please stage your changes first.", QString::fromUtf8(git.readAllStandardError())), true);
    }

    auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(m_mainWin->activeView());
    QFont font;
    if (ciface) {
        font = ciface->configValue(QStringLiteral("font")).value<QFont>();
    } else {
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }

    QDialog dialog;
    dialog.setWindowFlags(Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
    dialog.setWindowTitle(i18n("Commit Changes"));

    QVBoxLayout vlayout;
    QHBoxLayout hLayout;
    dialog.setLayout(&vlayout);

    QLineEdit le;
    le.setPlaceholderText(i18n("Write commit message..."));
    le.setFont(font);

    QPlainTextEdit pe;
    pe.setPlaceholderText(i18n("Extended commit description..."));
    pe.setFont(font);

    // restore last message ?
    if (!m_commitMessage.isEmpty()) {
        auto msgs = m_commitMessage.split(QStringLiteral("\n\n"));
        if (!msgs.isEmpty()) {
            le.setText(msgs.at(0));
            if (msgs.length() > 1) {
                pe.setPlainText(msgs.at(1));
            }
        }
    }

    vlayout.addWidget(&le);
    vlayout.addWidget(&pe);

    QPushButton ok(i18n("Ok"));
    QPushButton cancel(i18n("Cancel"));
    hLayout.addStretch();
    hLayout.addWidget(&ok);
    hLayout.addWidget(&cancel);

    connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(&cancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    vlayout.addLayout(&hLayout);

    int res = dialog.exec();
    if (res == QDialog::Accepted) {
        if (le.text().isEmpty()) {
            return sendMessage(i18n("Commit message cannot be empty."), true);
        }
        commitChanges(le.text(), pe.toPlainText());
    } else {
        m_commitMessage = le.text() + QStringLiteral("\n\n") + pe.toPlainText();
    }
}

void GitWidget::gitStatusReady(int exit, QProcess::ExitStatus)
{
    if (exit > 0) {
        sendMessage(i18n("Failed to get git-status. Error: %1", QString::fromUtf8(git.readAllStandardError())), true);
        return;
    }

    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    QByteArray s = git.readAllStandardOutput();
    auto future = QtConcurrent::run(this, &GitWidget::parseStatus, s);
    m_gitStatusWatcher.setFuture(future);
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
        }

        switch (x) {
        case 'M':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Modified});
            break;
        case 'A':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Added});
            break;
        case 'D':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Deleted});
            break;
        case 'R':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Renamed});
            break;
        case 'C':
            staged.append({QString::fromUtf8(file, size), GitStatus::Index_Copied});
            break;
        }

        switch (y) {
        case 'M':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Modified});
            break;
        case 'D':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_Deleted});
            break;
        case 'A':
            changed.append({QString::fromUtf8(file, size), GitStatus::WorkingTree_IntentToAdd});
            break;
        }
    }

    return {untracked, unmerge, staged, changed};
}

void GitWidget::hideEmptyTreeNodes()
{
    const auto emptyRows = m_model->emptyRows();
    for (int i = 0; i < 4; ++i) {
        if (emptyRows.contains(i)) {
            m_treeView->setRowHidden(i, QModelIndex(), true);
        } else {
            m_treeView->setRowHidden(i, QModelIndex(), false);
            if (i != GitStatusModel::NodeUntrack) {
                m_treeView->expand(m_model->getModelIndex((GitStatusModel::ItemType)i));
            }
        }
    }
}

void GitWidget::parseStatusReady()
{
    GitParsedStatus s = m_gitStatusWatcher.result();
    m_model->addItems(s.staged, s.changed, s.unmerge, s.untracked);

    hideEmptyTreeNodes();
}

bool GitWidget::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::ContextMenu) {
        if (o != m_treeView)
            return QWidget::eventFilter(o, e);
        QContextMenuEvent *cme = static_cast<QContextMenuEvent *>(e);
        treeViewContextMenuEvent(cme);
    }
    return QWidget::eventFilter(o, e);
}

void GitWidget::treeViewContextMenuEvent(QContextMenuEvent *e)
{
    // discard=>git checkout -q -- /home/waqar/Projects/syntest/App.js
    auto idx = m_model->index(m_treeView->currentIndex().row(), 0, m_treeView->currentIndex().parent());
    auto type = idx.data(GitStatusModel::TreeItemType);

    if (type == GitStatusModel::NodeChanges || type == GitStatusModel::NodeUntrack) {
        QMenu menu;
        auto stageAct = menu.addAction(i18n("Stage All"));
        //        auto discardAct = type == GitStatusModel::NodeChanges ? menu.addAction(i18n("Discard All")) : nullptr;

        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
        if (act == stageAct) {
            stage(QString(), type == GitStatusModel::NodeUntrack);
        }
    } else if (type == GitStatusModel::NodeFile) {
        QMenu menu;
        bool unstaging = idx.internalId() == GitStatusModel::NodeStage;
        auto stageAct = unstaging ? menu.addAction(i18n("Unstage file")) : menu.addAction(i18n("Stage file"));
        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));

        const QString file = QString(m_project->baseDir() + QStringLiteral("/") + idx.data().toString());
        if (act == stageAct) {
            if (unstaging) {
                return unstage(file);
            }
            return stage(file);
        }
    } else if (type == GitStatusModel::NodeStage) {
        QMenu menu;
        auto stage = menu.addAction(i18n("Unstage All"));
        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));

        // git reset -q HEAD --
        if (act == stage) {
            unstage(QString());
        }
    }
}
