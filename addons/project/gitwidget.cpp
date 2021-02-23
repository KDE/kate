/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "gitwidget.h"
#include "branchesdialog.h"
#include "gitcommitdialog.h"
#include "gitstatusmodel.h"
#include "kateproject.h"
#include "stashdialog.h"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStringListModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <KLocalizedString>
#include <KMessageBox>

#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/View>

GitWidget::GitWidget(KateProject *project, KTextEditor::MainWindow *mainWindow, KateProjectPluginView *pluginView)
    : m_project(project)
    , m_mainWin(mainWindow)
    , m_pluginView(pluginView)
{
    m_commitBtn = new QToolButton(this);
    m_treeView = new QTreeView(this);

    initGitExe();

    buildMenu();
    m_menuBtn = new QToolButton(this);
    m_menuBtn->setAutoRaise(true);
    m_menuBtn->setMenu(m_gitMenu);
    m_menuBtn->setArrowType(Qt::NoArrow);
    m_menuBtn->setStyleSheet(QStringLiteral("QToolButton::menu-indicator{ image: none; }"));
    connect(m_menuBtn, &QToolButton::clicked, this, [this](bool) {
        m_menuBtn->showMenu();
    });

    m_menuBtn->setIcon(QIcon::fromTheme(QStringLiteral("application-menu")));
    m_commitBtn->setText(i18n("Commit"));
    m_commitBtn->setIcon(QIcon::fromTheme(QStringLiteral("svn-commit"))); // ":/kxmlgui5/kateproject/git-commit-dark.svg"
    m_commitBtn->setAutoRaise(true);
    m_commitBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_commitBtn->setSizePolicy(QSizePolicy::Minimum, m_commitBtn->sizePolicy().verticalPolicy());

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *btnsLayout = new QHBoxLayout;
    btnsLayout->setContentsMargins(0, 0, 0, 0);

    btnsLayout->addWidget(m_commitBtn);
    btnsLayout->addWidget(m_menuBtn);

    layout->addLayout(btnsLayout);
    layout->addWidget(m_treeView);

    m_model = new GitStatusModel(this);

    m_treeView->setUniformRowHeights(true);
    m_treeView->setHeaderHidden(true);
    m_treeView->setSelectionMode(QTreeView::ExtendedSelection);
    m_treeView->setModel(m_model);
    m_treeView->installEventFilter(this);

    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    setLayout(layout);

    connect(&m_gitStatusWatcher, &QFutureWatcher<GitUtils::GitParsedStatus>::finished, this, &GitWidget::parseStatusReady);
    connect(m_commitBtn, &QPushButton::clicked, this, &GitWidget::opencommitChangesDialog);
}

void GitWidget::initGitExe()
{
    git.setProgram(QStringLiteral("git"));
    // we initially use project base dir
    // and then calculate the exit .git path
    git.setWorkingDirectory(m_project->baseDir());
    git.setArguments({QStringLiteral("rev-parse"), QStringLiteral("--absolute-git-dir")});
    git.start(QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            sendMessage(i18n("Failed to find .git directory. Things may not work correctly. Error:\n%1", QString::fromUtf8(git.readAllStandardError())), true);
            m_gitPath = m_project->baseDir();
            return;
        }
        m_gitPath = QString::fromUtf8(git.readAllStandardOutput());
        if (m_gitPath.endsWith(QLatin1String("\n"))) {
            m_gitPath.remove(QLatin1String(".git\n"));
        } else {
            m_gitPath.remove(QLatin1String(".git"));
        }
        // set once, use everywhere
        // This is per project
        git.setWorkingDirectory(m_gitPath);
    }
}

void GitWidget::sendMessage(const QString &message, bool warn)
{
    // quickfix crash on startup
    if (!m_mainWin->activeView()) {
        return;
    }

    KTextEditor::Message *msg = new KTextEditor::Message(message, warn ? KTextEditor::Message::Warning : KTextEditor::Message::Positive);
    msg->setPosition(KTextEditor::Message::TopInView);
    msg->setAutoHide(warn ? 5000 : 3000);
    msg->setAutoHideMode(KTextEditor::Message::Immediate);
    msg->setView(m_mainWin->activeView());
    m_mainWin->activeView()->document()->postMessage(msg);
}

QProcess *GitWidget::gitprocess()
{
    return &git;
}

KTextEditor::MainWindow *GitWidget::mainWindow()
{
    return m_mainWin;
}

