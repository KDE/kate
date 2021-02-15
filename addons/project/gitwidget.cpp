#include "gitwidget.h"
#include "gitstatusmodel.h"
#include "kateproject.h"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QEvent>
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

#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/View>

GitWidget::GitWidget(KateProject *project, QWidget *parent, KTextEditor::MainWindow *mainWindow)
    : QWidget(parent)
    , m_project(project)
    , m_mainWin(mainWindow)
{
    m_commitBtn = new QPushButton(this);
    m_treeView = new QTreeView(this);

    buildMenu();
    m_menuBtn = new QToolButton(this);
    m_menuBtn->setMenu(m_gitMenu);
    m_menuBtn->setArrowType(Qt::NoArrow);
    m_menuBtn->setStyleSheet(QStringLiteral("QToolButton::menu-indicator{ image: none; }"));
    connect(m_menuBtn, &QToolButton::clicked, this, [this](bool) {
        m_menuBtn->showMenu();
    });

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

    m_treeView->setUniformRowHeights(true);
    m_treeView->setHeaderHidden(true);
    m_treeView->setSelectionMode(QTreeView::ExtendedSelection);
    m_treeView->setModel(m_model);
    m_treeView->installEventFilter(this);

    setLayout(layout);

    connect(&m_gitStatusWatcher, &QFutureWatcher<GitUtils::GitParsedStatus>::finished, this, &GitWidget::parseStatusReady);
    connect(m_commitBtn, &QPushButton::clicked, this, &GitWidget::opencommitChangesDialog);
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
    git.setProgram(QStringLiteral("git"));
    git.setWorkingDirectory(m_project->baseDir());
    git.start();
}

void GitWidget::stage(const QStringList &files, bool)
{
    if (files.isEmpty()) {
        return;
    }

    auto args = QStringList{QStringLiteral("add"), QStringLiteral("-A"), QStringLiteral("--")};
    args.append(files);

    git.setWorkingDirectory(m_project->baseDir());
    git.setProgram(QStringLiteral("git"));
    git.setArguments(args);
    git.start();

    if (git.waitForStarted() && git.waitForFinished(-1)) {
        getStatus();
    }
}

void GitWidget::unstage(const QStringList &files)
{
    if (files.isEmpty()) {
        return;
    }

    // git reset -q HEAD --
    auto args = QStringList{QStringLiteral("reset"), QStringLiteral("-q"), QStringLiteral("HEAD"), QStringLiteral("--")};
    args.append(files);

    git.setWorkingDirectory(m_project->baseDir());
    git.setProgram(QStringLiteral("git"));
    git.setArguments(args);
    git.start();

    if (git.waitForStarted() && git.waitForFinished(-1)) {
        getStatus();
    }
}

void GitWidget::discard(const QStringList &files)
{
    if (files.isEmpty()) {
        return;
    }
    // discard=>git checkout -q -- xx.cpp
    auto args = QStringList{QStringLiteral("checkout"), QStringLiteral("-q"), QStringLiteral("--")};
    args.append(files);

    git.setWorkingDirectory(m_project->baseDir());
    git.setProgram(QStringLiteral("git"));
    git.setArguments(args);
    git.start();

    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    connect(&git, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
        if (exitCode > 0) {
            sendMessage(i18n("Failed to discard changes. Error:\n%1", QString::fromUtf8(git.readAllStandardError())), true);
        } else {
            getStatus();
        }
    });
}

