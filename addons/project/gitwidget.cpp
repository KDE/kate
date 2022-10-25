/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gitwidget.h"
#include "branchcheckoutdialog.h"
#include "branchdeletedialog.h"
#include "branchesdialog.h"
#include "comparebranchesview.h"
#include "diffparams.h"
#include "gitcommitdialog.h"
#include "gitstatusmodel.h"
#include "kateproject.h"
#include "kateprojectplugin.h"
#include "kateprojectpluginview.h"
#include "ktexteditor_utils.h"
#include "pushpulldialog.h"
#include "stashdialog.h"

#include <gitprocess.h>

#include <KColorScheme>
#include <QContextMenuEvent>
#include <QDialog>
#include <QEvent>
#include <QHeaderView>
#include <QInputMethodEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QtConcurrentRun>

#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <kwidgetsaddons_version.h>

#include <KFuzzyMatcher>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/View>

class NumStatStyle final : public QStyledItemDelegate
{
public:
    NumStatStyle(QObject *parent, KateProjectPlugin *p)
        : QStyledItemDelegate(parent)
        , m_plugin(p)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (!m_plugin->showGitStatusWithNumStat()) {
            return QStyledItemDelegate::paint(painter, option, index);
        }

        const auto strs = index.data().toString().split(QLatin1Char(' '));
        if (strs.count() < 3) {
            return QStyledItemDelegate::paint(painter, option, index);
        }

        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);
        painter->save();

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        const QString add = strs.at(0) + QStringLiteral(" ");
        const QString sub = strs.at(1) + QStringLiteral(" ");
        const QString Status = strs.at(2);

        int ha = option.fontMetrics.horizontalAdvance(add);
        int hs = option.fontMetrics.horizontalAdvance(sub);
        int hS = option.fontMetrics.horizontalAdvance(Status);

        QRect r = option.rect;
        int mw = r.width() - (ha + hs + hS);
        r.setX(r.x() + mw);

        KColorScheme c;
        const auto red = c.shade(c.foreground(KColorScheme::NegativeText).color(), KColorScheme::MidlightShade, 1);
        const auto green = c.shade(c.foreground(KColorScheme::PositiveText).color(), KColorScheme::MidlightShade, 1);

        painter->setPen(green);
        painter->drawText(r, Qt::AlignVCenter, add);
        r.setX(r.x() + ha);

        painter->setPen(red);
        painter->drawText(r, Qt::AlignVCenter, sub);
        r.setX(r.x() + hs);

        painter->setPen(index.data(Qt::ForegroundRole).value<QColor>());
        painter->drawText(r, Qt::AlignVCenter, Status);

        painter->restore();
    }

private:
    KateProjectPlugin *m_plugin;
};

class StatusProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    static bool isTopLevel(const QModelIndex &idx)
    {
        return !idx.isValid();
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &parent) const override
    {
        // top level node
        auto index = sourceModel()->index(sourceRow, 0, parent);
        if (isTopLevel(parent)) {
            // Staged are always visible
            if (index.row() == GitStatusModel::ItemType::NodeStage)
                return true;

            // otherwise visible only if rowCount > 0
            return sourceModel()->rowCount(index) > 0;
        }

        if (!index.isValid()) {
            return false;
        }

        // no pattern => everything visible
        if (m_text.isEmpty()) {
            return true;
        }

        const QString file = index.data().toString();
        // not using score atm
        return KFuzzyMatcher::matchSimple(m_text, file);
    }

    void setFilterText(const QString &text)
    {
        beginResetModel();
        m_text = text;
        endResetModel();
    }

private:
    QString m_text;
};

class GitWidgetTreeView : public QTreeView
{
public:
    GitWidgetTreeView(QWidget *parent)
        : QTreeView(parent)
    {
    }

    // we want no branches!
    void drawBranches(QPainter *, const QRect &, const QModelIndex &) const override
    {
    }
};

static QToolButton *toolButton(Qt::ToolButtonStyle t = Qt::ToolButtonIconOnly)
{
    auto tb = new QToolButton;
    tb->setAutoRaise(true);
    tb->setToolButtonStyle(t);
    tb->setSizePolicy(QSizePolicy::Minimum, tb->sizePolicy().verticalPolicy());
    return tb;
}

static QToolButton *toolButton(const QString &icon, const QString &tooltip, const QString &text = QString(), Qt::ToolButtonStyle t = Qt::ToolButtonIconOnly)
{
    auto tb = toolButton(t);
    tb->setToolTip(tooltip);
    tb->setIcon(QIcon::fromTheme(icon));
    tb->setText(text);
    return tb;
}

GitWidget::GitWidget(KateProject *project, KTextEditor::MainWindow *mainWindow, KateProjectPluginView *pluginView)
    : m_project(project)
    , m_mainWin(mainWindow)
    , m_pluginView(pluginView)
    , m_mainView(new QWidget(this))
    , m_stackWidget(new QStackedWidget(this))
{
    // We init delayed when the widget will be shown
}

void GitWidget::showEvent(QShowEvent *e)
{
    init();
    QWidget::showEvent(e);
}