std::vector<GitWidget::TempFileViewPair> *GitWidget::tempFilesVector()
{
    return &m_filesOpenAtHEAD;
}

void GitWidget::getStatus(bool untracked, bool submodules)
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
    git.start(QProcess::ReadOnly);
}

void GitWidget::runGitCmd(const QStringList &args, const QString &i18error)
{
    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    connect(&git, &QProcess::finished, this, [this, i18error](int exitCode, QProcess::ExitStatus es) {
        // sever connection
        disconnect(&git, &QProcess::finished, nullptr, nullptr);
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18error + QStringLiteral("\n") + QString::fromUtf8(git.readAllStandardError()), true);
        } else {
            getStatus();
        }
    });
    git.setArguments(args);
    git.start(QProcess::ReadOnly);
}

void GitWidget::stage(const QStringList &files, bool)
{
    if (files.isEmpty()) {
        return;
    }

    auto args = QStringList{QStringLiteral("add"), QStringLiteral("-A"), QStringLiteral("--")};
    args.append(files);

    runGitCmd(args, i18n("Failed to stage file. Error:"));
}

void GitWidget::unstage(const QStringList &files)
{
    if (files.isEmpty()) {
        return;
    }

    // git reset -q HEAD --
    auto args = QStringList{QStringLiteral("reset"), QStringLiteral("-q"), QStringLiteral("HEAD"), QStringLiteral("--")};
    args.append(files);

    runGitCmd(args, i18n("Failed to unstage file. Error:"));
}

void GitWidget::discard(const QStringList &files)
{
    if (files.isEmpty()) {
        return;
    }
    // discard=>git checkout -q -- xx.cpp
    auto args = QStringList{QStringLiteral("checkout"), QStringLiteral("-q"), QStringLiteral("--")};
    args.append(files);

    runGitCmd(args, i18n("Failed to discard changes. Error:"));
}

void GitWidget::clean(const QStringList &files)
{
    if (files.isEmpty()) {
        return;
    }
    // discard=>git clean -q -f -- xx.cpp
    auto args = QStringList{QStringLiteral("clean"), QStringLiteral("-q"), QStringLiteral("-f"), QStringLiteral("--")};
    args.append(files);

    runGitCmd(args, i18n("Failed to remove. Error:"));
}

void GitWidget::openAtHEAD(const QString &file)
{
    if (file.isEmpty()) {
        return;
    }

    auto args = QStringList{QStringLiteral("show"), QStringLiteral("--textconv")};
    args.append(QStringLiteral(":") + file);
    git.setArguments(args);
    git.start(QProcess::ReadOnly);

    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    connect(&git, &QProcess::finished, this, [this, file](int exitCode, QProcess::ExitStatus es) {
        // sever connection
        disconnect(&git, &QProcess::finished, nullptr, nullptr);
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18n("Failed to open file at HEAD. Error:\n%1", QString::fromUtf8(git.readAllStandardError())), true);
        } else {
            std::unique_ptr<QTemporaryFile> f(new QTemporaryFile);
            QFileInfo fi(file);
            f->setFileTemplate(QString(QStringLiteral("XXXXXX - (HEAD) - %1").arg(fi.fileName())));
            if (!f->open()) {
                return;
            }

            auto xx = git.readAllStandardOutput();
            f->write(xx);
            f->flush();
            const QUrl tempFileUrl(QUrl::fromLocalFile(f->fileName()));
            auto v = m_mainWin->openUrl(tempFileUrl);
            if (!v || !v->document()) {
                return;
            }

            TempFileViewPair tfvp = std::make_pair(std::move(f), v);
            m_filesOpenAtHEAD.push_back(std::move(tfvp));

            // close temp on document close
            auto clearTemp = [this](KTextEditor::Document *document) {
                m_filesOpenAtHEAD.erase(std::remove_if(m_filesOpenAtHEAD.begin(),
                                                       m_filesOpenAtHEAD.end(),
                                                       [document](const GitWidget::TempFileViewPair &tf) {
                                                           if (tf.second && tf.second->document() == document) {
                                                               return true;
                                                           }
                                                           return false;
                                                       }),
                                        m_filesOpenAtHEAD.end());
            };
            connect(v->document(), &KTextEditor::Document::aboutToClose, this, clearTemp);
        }
    });

    git.setArguments(args);
    git.start(QProcess::ReadOnly);
}

