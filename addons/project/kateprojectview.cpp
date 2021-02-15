/*  This file is part of the Kate project.
 *
 *  SPDX-FileCopyrightText: 2012 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kateprojectview.h"
#include "branchesdialog.h"
#include "git/gitutils.h"
#include "gitwidget.h"
#include "kateprojectfiltermodel.h"
#include "kateprojectpluginview.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KActionCollection>
#include <KLineEdit>
#include <KLocalizedString>

#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QVBoxLayout>

KateProjectView::KateProjectView(KateProjectPluginView *pluginView, KateProject *project, KTextEditor::MainWindow *mainWindow)
    : m_pluginView(pluginView)
    , m_project(project)
    , m_treeView(new KateProjectViewTree(pluginView, project))
    , m_filter(new KLineEdit())
    , m_branchBtn(new QPushButton)
    , m_gitBtn(new QPushButton)
    , m_stackWidget(new QStackedWidget)
    , m_gitWidget(new GitWidget(project, this, mainWindow))
{
    /**
     * layout tree view and co.
     */
    QVBoxLayout *layout = new QVBoxLayout();
    QHBoxLayout *btnLayout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(btnLayout);
    layout->addWidget(m_branchBtn);
    layout->addWidget(m_gitBtn);
    layout->addWidget(m_stackWidget);
    //    layout->addWidget(m_treeView);
    layout->addWidget(m_filter);
    setLayout(layout);

    btnLayout->addWidget(m_branchBtn);
    btnLayout->setStretch(0, 2);
    btnLayout->addWidget(m_gitBtn);

    m_stackWidget->addWidget(m_treeView);
    m_stackWidget->addWidget(m_gitWidget);

    connect(m_gitBtn, &QPushButton::clicked, this, [this] {
        m_gitWidget->getStatus();
        m_stackWidget->setCurrentWidget(m_gitWidget);
    });

    m_branchBtn->setText(GitUtils::getCurrentBranchName(m_project->baseDir()));
    m_branchBtn->setIcon(QIcon(QStringLiteral(":/kxmlgui5/kateproject/sc-apps-git.svg")));
    m_gitBtn->setIcon(QIcon(QStringLiteral(":/kxmlgui5/kateproject/sc-apps-git.svg")));

    // let tree get focus for keyboard selection of file to open
    setFocusProxy(m_treeView);

    // add to actionCollection so that this is available in Kate Command bar
    auto chckbr = pluginView->actionCollection()->addAction(QStringLiteral("checkout_branch"), this, [this] {
        m_branchBtn->click();
    });
    chckbr->setText(QStringLiteral("Checkout Git Branch"));

    /**
     * setup filter line edit
     */
    m_filter->setPlaceholderText(i18n("Filter..."));
    m_filter->setClearButtonEnabled(true);
    connect(m_filter, &KLineEdit::textChanged, this, &KateProjectView::filterTextChanged);

    /**
     * Setup git checkout stuff
     */
    m_branchBtn->setHidden(true);
    m_branchesDialog = new BranchesDialog(this, mainWindow, m_project->baseDir());
    connect(m_branchBtn, &QPushButton::clicked, this, [this] {
        if (m_stackWidget->currentWidget() != m_treeView) {
            m_stackWidget->setCurrentWidget(m_treeView);
            return;
        }
        m_branchesDialog->openDialog();
    });
    connect(m_branchesDialog, &BranchesDialog::branchChanged, this, [this](const QString &branch) {
        m_branchBtn->setText(branch);
        m_project->reload(true);
    });
    connect(m_project, &KateProject::modelChanged, this, [this] {
        if (GitUtils::isGitRepo(m_project->baseDir())) {
            m_branchBtn->setHidden(false);
            m_branchBtn->setText(GitUtils::getCurrentBranchName(m_project->baseDir()));
            if (m_branchChangedWatcher.files().isEmpty()) {
                m_branchChangedWatcher.addPath(m_project->baseDir() + QStringLiteral("/.git/HEAD"));
            }
            m_gitWidget->getStatus();
        } else {
            if (!m_branchChangedWatcher.files().isEmpty()) {
                m_branchChangedWatcher.removePaths(m_branchChangedWatcher.files());
            }
            m_branchBtn->setHidden(true);
        }
    });
    connect(&m_branchChangedWatcher, &QFileSystemWatcher::fileChanged, this, [this] {
        m_project->reload(true);
    });
    connect(m_gitWidget, &GitWidget::checkoutBranch, this, [this] {
        m_branchesDialog->openDialog();
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

void KateProjectView::filterTextChanged(const QString &filterText)
{
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