void GitWidget::init()
{
    if (m_initialized) {
        return;
    }
    m_initialized = true;

    setDotGitPath();

    m_treeView = new GitWidgetTreeView(this);
    auto ac = m_pluginView->actionCollection();

    buildMenu(ac);
    m_menuBtn = toolButton(QStringLiteral("application-menu"), QString());
    m_menuBtn->setMenu(m_gitMenu);
    m_menuBtn->setArrowType(Qt::NoArrow);
    m_menuBtn->setStyleSheet(QStringLiteral("QToolButton::menu-indicator{ image: none; }"));
    connect(m_menuBtn, &QToolButton::clicked, this, [this](bool) {
        m_menuBtn->showMenu();
    });

    const QString &commitText = i18n("Commit");
    const QIcon &commitIcon = QIcon::fromTheme(QStringLiteral("vcs-commit"));
    m_commitBtn = toolButton(Qt::ToolButtonTextBesideIcon);
    m_commitBtn->setIcon(commitIcon);
    m_commitBtn->setText(commitText);
    m_commitBtn->setToolTip(commitText);
    m_commitBtn->setMinimumHeight(16);

    const QString &pushText = i18n("Git Push");
    m_pushBtn = toolButton();
    auto a = ac->addAction(QStringLiteral("vcs_push"), this, [this]() {
        PushPullDialog ppd(m_mainWin, m_activeGitDirPath);
        connect(&ppd, &PushPullDialog::runGitCommand, this, &GitWidget::runPushPullCmd);
        ppd.openDialog(PushPullDialog::Push);
    });
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-push")));
    a->setText(pushText);
    a->setToolTip(pushText);
    ac->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+T, P"), QKeySequence::PortableText));
    m_pushBtn->setDefaultAction(a);

    const QString &pullText = i18n("Git Pull");
    m_pullBtn = toolButton(QStringLiteral("vcs-pull"), pullText);
    a = ac->addAction(QStringLiteral("vcs_pull"), this, [this]() {
        PushPullDialog ppd(m_mainWin, m_activeGitDirPath);
        connect(&ppd, &PushPullDialog::runGitCommand, this, &GitWidget::runPushPullCmd);
        ppd.openDialog(PushPullDialog::Pull);
    });
    ac->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+T, U"), QKeySequence::PortableText));
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-pull")));
    a->setText(pullText);
    a->setToolTip(pullText);
    m_pullBtn->setDefaultAction(a);

    m_cancelBtn = toolButton(QStringLiteral("dialog-cancel"), i18n("Cancel Operation"));
    m_cancelBtn->setHidden(true);
    connect(m_cancelBtn, &QToolButton::clicked, this, [this] {
        if (m_cancelHandle) {
            // we don't want error occurred, this is intentional
            disconnect(m_cancelHandle, &QProcess::errorOccurred, nullptr, nullptr);
            const auto args = m_cancelHandle->arguments();
            m_cancelHandle->kill();
            sendMessage(QStringLiteral("git ") + args.join(QLatin1Char(' ')) + i18n(" canceled."), false);
            hideCancel();
        }
    });

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *btnsLayout = new QHBoxLayout;
    btnsLayout->setContentsMargins(0, 0, 0, 0);

    for (auto *btn : {m_commitBtn, m_cancelBtn, m_pushBtn, m_pullBtn, m_menuBtn}) {
        btnsLayout->addWidget(btn);
    }
    btnsLayout->setStretch(0, 1);

    layout->addLayout(btnsLayout);
    layout->addWidget(m_treeView);

    m_filterLineEdit = new QLineEdit(this);
    m_filterLineEdit->setPlaceholderText(i18n("Filter..."));
    layout->addWidget(m_filterLineEdit);

    m_model = new GitStatusModel(this);
    auto proxy = new StatusProxyModel(this);
    proxy->setSourceModel(m_model);

    connect(m_filterLineEdit, &QLineEdit::textChanged, proxy, &StatusProxyModel::setFilterText);
    connect(m_filterLineEdit, &QLineEdit::textChanged, m_treeView, &QTreeView::expandAll);

    m_treeView->setUniformRowHeights(true);
    m_treeView->setHeaderHidden(true);
    m_treeView->setSelectionMode(QTreeView::ExtendedSelection);
    m_treeView->setModel(proxy);
    m_treeView->installEventFilter(this);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setAllColumnsShowFocus(true);
    m_treeView->expandAll();

    if (m_treeView->style()) {
        auto indent = m_treeView->style()->pixelMetric(QStyle::PM_TreeViewIndentation, nullptr, m_treeView);
        m_treeView->setIndentation(indent / 4);
    }

    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_treeView->setItemDelegateForColumn(1, new NumStatStyle(this, m_pluginView->plugin()));

    // our main view - status view + btns
    m_mainView->setLayout(layout);

    a = ac->addAction(QStringLiteral("vcs_commit"), this, [this] {
        openCommitChangesDialog();
        slotUpdateStatus();
    });
    ac->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+T, K"), QKeySequence::PortableText));
    a->setText(commitText);
    a->setToolTip(commitText);
    a->setIcon(commitIcon);

    connect(&m_gitStatusWatcher, &QFutureWatcher<GitUtils::GitParsedStatus>::finished, this, &GitWidget::parseStatusReady);
    connect(m_commitBtn, &QPushButton::clicked, this, &GitWidget::openCommitChangesDialog);
    // This may not needed anylonger, but we do it anyway, just to be on the save side, see e239cb310
    connect(m_commitBtn, &QPushButton::pressed, this, &GitWidget::slotUpdateStatus);

    // single / double click
    connect(m_treeView, &QTreeView::clicked, this, &GitWidget::treeViewSingleClicked);
    connect(m_treeView, &QTreeView::doubleClicked, this, &GitWidget::treeViewDoubleClicked);

    m_stackWidget->addWidget(m_mainView);

    // This Widget's layout
    setLayout(new QVBoxLayout);
    this->layout()->addWidget(m_stackWidget);
    this->layout()->setContentsMargins(0, 0, 0, 0);

    // Setup update protection
    m_updateTrigger.setSingleShot(true);
    m_updateTrigger.setInterval(500);
    connect(&m_updateTrigger, &QTimer::timeout, this, &GitWidget::slotUpdateStatus);
    slotUpdateStatus();

    connect(m_mainWin, &KTextEditor::MainWindow::viewChanged, this, &GitWidget::setActiveGitDir);
}

