#include "branchdeletedialog.h"

#include "git/gitutils.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

#include <KLocalizedString>
#include <KMessageBox>
#include <ktexteditor/editor.h>
#include <ktexteditor_version.h>
#include <kwidgetsaddons_version.h>

class CheckableHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    CheckableHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QHeaderView(orientation, parent)
    {
    }

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override
    {
        const int w = style()->pixelMetric(QStyle::PM_IndicatorWidth);
        const int h = style()->pixelMetric(QStyle::PM_IndicatorHeight);
        const int margin = style()->pixelMetric(QStyle::PM_FocusFrameHMargin) * 2;

        QStyleOptionHeader optHeader;
        initStyleOption(&optHeader);
        optHeader.rect = rect;
        painter->save();
        style()->drawControl(QStyle::CE_Header, &optHeader, painter, this);
        painter->restore();

        painter->save();
        QHeaderView::paintSection(painter, rect.adjusted(margin + w, 0, 0, 0), logicalIndex);
        painter->restore();

        if (logicalIndex == 0) {
            QStyleOptionButton option;

            option.rect = QRect(0, 0, w, h);
            option.rect = QStyle::alignedRect(layoutDirection(), Qt::AlignVCenter, option.rect.size(), rect);
            option.rect.moveLeft(rect.left() + margin);
            option.state = QStyle::State_Enabled;
            if (m_isChecked) {
                option.state |= QStyle::State_On;
            } else {
                option.state |= QStyle::State_Off;
            }
            option.state.setFlag(QStyle::State_MouseOver, m_hovered);
            painter->save();
            this->style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
            painter->restore();
        }
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (!isPosOnCheckBox(event->pos())) {
            return;
        }

        m_isChecked = !m_isChecked;
        viewport()->update();
        QMetaObject::invokeMethod(
            this,
            [this] {
                checkAll(m_isChecked);
            },
            Qt::QueuedConnection);

        QHeaderView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        m_hovered = isPosOnCheckBox(e->pos());
        viewport()->update();
    }

    void leaveEvent(QEvent *) override
    {
        m_hovered = false;
        viewport()->update();
    }

private:
    bool isPosOnCheckBox(QPoint p)
    {
        const int pos = sectionPosition(0);
        const int w = style()->pixelMetric(QStyle::PM_IndicatorWidth);
        const int h = style()->pixelMetric(QStyle::PM_IndicatorHeight);
        const int margin = style()->pixelMetric(QStyle::PM_FocusFrameHMargin) * 2;
        QRect rect = QStyle::alignedRect(layoutDirection(), Qt::AlignVCenter, {w, h}, this->rect());
        rect.moveLeft(pos + margin);
        return rect.contains(p);
    }

    bool m_isChecked = false;
    bool m_hovered = false;

Q_SIGNALS:
    void checkAll(bool);
};

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
    auto header = new CheckableHeaderView(Qt::Horizontal, this);
    connect(header, &CheckableHeaderView::checkAll, this, &BranchDeleteDialog::onCheckAllClicked);
    header->setStretchLastSection(true);
    m_listView.setHeader(header);

    // setup the buttons
    using Btns = QDialogButtonBox::StandardButton;
    auto dlgBtns = new QDialogButtonBox(Btns::Cancel, Qt::Horizontal, this);
    auto deleteBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"));
    dlgBtns->addButton(deleteBtn, QDialogButtonBox::DestructiveRole);
    connect(dlgBtns, &QDialogButtonBox::clicked, this, [this, deleteBtn, dlgBtns](QAbstractButton *btn) {
        if (btn == deleteBtn) {
            auto count = branchesToDelete().count();
            QString ques = i18np("Are you sure you want to delete the selected branch?", "Are you sure you want to delete the selected branches?", count);
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            auto ret = KMessageBox::questionTwoActions(this,
#else
            auto ret = KMessageBox::questionYesNo(this,
#endif
                                                       ques,
                                                       {},
                                                       KStandardGuiItem::del(),
                                                       KStandardGuiItem::cancel(),
                                                       {},
                                                       KMessageBox::Dangerous);
#if KWIDGETSADDONS_VERSION >= QT_VERSION_CHECK(5, 100, 0)
            if (ret == KMessageBox::PrimaryAction) {
#else
            if (ret == KMessageBox::Yes) {
#endif
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

void BranchDeleteDialog::onCheckAllClicked(bool checked)
{
    const int rowCount = m_model.rowCount();
    for (int i = 0; i < rowCount; ++i) {
        if (auto item = m_model.item(i)) {
            item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        }
    }
}

#include "branchdeletedialog.moc"