void GitWidget::showDiff(const QString &file, bool staged)
{
    if (file.isEmpty()) {
        return;
    }

    auto args = QStringList{QStringLiteral("diff")};
    if (staged) {
        args.append(QStringLiteral("--staged"));
    }

    args.append(QStringLiteral("--"));
    args.append(file);

    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    connect(&git, &QProcess::finished, this, [this, file](int exitCode, QProcess::ExitStatus es) {
        // sever connection
        disconnect(&git, &QProcess::finished, nullptr, nullptr);
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18n("Failed to get Diff of file. Error:\n%1", QString::fromUtf8(git.readAllStandardError())), true);
        } else {
            std::unique_ptr<QTemporaryFile> f(new QTemporaryFile);
            QFileInfo fi(file);
            f->setFileTemplate(QString(QStringLiteral("XXXXXX %1.diff").arg(fi.fileName())));
            if (!f->open()) {
                return;
            }
            f->write(git.readAllStandardOutput());
            f->flush();
            const QUrl tempFileUrl(QUrl::fromLocalFile(f->fileName()));
            auto v = m_mainWin->openUrl(tempFileUrl);

            if (!v || !v->document()) {
                return;
            }

            TempFileViewPair tfvp = std::make_pair(std::move(f), v);
            m_filesOpenAtHEAD.push_back(std::move(tfvp));

            // close temp on document close
            auto clearTemp = [this](KTextEditor::Document *document) {
                m_filesOpenAtHEAD.erase(std::remove_if(m_filesOpenAtHEAD.begin(),
                                                       m_filesOpenAtHEAD.end(),
                                                       [document](const GitWidget::TempFileViewPair &tf) {
                                                           if (tf.second && tf.second->document() == document) {
                                                               return true;
                                                           }
                                                           return false;
                                                       }),
                                        m_filesOpenAtHEAD.end());
            };
            connect(v->document(), &KTextEditor::Document::aboutToClose, this, clearTemp);
        }
    });

    git.setArguments(args);
    git.start(QProcess::ReadOnly);
}

void GitWidget::launchExternalDiffTool(const QString &file, bool staged)
{
    if (file.isEmpty()) {
        return;
    }

    auto args = QStringList{QStringLiteral("difftool"), QStringLiteral("-y")};
    if (staged) {
        args.append(QStringLiteral("--staged"));
    }
    args.append(file);

    git.setArguments(args);
    git.start(QProcess::ReadOnly);
}

void GitWidget::commitChanges(const QString &msg, const QString &desc, bool signOff)
{
    auto args = QStringList{QStringLiteral("commit")};
    if (signOff) {
        args.append(QStringLiteral("-s"));
    }
    args.append(QStringLiteral("-m"));
    args.append(msg);
    if (!desc.isEmpty()) {
        args.append(QStringLiteral("-m"));
        args.append(desc);
    }

    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    connect(&git, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus es) {
        // sever connection
        disconnect(&git, &QProcess::finished, nullptr, nullptr);
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18n("Failed to commit.\n %1", QString::fromUtf8(git.readAllStandardError())), true);
        } else {
            m_commitMessage.clear();
            getStatus();
            sendMessage(i18n("Changes committed successfully."), false);
        }
    });
    git.setArguments(args);
    git.start(QProcess::ReadOnly);
}

void GitWidget::opencommitChangesDialog()
{
    if (m_model->stagedFiles().isEmpty()) {
        return sendMessage(i18n("Nothing to commit. Please stage your changes first."), true);
    }

    auto ciface = qobject_cast<KTextEditor::ConfigInterface *>(m_mainWin->activeView());
    QFont font;
    if (ciface) {
        font = ciface->configValue(QStringLiteral("font")).value<QFont>();
    } else {
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }

    GitCommitDialog dialog(m_commitMessage, font);
    dialog.setWindowFlags(Qt::WindowSystemMenuHint | Qt::WindowTitleHint);

    int res = dialog.exec();
    if (res == QDialog::Accepted) {
        if (dialog.subject().isEmpty()) {
            return sendMessage(i18n("Commit message cannot be empty."), true);
        }
        m_commitMessage = dialog.subject() + QStringLiteral("[[\n\n]]") + dialog.description();
        commitChanges(dialog.subject(), dialog.description(), dialog.signoff());
    }
}

