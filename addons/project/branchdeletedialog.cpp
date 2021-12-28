#include "branchdeletedialog.h"

#include "git/gitutils.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QFontDatabase>
#include <QPushButton>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KMessageBox>
#include <ktexteditor/editor.h>
#include <ktexteditor_version.h>

BranchDeleteDialog::BranchDeleteDialog(const QString &dotGitPath, QWidget *parent)
    : QDialog(parent)
{
    loadBranches(dotGitPath);

    auto l = new QVBoxLayout(this);

    l->addWidget(&m_listView);

    m_model.setHorizontalHeaderLabels({i18n("Branch"), i18n("Last Commit")});

    m_listView.setUniformRowHeights(true);
    m_listView.setRootIsDecorated(false);
    m_listView.setModel(&m_model);

    // setup the buttons
    using Btns = QDialogButtonBox::StandardButton;
    auto dlgBtns = new QDialogButtonBox(Btns::Cancel, Qt::Horizontal, this);
    auto deleteBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"));
    dlgBtns->addButton(deleteBtn, QDialogButtonBox::DestructiveRole);
    connect(dlgBtns, &QDialogButtonBox::clicked, this, [this, deleteBtn, dlgBtns](QAbstractButton *btn) {
        if (btn == deleteBtn) {
            auto count = branchesToDelete().count();
            QString ques = i18np("Are you sure you want to delete the selected branch?", "Are you sure you want to delete the selected branches?", count);
            auto ret = KMessageBox::questionYesNo(this, ques, {}, KStandardGuiItem::yes(), KStandardGuiItem::no(), {}, KMessageBox::Dangerous);
            if (ret == KMessageBox::Yes) {
                accept();
            } else {
                // do nothing
            }
        } else if (dlgBtns->button(QDialogButtonBox::Cancel) == btn) {
            reject();
        }
    });

    connect(dlgBtns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dlgBtns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    l->addWidget(dlgBtns);

    m_listView.resizeColumnToContents(0);
    m_listView.resizeColumnToContents(1);

    resize(m_listView.width() * 1.5, m_listView.height() + l->contentsMargins().top() * 2);
}

void BranchDeleteDialog::loadBranches(const QString &dotGitPath)
{
    const auto f = KTextEditor::Editor::instance()->font();
    static const auto branchIcon = QIcon::fromTheme(QStringLiteral("vcs-branch"));
    const auto branches = GitUtils::getAllLocalBranchesWithLastCommitSubject(dotGitPath);
    for (const auto &branch : branches) {
        auto branchName = new QStandardItem(branchIcon, branch.name);
        auto branchLastCommit = new QStandardItem(branch.lastCommit);
        branchName->setFont(f);
        branchName->setCheckable(true);
        m_model.appendRow({branchName, branchLastCommit});
    }
}

QStringList BranchDeleteDialog::branchesToDelete() const
{
    QStringList branches;
    int rowCount = m_model.rowCount();
    for (int i = 0; i < rowCount; ++i) {
        auto item = m_model.item(i);
        if (item->checkState() == Qt::Checked) {
            branches << item->text();
        }
    }
    return branches;
}