void GitWidget::openAtHEAD(const QString &file)
{
    if (file.isEmpty()) {
        return;
    }

    auto args = QStringList{QStringLiteral("show"), QStringLiteral("--textconv")};
    args.append(QStringLiteral(":") + file);

    git.setWorkingDirectory(m_project->baseDir());
    git.setProgram(QStringLiteral("git"));
    git.setArguments(args);
    git.start();

    disconnect(&git, &QProcess::finished, nullptr, nullptr);
    connect(&git, &QProcess::finished, this, [this, file](int exitCode, QProcess::ExitStatus) {
        if (exitCode > 0) {
            sendMessage(i18n("Failed to open file at HEAD. Error:\n%1", QString::fromUtf8(git.readAllStandardError())), true);
        } else {
            std::unique_ptr<QTemporaryFile> f(new QTemporaryFile);
            f->setFileTemplate(QString(QStringLiteral("XXXXXX (HEAD)") + file));
            if (f->open()) {
                f->write(git.readAll());
                f->flush();
                const QUrl tempFileUrl(QUrl::fromLocalFile(f->fileName()));
                auto v = m_mainWin->openUrl(tempFileUrl);
                if (v && v->document()) {
                    TempFileViewPair tfvp = std::make_pair(std::move(f), v);
                    m_filesOpenAtHEAD.push_back(std::move(tfvp));

                    // close temp on document close
                    auto clearTemp = [this](KTextEditor::Document *document) {
                        KTextEditor::Editor::instance();
                        for (auto i = m_filesOpenAtHEAD.begin(); i != m_filesOpenAtHEAD.end(); ++i) {
                            if (i->second->document() == document) {
                                qWarning() << "CLOSING FILE";
                                m_filesOpenAtHEAD.erase(i);
                                break;
                            }
                        }
                    };
                    connect(v->document(), &KTextEditor::Document::aboutToClose, this, clearTemp);
                }
            }
        }
    });
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
            getStatus();
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
    dialog.open();

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
    auto future = QtConcurrent::run(&GitUtils::parseStatus, s);
    m_gitStatusWatcher.setFuture(future);
}

void GitWidget::hideEmptyTreeNodes()
{
    const auto emptyRows = m_model->emptyRows();
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
    m_gitMenu->addAction(i18n("Checkout Branch"), this, &GitWidget::checkoutBranch);
}

void GitWidget::treeViewContextMenuEvent(QContextMenuEvent *e)
{
    //    git show --textconv :App.js
    // current diff => git show --textconv HEAD:App.js
    if (auto selModel = m_treeView->selectionModel()) {
        if (selModel->selectedIndexes().count() > 1) {
            return selectedContextMenu(e);
        }
    }

    auto idx = m_model->index(m_treeView->currentIndex().row(), 0, m_treeView->currentIndex().parent());
    auto type = idx.data(GitStatusModel::TreeItemType);

    if (type == GitStatusModel::NodeChanges || type == GitStatusModel::NodeUntrack) {
        QMenu menu;
        auto stageAct = menu.addAction(i18n("Stage All"));
        bool untracked = type == GitStatusModel::NodeUntrack;
        auto discardAct = untracked ? nullptr : menu.addAction(i18n("Discard All"));

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
            discard(filesList);
        }
    } else if (type == GitStatusModel::NodeFile) {
        QMenu menu;
        bool unstaging = idx.internalId() == GitStatusModel::NodeStage;
        bool untracked = idx.internalId() == GitStatusModel::NodeUntrack;

        auto stageAct = unstaging ? menu.addAction(i18n("Unstage file")) : menu.addAction(i18n("Stage file"));
        auto discardAct = untracked ? nullptr : menu.addAction(i18n("Discard"));
        auto openAtHead = untracked ? nullptr : menu.addAction(i18n("Open at HEAD"));

        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
        const QString file = QString(m_project->baseDir() + QStringLiteral("/") + idx.data().toString());
        if (act == stageAct) {
            if (unstaging) {
                return unstage({file});
            }
            return stage({file});
        } else if (act == discardAct && !untracked) {
            discard({file});
        } else if (act == openAtHead && !untracked) {
            openAtHEAD(idx.data().toString());
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
            files.append(idx.data().toString());
        }
    }

    // cant allow both
    if (selectionHasChangedItems && selectionHasStageItems)
        return;

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