void GitWidget::gitStatusReady(int exit, QProcess::ExitStatus status)
{
    // sever connection
    disconnect(&git, &QProcess::finished, nullptr, nullptr);

    if (status != QProcess::NormalExit || exit != 0) {
        // we don't want to disturb non-git users
        // sendMessage(i18n("Failed to get git-status. Error: %1", QString::fromUtf8(git.readAllStandardError())), true);
        return;
    }

    QByteArray s = git.readAllStandardOutput();
    auto future = QtConcurrent::run(&GitUtils::parseStatus, s);
    m_gitStatusWatcher.setFuture(future);
}

void GitWidget::hideEmptyTreeNodes()
{
    const auto emptyRows = m_model->emptyRows();
    m_treeView->expand(m_model->getModelIndex((GitStatusModel::NodeStage)));
    // 1 because "Staged" will always be visible
    for (int i = 1; i < 4; ++i) {
        if (emptyRows.contains(i)) {
            m_treeView->setRowHidden(i, QModelIndex(), true);
        } else {
            m_treeView->setRowHidden(i, QModelIndex(), false);
            if (i != GitStatusModel::NodeUntrack) {
                m_treeView->expand(m_model->getModelIndex((GitStatusModel::ItemType)i));
            }
        }
    }

    m_treeView->resizeColumnToContents(0);
    m_treeView->resizeColumnToContents(1);
}

void GitWidget::parseStatusReady()
{
    GitUtils::GitParsedStatus s = m_gitStatusWatcher.result();
    m_model->addItems(std::move(s));

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

void GitWidget::buildMenu()
{
    m_gitMenu = new QMenu(this);
    m_gitMenu->addAction(i18n("Refresh"), this, [this] {
        if (m_project) {
            getStatus();
        }
    });
    m_gitMenu->addAction(i18n("Checkout Branch"), this, [this] {
        BranchesDialog bd(this, m_mainWin, m_project->baseDir());
        bd.openDialog();
    });

    m_gitMenu->addAction(i18n("Stash"))->setMenu(stashMenu());
}

QMenu *GitWidget::stashMenu()
{
    QMenu *menu = new QMenu(this);
    auto stashAct = menu->addAction(i18n("Stash"));
    auto popLastAct = menu->addAction(i18n("Pop Last Stash"));
    auto popAct = menu->addAction(i18n("Pop Stash"));
    auto applyLastAct = menu->addAction(i18n("Apply Last Stash"));
    auto stashKeepStagedAct = menu->addAction(i18n("Stash (Keep Staged)"));
    auto stashUAct = menu->addAction(i18n("Stash (Include Untracked)"));
    auto applyStashAct = menu->addAction(i18n("Apply Stash"));
    auto dropAct = menu->addAction(i18n("Drop Stash"));
    auto showStashAct = menu->addAction(i18n("Show Stash Content"));

    connect(stashAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::Stash);
    });
    connect(stashUAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::StashUntrackIncluded);
    });
    connect(stashKeepStagedAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::StashKeepIndex);
    });
    connect(popAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::StashPop);
    });
    connect(applyStashAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::StashApply);
    });
    connect(dropAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::StashDrop);
    });
    connect(popLastAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::StashPopLast);
    });
    connect(applyLastAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::StashApplyLast);
    });
    connect(showStashAct, &QAction::triggered, this, [this] {
        StashDialog stashDialog(this, m_mainWin);
        stashDialog.openDialog(StashDialog::ShowStashContent);
    });

    return menu;
}

static KMessageBox::ButtonCode confirm(GitWidget *_this, const QString &text)
{
    return KMessageBox::questionYesNo(_this, text, {}, KStandardGuiItem::yes(), KStandardGuiItem::no(), {}, KMessageBox::Dangerous);
}