GitWidget::~GitWidget()
{
    if (m_cancelHandle) {
        m_cancelHandle->kill();
    }

    // if there are any living processes, disconnect them now before gitwidget get destroyed
    for (QObject *child : children()) {
        QProcess *p = qobject_cast<QProcess *>(child);
        if (p) {
            disconnect(p, nullptr, nullptr, nullptr);
        }
    }
}

void GitWidget::setDotGitPath()
{
    const auto dotGitPath = getRepoBasePath(m_project->baseDir());
    if (!dotGitPath.has_value()) {
        QTimer::singleShot(1, this, [this] {
            sendMessage(i18n("Failed to find .git directory for '%1', things may not work correctly", m_project->baseDir()), false);
        });
        m_topLevelGitPath = m_project->baseDir();
        return;
    }

    m_topLevelGitPath = dotGitPath.value();
    m_activeGitDirPath = m_topLevelGitPath;

    QMetaObject::invokeMethod(this, &GitWidget::setSubmodulesPaths, Qt::QueuedConnection);
}

void GitWidget::setSubmodulesPaths()
{
    // git submodule foreach --recursive -q git rev-parse --show-toplevel
    QStringList args{QStringLiteral("submodule"),
                     QStringLiteral("foreach"),
                     QStringLiteral("--recursive"),
                     QStringLiteral("-q"),
                     QStringLiteral("git"),
                     QStringLiteral("rev-parse"),
                     QStringLiteral("--show-toplevel")};
    auto git = gitp(args);
    startHostProcess(*git, QProcess::ReadOnly);
    connect(git, &QProcess::finished, this, [this, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            // no error on status failure
            sendMessage(QString::fromUtf8(git->readAllStandardError()), true);
        } else {
            QString s = QString::fromUtf8(git->readAllStandardOutput());
            static const QRegularExpression lineEndings(QStringLiteral("\r\n?"));
            s.replace(lineEndings, QStringLiteral("\n"));
            m_submodulePaths = s.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (auto &p : m_submodulePaths) {
                if (!p.endsWith(QLatin1Char('/'))) {
                    p.append(QLatin1Char('/'));
                }
            }
            // Sort by size so that we can early out on matching paths later.
            std::sort(m_submodulePaths.begin(), m_submodulePaths.end(), [](const QString &l, const QString &r) {
                return l.size() > r.size();
            });
            setActiveGitDir();
        }
        git->deleteLater();
    });
}

void GitWidget::setActiveGitDir()
{
    // No submodules
    if (m_submodulePaths.size() <= 1) {
        return;
    }

    auto av = m_mainWin->activeView();
    if (!av || !av->document() || !av->document()->url().isValid()) {
        return;
    }

    int idx = 0;
    int activeSubmoduleIdx = -1;
    const QString path = av->document()->url().toLocalFile();
    for (const auto &submodulePath : std::as_const(m_submodulePaths)) {
        if (path.startsWith(submodulePath)) {
            activeSubmoduleIdx = idx;
            break;
        }
        idx++;
    }

    if (activeSubmoduleIdx == -1) {
        if (m_activeGitDirPath != m_topLevelGitPath) {
            m_activeGitDirPath = m_topLevelGitPath;
            updateStatus();
        }
        return;
    }

    // Only trigger update if this is a different path
    auto foundPath = m_submodulePaths.at(activeSubmoduleIdx);
    if (foundPath != m_activeGitDirPath) {
        m_activeGitDirPath = foundPath;
        updateStatus();
    }
}

void GitWidget::sendMessage(const QString &plainText, bool warn)
{
    // use generic output view
    QVariantMap genericMessage;
    genericMessage.insert(QStringLiteral("type"), warn ? QStringLiteral("Error") : QStringLiteral("Info"));
    genericMessage.insert(QStringLiteral("category"), i18n("Git"));
    genericMessage.insert(QStringLiteral("categoryIcon"), gitIcon());
    genericMessage.insert(QStringLiteral("text"), plainText);
    Utils::showMessage(genericMessage, mainWindow());
}

KTextEditor::MainWindow *GitWidget::mainWindow()
{
    return m_mainWin;
}

