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

    resize(500, 500);
}

void BranchDeleteDialog::loadBranches(const QString &dotGitPath)
{
#if KTEXTEDITOR_VERSION >= QT_VERSION_CHECK(5, 80, 0)
    QFont f = KTextEditor::Editor::instance()->font();
#else
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#endif

    static const auto branchIcon = QIcon::fromTheme(QStringLiteral("vcs-branch"));
    const auto branches = GitUtils::getAllBranchesAndTags(dotGitPath, GitUtils::RefType::Head);
    for (const auto &branch : branches) {
        auto item = new QStandardItem(branchIcon, branch.name);
        item->setFont(f);
        item->setCheckable(true);
        m_model.appendRow(item);
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