void GitWidget::treeViewContextMenuEvent(QContextMenuEvent *e)
{
    if (auto selModel = m_treeView->selectionModel()) {
        if (selModel->selectedRows().count() > 1) {
            return selectedContextMenu(e);
        }
    }

    auto idx = m_model->index(m_treeView->currentIndex().row(), 0, m_treeView->currentIndex().parent());
    auto type = idx.data(GitStatusModel::TreeItemType);

    if (type == GitStatusModel::NodeChanges || type == GitStatusModel::NodeUntrack) {
        QMenu menu;
        auto stageAct = menu.addAction(i18n("Stage All"));
        bool untracked = type == GitStatusModel::NodeUntrack;
        auto discardAct = untracked ? menu.addAction(i18n("Remove All")) : menu.addAction(i18n("Discard All"));
        auto ignoreAct = untracked ? menu.addAction(i18n("Open .gitignore")) : nullptr;

        // get files
        const QVector<GitUtils::StatusItem> &files = untracked ? m_model->untrackedFiles() : m_model->changedFiles();
        QStringList filesList;
        filesList.reserve(files.size());
        for (const auto &file : files) {
            filesList.append(file.file);
        }

        // execute action
        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
        if (act == stageAct) {
            stage(filesList, type == GitStatusModel::NodeUntrack);
        } else if (act == discardAct && !untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to remove these files?"));
            if (ret == KMessageBox::Yes) {
                discard(filesList);
            }
        } else if (act == discardAct && untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to discard all changes?"));
            if (ret == KMessageBox::Yes) {
                clean(filesList);
            }
        } else if (untracked && act == ignoreAct) {
            const auto files = m_project->files();
            const auto it = std::find_if(files.cbegin(), files.cend(), [](const QString &s) {
                if (s.contains(QStringLiteral(".gitignore"))) {
                    return true;
                }
                return false;
            });
            if (it != files.cend()) {
                m_mainWin->openUrl(QUrl::fromLocalFile(*it));
            }
        }
    } else if (type == GitStatusModel::NodeFile) {
        QMenu menu;
        bool staged = idx.internalId() == GitStatusModel::NodeStage;
        bool untracked = idx.internalId() == GitStatusModel::NodeUntrack;

        auto openFile = menu.addAction(i18n("Open file"));
        auto showDiffAct = untracked ? nullptr : menu.addAction(i18n("Show raw diff"));
        auto launchDifftoolAct = untracked ? nullptr : menu.addAction(i18n("Show in external git diff tool"));
        auto openAtHead = untracked ? nullptr : menu.addAction(i18n("Open at HEAD"));
        auto stageAct = staged ? menu.addAction(i18n("Unstage file")) : menu.addAction(i18n("Stage file"));
        auto discardAct = untracked ? menu.addAction(i18n("Remove")) : menu.addAction(i18n("Discard"));

        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
        const QString file = m_gitPath + idx.data(GitStatusModel::FileNameRole).toString();
        if (act == stageAct) {
            if (staged) {
                return unstage({file});
            }
            return stage({file});
        } else if (act == discardAct && !untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to discard the changes in this file?"));
            if (ret == KMessageBox::Yes) {
                discard({file});
            }
        } else if (act == openAtHead && !untracked) {
            openAtHEAD(idx.data(GitStatusModel::FileNameRole).toString());
        } else if (act == showDiffAct && !untracked) {
            showDiff(file, staged);
        } else if (act == discardAct && untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to remove this file?"));
            if (ret == KMessageBox::Yes) {
                clean({file});
            }
        } else if (act == launchDifftoolAct) {
            launchExternalDiffTool(idx.data(GitStatusModel::FileNameRole).toString(), staged);
        } else if (act == openFile) {
            m_mainWin->openUrl(QUrl::fromLocalFile(file));
        }
    } else if (type == GitStatusModel::NodeStage) {
        QMenu menu;
        auto stage = menu.addAction(i18n("Unstage All"));
        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));

        // git reset -q HEAD --
        if (act == stage) {
            const QVector<GitUtils::StatusItem> &files = m_model->stagedFiles();
            QStringList filesList;
            filesList.reserve(filesList.size() + files.size());
            for (const auto &file : files) {
                filesList.append(file.file);
            }
            unstage(filesList);
        }
    }
}

void GitWidget::selectedContextMenu(QContextMenuEvent *e)
{
    QStringList files;

    bool selectionHasStageItems = false;
    bool selectionHasChangedItems = false;

    if (auto selModel = m_treeView->selectionModel()) {
        const auto idxList = selModel->selectedIndexes();
        for (const auto &idx : idxList) {
            if (idx.internalId() == GitStatusModel::NodeStage) {
                selectionHasStageItems = true;
            } else if (!idx.parent().isValid()) {
                // can't allow main nodes to be selected
                return;
            } else {
                selectionHasChangedItems = true;
            }
            files.append(idx.data(GitStatusModel::FileNameRole).toString());
        }
    }

    // cant allow both
    if (selectionHasChangedItems && selectionHasStageItems) {
        return;
    }

    QMenu menu;
    auto stageAct = selectionHasStageItems ? menu.addAction(i18n("Unstage Selected Files")) : menu.addAction(i18n("Stage Selected Files"));
    auto execAct = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));

    if (execAct == stageAct) {
        if (selectionHasChangedItems) {
            stage(files);
        } else {
            unstage(files);
        }
    }
}