QProcess *GitWidget::gitp(const QStringList &arguments)
{
    auto git = new QProcess(this);
    setupGitProcess(*git, m_activeGitDirPath, arguments);
    connect(git, &QProcess::errorOccurred, this, [this, git](QProcess::ProcessError pe) {
        // git program missing is not an error
        sendMessage(git->errorString(), pe != QProcess::FailedToStart);
        git->deleteLater();
    });
    return git;
}

void GitWidget::updateStatus()
{
    if (m_initialized) {
        m_updateTrigger.start();
    }
}

void GitWidget::slotUpdateStatus()
{
    if (!isVisible()) {
        return; // No need to update
    }

    const auto args = QStringList{QStringLiteral("status"), QStringLiteral("-z"), QStringLiteral("-u"), QStringLiteral("--ignore-submodules")};

    auto git = gitp(args);
    connect(git, &QProcess::finished, this, [this, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            // no error on status failure
            // sendMessage(QString::fromUtf8(git->readAllStandardError()), true);
        } else {
            const bool withNumStat = m_pluginView->plugin()->showGitStatusWithNumStat();
            auto future = QtConcurrent::run(GitUtils::parseStatus, git->readAllStandardOutput(), withNumStat, m_activeGitDirPath);
            m_gitStatusWatcher.setFuture(future);
        }
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
}

void GitWidget::runGitCmd(const QStringList &args, const QString &i18error)
{
    auto git = gitp(args);
    connect(git, &QProcess::finished, this, [this, i18error, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18error + QStringLiteral(": ") + QString::fromUtf8(git->readAllStandardError()), true);
        } else {
            updateStatus();
        }
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
}

void GitWidget::runPushPullCmd(const QStringList &args)
{
    auto git = gitp(args);
    git->setProcessChannelMode(QProcess::MergedChannels);

    connect(git, &QProcess::finished, this, [this, args, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(QStringLiteral("git ") + args.first() + i18n(" error: %1", QString::fromUtf8(git->readAll())), true);
        } else {
            auto gargs = args;
            gargs.push_front(QStringLiteral("git"));
            QString cmd = gargs.join(QStringLiteral(" "));
            QString out = QString::fromUtf8(git->readAll());
            sendMessage(i18n("\"%1\" executed successfully: %2", cmd, out), false);
            updateStatus();
        }
        hideCancel();
        git->deleteLater();
    });

    enableCancel(git);
    startHostProcess(*git, QProcess::ReadOnly);
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
    auto git = gitp(args);
    startHostProcess(*git, QProcess::ReadOnly);

    connect(git, &QProcess::finished, this, [this, file, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18n("Failed to open file at HEAD: %1", QString::fromUtf8(git->readAllStandardError())), true);
        } else {
            auto view = m_mainWin->openUrl(QUrl());
            if (view) {
                view->document()->setText(QString::fromUtf8(git->readAllStandardOutput()));
                auto mode = KTextEditor::Editor::instance()->repository().definitionForFileName(file).name();
                view->document()->setHighlightingMode(mode);
                view->document()->setModified(false); // no save file dialog when closing
            }
        }
        git->deleteLater();
    });

    git->setArguments(args);
    startHostProcess(*git, QProcess::ReadOnly);
}

void GitWidget::showDiff(const QString &file, bool staged)
{
    auto args = QStringList{QStringLiteral("diff")};
    if (staged) {
        args.append(QStringLiteral("--staged"));
    }

    if (!file.isEmpty()) {
        args.append(QStringLiteral("--"));
        args.append(file);
    }

    auto git = gitp(args);
    connect(git, &QProcess::finished, this, [this, file, staged, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18n("Failed to get Diff of file: %1", QString::fromUtf8(git->readAllStandardError())), true);
        } else {
            DiffParams d;
            d.srcFile = file;
            d.workingDir = m_activeGitDirPath;
            d.arguments = git->arguments();
            d.flags.setFlag(DiffParams::Flag::ShowStage, !staged);
            d.flags.setFlag(DiffParams::Flag::ShowUnstage, staged);
            d.flags.setFlag(DiffParams::Flag::ShowDiscard, !staged);
            // When file is empty, we are showing diff of multiple file usually
            const bool showfile = file.isEmpty() && (staged ? m_model->stagedFiles().size() > 1 : m_model->changedFiles().size() > 1);
            d.flags.setFlag(DiffParams::Flag::ShowFileName, showfile);
            Utils::showDiff(git->readAllStandardOutput(), d, mainWindow());
        }
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
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

    QProcess git;
    if (setupGitProcess(git, m_activeGitDirPath, args)) {
        git.startDetached();
    }
}

void GitWidget::commitChanges(const QString &msg, const QString &desc, bool signOff, bool amend)
{
    auto args = QStringList{QStringLiteral("commit")};

    if (amend) {
        args.append(QStringLiteral("--amend"));
    }

    if (signOff) {
        args.append(QStringLiteral("-s"));
    }

    args.append(QStringLiteral("-m"));
    args.append(msg);
    if (!desc.isEmpty()) {
        args.append(QStringLiteral("-m"));
        args.append(desc);
    }

    auto git = gitp(args);

    connect(git, &QProcess::finished, this, [this, git](int exitCode, QProcess::ExitStatus es) {
        if (es != QProcess::NormalExit || exitCode != 0) {
            sendMessage(i18n("Failed to commit: %1", QString::fromUtf8(git->readAllStandardError())), true);
        } else {
            m_commitMessage.clear();
            updateStatus();
            sendMessage(i18n("Changes committed successfully."), false);
        }
        git->deleteLater();
    });
    startHostProcess(*git, QProcess::ReadOnly);
}

