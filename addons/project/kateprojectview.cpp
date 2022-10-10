/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectview.h"
#include "branchcheckoutdialog.h"
#include "gitprocess.h"
#include "gitwidget.h"
#include "kateprojectfiltermodel.h"
#include "kateprojectpluginview.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KAcceleratorManager>
#include <KActionCollection>
#include <KLineEdit>
#include <KLocalizedString>

#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

KateProjectView::KateProjectView(KateProjectPluginView *pluginView, KateProject *project, KTextEditor::MainWindow *mainWindow)
    : m_pluginView(pluginView)
    , m_project(project)
    , m_treeView(new KateProjectViewTree(pluginView, project))
    , m_filter(new KLineEdit())
    , m_branchBtn(new QToolButton)
{
    /**
     * layout tree view and co.
     */
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_branchBtn);
    layout->addWidget(m_treeView);
    layout->addWidget(m_filter);
    setLayout(layout);

    m_branchBtn->setAutoRaise(true);
    m_branchBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_branchBtn->setSizePolicy(QSizePolicy::Minimum, m_branchBtn->sizePolicy().verticalPolicy());
    KAcceleratorManager::setNoAccel(m_branchBtn);

    // let tree get focus for keyboard selection of file to open
    setFocusProxy(m_treeView);

    // add to actionCollection so that this is available in Kate Command bar
    auto chckbr = pluginView->actionCollection()->addAction(QStringLiteral("checkout_branch"), this, [this] {
        m_branchBtn->click();
    });
    chckbr->setText(i18n("Checkout Git Branch"));

    m_filterStartTimer.setSingleShot(true);
    m_filterStartTimer.setInterval(400);
    m_filterStartTimer.callOnTimeout(this, &KateProjectView::filterTextChanged);

    /**
     * setup filter line edit
     */
    m_filter->setPlaceholderText(i18n("Filter..."));
    m_filter->setClearButtonEnabled(true);
    connect(m_filter, &KLineEdit::textChanged, this, [this] {
        m_filterStartTimer.start();
    });

    /**
     * Setup git checkout stuff
     */
    auto currBranchAct = pluginView->actionCollection()->addAction(QStringLiteral("current_branch"), this, [this, mainWindow] {
        BranchCheckoutDialog bd(mainWindow->window(), m_pluginView, m_project->baseDir());
        bd.openDialog();
    });
    currBranchAct->setIcon(gitIcon());
    currBranchAct->setToolTip(i18n("Checkout branch"));
    m_branchBtn->setDefaultAction(currBranchAct);

    checkAndRefreshGit();

    connect(m_project, &KateProject::modelChanged, this, &KateProjectView::checkAndRefreshGit);
    connect(&m_branchChangedWatcher, &QFileSystemWatcher::fileChanged, this, [this] {
        m_project->reload(true);
    });
}

KateProjectView::~KateProjectView()
{
}

void KateProjectView::selectFile(const QString &file)
{
    m_treeView->selectFile(file);
}

void KateProjectView::openSelectedDocument()
{
    m_treeView->openSelectedDocument();
}

void KateProjectView::filterTextChanged()
{
    const auto filterText = m_filter->text();
    /**
     * filter
     */
    static_cast<KateProjectFilterProxyModel *>(m_treeView->model())->setFilterString(filterText);

    /**
     * expand
     */
    if (!filterText.isEmpty()) {
        QTimer::singleShot(100, m_treeView, &QTreeView::expandAll);
    }
}

void KateProjectView::checkAndRefreshGit()
{
    const auto dotGitPath = getRepoBasePath(m_project->baseDir());
    /**
     * Not in a git repo or git was removed
     */
    if (!dotGitPath.has_value()) {
        if (!m_branchChangedWatcher.files().isEmpty()) {
            m_branchChangedWatcher.removePaths(m_branchChangedWatcher.files());
        }
        m_branchBtn->setHidden(true);
    } else {
        m_branchBtn->setHidden(false);
        auto act = m_branchBtn->defaultAction();
        Q_ASSERT(act);
        act->setText(GitUtils::getCurrentBranchName(dotGitPath.value()));
        if (m_branchChangedWatcher.files().isEmpty()) {
            m_branchChangedWatcher.addPath(dotGitPath.value() + QStringLiteral(".git/HEAD"));
        }
    }
}