void GitWidget::openCommitChangesDialog(bool amend)
{
    if (!amend && m_model->stagedFiles().isEmpty()) {
        return sendMessage(i18n("Nothing to commit. Please stage your changes first."), true);
    }

    GitCommitDialog *dialog = new GitCommitDialog(m_commitMessage, this);

    if (amend) {
        dialog->setAmendingCommit();
    }

    connect(dialog, &QDialog::finished, this, [this, dialog](int res) {
        dialog->deleteLater();
        if (res == QDialog::Accepted) {
            if (dialog->subject().isEmpty()) {
                return sendMessage(i18n("Commit message cannot be empty."), true);
            }
            m_commitMessage = dialog->subject() + QStringLiteral("[[\n\n]]") + dialog->description();
            commitChanges(dialog->subject(), dialog->description(), dialog->signoff(), dialog->amendingLastCommit());
        }
    });

    dialog->open();
}

void GitWidget::handleClick(const QModelIndex &idx, ClickAction clickAction)
{
    const auto type = idx.data(GitStatusModel::TreeItemType);
    if (type != GitStatusModel::NodeFile) {
        return;
    }

    if (clickAction == ClickAction::NoAction) {
        return;
    }

    const QString file = m_activeGitDirPath + idx.data(GitStatusModel::FileNameRole).toString();
    const auto statusItemType = idx.data(GitStatusModel::GitItemType).value<GitStatusModel::ItemType>();
    const bool staged = statusItemType == GitStatusModel::NodeStage;

    if (clickAction == ClickAction::StageUnstage) {
        if (staged) {
            return unstage({file});
        }
        return stage({file});
    }

    if (clickAction == ClickAction::ShowDiff && statusItemType != GitStatusModel::NodeUntrack) {
        showDiff(file, staged);
    }

    if (clickAction == ClickAction::OpenFile) {
        m_mainWin->openUrl(QUrl::fromLocalFile(file));
    }
}

void GitWidget::treeViewSingleClicked(const QModelIndex &idx)
{
    handleClick(idx, m_pluginView->plugin()->singleClickAcion());
}

void GitWidget::treeViewDoubleClicked(const QModelIndex &idx)
{
    handleClick(idx, m_pluginView->plugin()->doubleClickAcion());
}

void GitWidget::parseStatusReady()
{
    // Remember collapse/expand state
    // The default is expanded, so only add here which should be not expanded
    QMap<int, bool> nodeIsExpanded;
    nodeIsExpanded.insert(GitStatusModel::NodeUntrack, false);

    const auto *model = m_treeView->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto index = model->index(i, 0);
        if (!index.isValid()) {
            continue;
        }
        const auto t = index.data(GitStatusModel::TreeItemType).toInt();
        nodeIsExpanded[t] = m_treeView->isExpanded(index);
    }

    // Set new data
    GitUtils::GitParsedStatus s = m_gitStatusWatcher.result();
    m_model->setStatusItems(std::move(s), m_pluginView->plugin()->showGitStatusWithNumStat());

    // Restore collapse/expand state
    for (int i = 0; i < model->rowCount(); ++i) {
        const auto index = model->index(i, 0);
        if (!index.isValid()) {
            continue;
        }
        const auto t = index.data(GitStatusModel::TreeItemType).toInt();
        if (!nodeIsExpanded.contains(t) || nodeIsExpanded[t]) {
            m_treeView->expand(index);
        } else {
            m_treeView->collapse(index);
        }
    }

    m_treeView->resizeColumnToContents(0);
    m_treeView->resizeColumnToContents(1);
}

void GitWidget::branchCompareFiles(const QString &from, const QString &to)
{
    if (from.isEmpty() && to.isEmpty()) {
        return;
    }

    // git diff br...br2 --name-only -z
    auto args = QStringList{QStringLiteral("diff"), QStringLiteral("%1...%2").arg(from).arg(to), QStringLiteral("--name-status")};

    QProcess git;

    // early out if we can't find git
    if (!setupGitProcess(git, m_activeGitDirPath, args)) {
        return;
    }

    startHostProcess(git, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            return;
        }
    }

    const QByteArray diff = git.readAllStandardOutput();
    if (diff.isEmpty()) {
        sendMessage(i18n("No diff for %1...%2", from, to), false);
        return;
    }

    auto filesWithNameStatus = GitUtils::parseDiffNameStatus(diff);
    if (filesWithNameStatus.isEmpty()) {
        sendMessage(i18n("Failed to compare %1...%2", from, to), true);
        return;
    }

    // get --num-stat
    args = QStringList{QStringLiteral("diff"), QStringLiteral("%1...%2").arg(from).arg(to), QStringLiteral("--numstat"), QStringLiteral("-z")};

    // early out if we can't find git
    if (!setupGitProcess(git, m_activeGitDirPath, args)) {
        return;
    }

    startHostProcess(git, QProcess::ReadOnly);
    if (git.waitForStarted() && git.waitForFinished(-1)) {
        if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
            sendMessage(i18n("Failed to get numstat when diffing %1...%2", from, to), true);
            return;
        }
    }

    GitUtils::parseDiffNumStat(filesWithNameStatus, git.readAllStandardOutput());

    CompareBranchesView *w = new CompareBranchesView(this, m_activeGitDirPath, from, to, filesWithNameStatus);
    w->setPluginView(m_pluginView);
    connect(w, &CompareBranchesView::backClicked, this, [this] {
        auto x = m_stackWidget->currentWidget();
        if (x) {
            m_stackWidget->setCurrentWidget(m_mainView);
            x->deleteLater();
        }
    });
    m_stackWidget->addWidget(w);
    m_stackWidget->setCurrentWidget(w);
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

void GitWidget::buildMenu(KActionCollection *ac)
{
    m_gitMenu = new QMenu(this);
    auto a = ac->addAction(QStringLiteral("vcs_status_refresh"), this, [this] {
        if (m_project) {
            updateStatus();
        }
    });
    a->setText(i18n("Refresh"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_gitMenu->addAction(a);

    a = ac->addAction(QStringLiteral("vcs_amend"), this, [this] {
        openCommitChangesDialog(/* amend = */ true);
    });
    a->setIcon(QIcon::fromTheme(QStringLiteral("document-edit")));
    a->setText(i18n("Amend Last Commit"));
    ac->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+T, Ctrl+K"), QKeySequence::PortableText));
    m_gitMenu->addAction(a);

    a = ac->addAction(QStringLiteral("vcs_branch_checkout"), this, [this] {
        BranchCheckoutDialog bd(m_mainWin->window(), m_pluginView, m_project->baseDir());
        bd.openDialog();
    });
    a->setText(i18n("Checkout Branch"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-branch")));
    ac->setDefaultShortcut(a, QKeySequence(QStringLiteral("Ctrl+T, C"), QKeySequence::PortableText));
    m_gitMenu->addAction(a);

    a = ac->addAction(QStringLiteral("vcs_branch_delete"), this, [this] {
        BranchDeleteDialog dlg(m_activeGitDirPath, this);
        if (dlg.exec() == QDialog::Accepted) {
            auto result = GitUtils::deleteBranches(dlg.branchesToDelete(), m_activeGitDirPath);
            sendMessage(result.error, result.returnCode != 0);
        }
    });
    a->setText(i18n("Delete Branch"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    m_gitMenu->addAction(a);

    a = ac->addAction(QStringLiteral("vcs_branch_diff"), this, [this] {
        BranchesDialog bd(m_mainWin->window(), m_pluginView, m_project->baseDir());
        using GitUtils::RefType;
        bd.openDialog(static_cast<GitUtils::RefType>(RefType::Head | RefType::Remote));
        QString branch = bd.branch();
        branchCompareFiles(branch, QString());
    });
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-diff")));
    a->setText(i18n("Compare Branch with..."));
    m_gitMenu->addAction(a);

    auto stashMenu = m_gitMenu->addAction(QIcon::fromTheme(QStringLiteral("vcs-stash")), i18n("Stash"));
    stashMenu->setMenu(this->stashMenu(ac));
}

void GitWidget::createStashDialog(StashMode m, const QString &gitPath)
{
    auto stashDialog = new StashDialog(this, mainWindow()->window(), gitPath);
    connect(stashDialog, &StashDialog::message, this, &GitWidget::sendMessage);
    connect(stashDialog, &StashDialog::showStashDiff, this, [this](const QByteArray &r) {
        DiffParams d;
        d.tabTitle = i18n("Diff - stash");
        d.workingDir = m_activeGitDirPath;
        Utils::showDiff(r, d, mainWindow());
    });
    connect(stashDialog, &StashDialog::done, this, [this, stashDialog] {
        updateStatus();
        stashDialog->deleteLater();
    });
    stashDialog->openDialog(m);
}

void GitWidget::enableCancel(QProcess *git)
{
    m_cancelHandle = git;
    m_pushBtn->hide();
    m_cancelBtn->show();
}

void GitWidget::hideCancel()
{
    m_cancelBtn->hide();
    m_pushBtn->show();
}

QMenu *GitWidget::stashMenu(KActionCollection *ac)
{
    QMenu *menu = new QMenu(this);
    auto a = stashMenuAction(ac, QStringLiteral("vcs_stash"), i18n("Stash"), StashMode::Stash);
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-stash")));
    menu->addAction(a);

    a = stashMenuAction(ac, QStringLiteral("vcs_stash_pop_last"), i18n("Pop Last Stash"), StashMode::StashPopLast);
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-stash-pop")));
    menu->addAction(a);

    a = stashMenuAction(ac, QStringLiteral("vcs_stash_pop"), i18n("Pop Stash"), StashMode::StashPop);
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-stash-pop")));
    menu->addAction(a);

    menu->addAction(stashMenuAction(ac, QStringLiteral("vcs_stash_apply_last"), i18n("Apply Last Stash"), StashMode::StashApplyLast));

    a = stashMenuAction(ac, QStringLiteral("vcs_stash_keep_staged"), i18n("Stash (Keep Staged)"), StashMode::StashKeepIndex);
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-stash")));
    menu->addAction(a);

    a = stashMenuAction(ac, QStringLiteral("vcs_stash_include_untracked"), i18n("Stash (Include Untracked)"), StashMode::StashUntrackIncluded);
    a->setIcon(QIcon::fromTheme(QStringLiteral("vcs-stash")));
    menu->addAction(a);

    menu->addAction(stashMenuAction(ac, QStringLiteral("vcs_stash_apply"), i18n("Apply Stash"), StashMode::StashApply));
    menu->addAction(stashMenuAction(ac, QStringLiteral("vcs_stash_drop"), i18n("Drop Stash"), StashMode::StashDrop));
    menu->addAction(stashMenuAction(ac, QStringLiteral("vcs_stash_show"), i18n("Show Stash Content"), StashMode::ShowStashContent));

    return menu;
}

QAction *GitWidget::stashMenuAction(KActionCollection *ac, const QString &name, const QString &text, StashMode m)
{
    auto a = ac->addAction(name, this, [this, m] {
        createStashDialog(m, m_activeGitDirPath);
    });
    a->setText(text);
    return a;
}

static KMessageBox::ButtonCode confirm(GitWidget *_this, const QString &text, const KGuiItem &confirmItem)
{
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
    return KMessageBox::questionTwoActions(_this, text, {}, confirmItem, KStandardGuiItem::cancel(), {}, KMessageBox::Dangerous);
#else
    return KMessageBox::questionYesNo(_this, text, {}, confirmItem, KStandardGuiItem::cancel(), {}, KMessageBox::Dangerous);
#endif
}

void GitWidget::treeViewContextMenuEvent(QContextMenuEvent *e)
{
    if (auto selModel = m_treeView->selectionModel()) {
        if (selModel->selectedRows().count() > 1) {
            return selectedContextMenu(e);
        }
    }

    const QPersistentModelIndex idx = m_treeView->indexAt(e->pos());
    if (!idx.isValid())
        return;
    auto treeItem = idx.data(GitStatusModel::TreeItemType);

    if (treeItem == GitStatusModel::NodeChanges || treeItem == GitStatusModel::NodeUntrack) {
        QMenu menu(this);
        bool untracked = treeItem == GitStatusModel::NodeUntrack;

        auto stageAct = menu.addAction(i18n("Stage All"));

        auto discardAct = untracked ? menu.addAction(i18n("Remove All")) : menu.addAction(i18n("Discard All"));
        discardAct->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete-remove")));

        auto ignoreAct = untracked ? menu.addAction(i18n("Open .gitignore")) : nullptr;
        auto diff = !untracked ? menu.addAction(QIcon::fromTheme(QStringLiteral("vcs-diff")), i18n("Show Diff")) : nullptr;
        // get files
        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
        if (!act) {
            return;
        }

        const QVector<GitUtils::StatusItem> &items = untracked ? m_model->untrackedFiles() : m_model->changedFiles();
        QStringList files;
        files.reserve(items.size());
        std::transform(items.begin(), items.end(), std::back_inserter(files), [](const GitUtils::StatusItem &i) {
            return QString::fromUtf8(i.file);
        });

        if (act == stageAct) {
            stage(files, treeItem == GitStatusModel::NodeUntrack);
        } else if (act == discardAct && !untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to remove these files?"), KStandardGuiItem::remove());
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            if (ret == KMessageBox::PrimaryAction) {
#else
            if (ret == KMessageBox::Yes) {
#endif
                discard(files);
            }
        } else if (act == discardAct && untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to discard all changes?"), KStandardGuiItem::discard());
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            if (ret == KMessageBox::PrimaryAction) {
#else
            if (ret == KMessageBox::Yes) {
#endif
                clean(files);
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
        } else if (!untracked && act == diff) {
            showDiff(QString(), false);
        }
    } else if (treeItem == GitStatusModel::NodeFile) {
        QMenu menu(this);
        const auto statusItemType = idx.data(GitStatusModel::GitItemType).value<GitStatusModel::ItemType>();
        const bool staged = statusItemType == GitStatusModel::NodeStage;
        const bool untracked = statusItemType == GitStatusModel::NodeUntrack;

        auto openFile = menu.addAction(i18n("Open File"));
        auto showDiffAct = untracked ? nullptr : menu.addAction(QIcon::fromTheme(QStringLiteral("vcs-diff")), i18n("Show Diff"));
        auto launchDifftoolAct = untracked ? nullptr : menu.addAction(QIcon::fromTheme(QStringLiteral("kdiff3")), i18n("Show in External Git Diff Tool"));
        auto openAtHead = untracked ? nullptr : menu.addAction(i18n("Open at HEAD"));
        auto stageAct = staged ? menu.addAction(i18n("Unstage File")) : menu.addAction(i18n("Stage File"));
        auto discardAct = staged ? nullptr : untracked ? menu.addAction(i18n("Remove")) : menu.addAction(i18n("Discard"));
        if (discardAct) {
            discardAct->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete-remove")));
        }

        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
        if (!act || !idx.isValid()) {
            return;
        }

        const QString file = m_activeGitDirPath + idx.data(GitStatusModel::FileNameRole).toString();
        if (act == stageAct) {
            if (staged) {
                return unstage({file});
            }
            return stage({file});
        } else if (act == discardAct && !untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to discard the changes in this file?"), KStandardGuiItem::discard());
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            if (ret == KMessageBox::PrimaryAction) {
#else
            if (ret == KMessageBox::Yes) {
#endif
                discard({file});
            }
        } else if (act == openAtHead && !untracked) {
            openAtHEAD(idx.data(GitStatusModel::FileNameRole).toString());
        } else if (showDiffAct && act == showDiffAct && !untracked) {
            showDiff(file, staged);
        } else if (act == discardAct && untracked) {
            auto ret = confirm(this, i18n("Are you sure you want to remove this file?"), KStandardGuiItem::remove());
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            if (ret == KMessageBox::PrimaryAction) {
#else
            if (ret == KMessageBox::Yes) {
#endif
                clean({file});
            }
        } else if (act == launchDifftoolAct) {
            launchExternalDiffTool(idx.data(GitStatusModel::FileNameRole).toString(), staged);
        } else if (act == openFile) {
            m_mainWin->openUrl(QUrl::fromLocalFile(file));
        }
    } else if (treeItem == GitStatusModel::NodeStage) {
        QMenu menu(this);
        auto stage = menu.addAction(i18n("Unstage All"));
        auto diff = menu.addAction(i18n("Show Diff"));
        auto model = m_treeView->model();
        bool disable = model->rowCount(idx) == 0;
        stage->setDisabled(disable);
        diff->setDisabled(disable);

        auto act = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
        if (!act) {
            return;
        }

        // git reset -q HEAD --
        if (act == stage) {
            const QVector<GitUtils::StatusItem> &items = m_model->stagedFiles();
            QStringList files;
            files.reserve(items.size());
            std::transform(items.begin(), items.end(), std::back_inserter(files), [](const GitUtils::StatusItem &i) {
                return QString::fromUtf8(i.file);
            });
            unstage(files);
        } else if (act == diff) {
            showDiff(QString(), true);
        }
    }
}

void GitWidget::selectedContextMenu(QContextMenuEvent *e)
{
    QStringList files;

    bool selectionHasStagedItems = false;
    bool selectionHasChangedItems = false;
    bool selectionHasUntrackedItems = false;

    if (auto selModel = m_treeView->selectionModel()) {
        const auto idxList = selModel->selectedIndexes();
        for (const auto &idx : idxList) {
            // no context menu for multi selection of top level nodes
            const bool isTopLevel = idx.data(GitStatusModel::TreeItemType).value<GitStatusModel::ItemType>() != GitStatusModel::NodeFile;
            if (isTopLevel) {
                return;
            }

            // what type of status item is this?
            auto type = idx.data(GitStatusModel::GitItemType).value<GitStatusModel::ItemType>();

            if (type == GitStatusModel::NodeStage) {
                selectionHasStagedItems = true;
            } else if (type == GitStatusModel::NodeUntrack) {
                selectionHasUntrackedItems = true;
            } else if (type == GitStatusModel::NodeChanges) {
                selectionHasChangedItems = true;
            }

            files.append(idx.data(GitStatusModel::FileNameRole).toString());
        }
    }

    const bool selHasUnstagedItems = selectionHasUntrackedItems || selectionHasChangedItems;

    // cant allow both
    if (selHasUnstagedItems && selectionHasStagedItems) {
        return;
    }

    QMenu menu(this);
    auto stageAct = selectionHasStagedItems ? menu.addAction(i18n("Unstage Selected Files")) : menu.addAction(i18n("Stage Selected Files"));
    auto discardAct = selectionHasChangedItems && !selectionHasUntrackedItems ? menu.addAction(i18n("Discard Selected Files")) : nullptr;
    if (discardAct) {
        discardAct->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete-remove")));
    }
    auto removeAct = !selectionHasChangedItems && selectionHasUntrackedItems ? menu.addAction(i18n("Remove Selected Files")) : nullptr;
    auto execAct = menu.exec(m_treeView->viewport()->mapToGlobal(e->pos()));
    if (!execAct) {
        return;
    }

    if (execAct == stageAct) {
        if (selectionHasChangedItems || selectionHasUntrackedItems) {
            stage(files);
        } else {
            unstage(files);
        }
    } else if (selectionHasChangedItems && !selectionHasUntrackedItems && execAct == discardAct) {
        auto ret = confirm(this, i18n("Are you sure you want to discard the changes?"), KStandardGuiItem::discard());
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        if (ret == KMessageBox::PrimaryAction) {
#else
        if (ret == KMessageBox::Yes) {
#endif
            discard(files);
        }
    } else if (!selectionHasChangedItems && selectionHasUntrackedItems && execAct == removeAct) {
        auto ret = confirm(this, i18n("Are you sure you want to remove these untracked changes?"), KStandardGuiItem::remove());
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
        if (ret == KMessageBox::PrimaryAction) {
#else
        if (ret == KMessageBox::Yes) {
#endif
            clean(files);
        }
    }
}
