/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "quickdialog.h"

#include "drawing_utils.h"
#include <QCoreApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QStringListModel>
#include <QTextLayout>
#include <QVBoxLayout>

#include <KFuzzyMatcher>

namespace
{
class FuzzyFilterModel final : public QSortFilterProxyModel
{
public:
    explicit FuzzyFilterModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override
    {
        if (!m_pattern.isEmpty() && m_scoreRole > -1 && m_scoreRole > Qt::UserRole) {
            const int l = sourceLeft.data(m_scoreRole).toInt();
            const int r = sourceRight.data(m_scoreRole).toInt();
            return l < r;
        }
        return sourceLeft.row() > sourceRight.row();
    }

    bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        if (m_pattern.isEmpty()) {
            return true;
        }

        const auto index = sourceModel()->index(row, filterKeyColumn(), parent);
        const auto text = index.data(filterRole()).toString();
        if (m_filterType == HUDDialog::Fuzzy) {
            return KFuzzyMatcher::matchSimple(m_pattern, text);
        } else if (m_filterType == HUDDialog::Contains) {
            return text.contains(m_pattern, Qt::CaseInsensitive);
        } else if (m_filterType == HUDDialog::ScoredFuzzy) {
            auto res = KFuzzyMatcher::match(m_pattern, text);
            Q_ASSERT(m_scoreRole > -1 && m_scoreRole > Qt::UserRole);
            sourceModel()->setData(index, res.score, m_scoreRole);
            return res.matched;
        }
        return false;
    }

    void setFilterString(const QString &text)
    {
        beginResetModel();
        m_pattern = text;
        endResetModel();
    }

    void setFilterType(HUDDialog::FilterType t)
    {
        m_filterType = t;
    }

    void setScoreRole(int role)
    {
        m_scoreRole = role;
    }

private:
    HUDDialog::FilterType m_filterType;
    QString m_pattern;
    int m_scoreRole = -1;
};

}

void HUDStyleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    QString text = index.data(m_displayRole).toString();

    QVector<QTextLayout::FormatRange> formats;

    QTextCharFormat fmt;
    fmt.setForeground(options.palette.link());
    fmt.setFontWeight(QFont::Bold);
    auto ranges = KFuzzyMatcher::matchedRanges(m_filterString, text);
    QVector<QTextLayout::FormatRange> resFmts;
    std::transform(ranges.begin(), ranges.end(), std::back_inserter(resFmts), [fmt](const KFuzzyMatcher::Range &fr) {
        return QTextLayout::FormatRange{fr.start, fr.length, fmt};
    });

    formats.append(resFmts);

    painter->save();

    options.text = QString(); // clear old text
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);
    options.rect.adjust(4, 0, 0, 0);

    Utils::paintItemViewText(painter, text, options, formats);

    painter->restore();
}

HUDDialog::HUDDialog(QWidget *parent, QWidget *mainWindow)
    : QMenu(parent)
    , m_mainWindow(mainWindow)
    , m_model(new QStringListModel(this))
    , m_proxy(new FuzzyFilterModel(this))
{
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterRole(Qt::DisplayRole);
    m_proxy->setFilterKeyColumn(0);

    m_delegate = new HUDStyleDelegate(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(4, 4, 4, 4);

    setFocusProxy(&m_lineEdit);

    layout->addWidget(&m_lineEdit);

    layout->addWidget(&m_treeView, 1);

    m_treeView.setModel(m_proxy);
    m_treeView.setTextElideMode(Qt::ElideLeft);
    m_treeView.setUniformRowHeights(true);
    m_treeView.setItemDelegate(m_delegate);

    connect(&m_lineEdit, &QLineEdit::returnPressed, this, [this] {
        slotReturnPressed(m_treeView.currentIndex());
    });
    // user can add this as necessary
    setFilteringEnabled(true);
    connect(&m_treeView, &QTreeView::clicked, this, &HUDDialog::slotReturnPressed);
    m_treeView.setSortingEnabled(true);

    m_treeView.installEventFilter(this);
    m_lineEdit.installEventFilter(this);

    m_treeView.setHeaderHidden(true);
    m_treeView.setRootIsDecorated(false);
    m_treeView.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView.setSelectionMode(QTreeView::SingleSelection);

    updateViewGeometry();
    setFocus();
}

HUDDialog::~HUDDialog()
{
    m_treeView.removeEventFilter(this);
    m_lineEdit.removeEventFilter(this);
}

void HUDDialog::slotReturnPressed(const QModelIndex &index)
{
    Q_EMIT itemExecuted(index);

    clearLineEdit();
    hide();
}

void HUDDialog::setDelegate(HUDStyleDelegate *delegate)
{
    m_delegate = delegate;
    delete m_treeView.itemDelegate();
    m_treeView.setItemDelegate(m_delegate);
}

void HUDDialog::reselectFirst()
{
    const QModelIndex index = m_treeView.model()->index(0, 0);
    m_treeView.setCurrentIndex(index);
}

void HUDDialog::setStringList(const QStringList &strList)
{
    if (auto strModel = qobject_cast<QStringListModel *>(m_proxy->sourceModel())) {
        strModel->setStringList(strList);
    } else {
        qWarning() << "You are using a custom model: " << m_model.data() << ", setStringList has no effect";
    }
}

bool HUDDialog::eventFilter(QObject *obj, QEvent *event)
{
    // catch key presses + shortcut overrides to allow to have ESC as application wide shortcut, too, see bug 409856
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == &m_lineEdit) {
            const bool forward2list = (keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down) || (keyEvent->key() == Qt::Key_PageUp)
                || (keyEvent->key() == Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(&m_treeView, event);
                return true;
            }

            if (keyEvent->key() == Qt::Key_Escape) {
                clearLineEdit();
                keyEvent->accept();
                hide();
                return true;
            }
        } else {
            const bool forward2input = (keyEvent->key() != Qt::Key_Up) && (keyEvent->key() != Qt::Key_Down) && (keyEvent->key() != Qt::Key_PageUp)
                && (keyEvent->key() != Qt::Key_PageDown) && (keyEvent->key() != Qt::Key_Tab) && (keyEvent->key() != Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(&m_lineEdit, event);
                return true;
            }
        }
    }

    // hide on focus out, if neither input field nor list have focus!
    else if (event->type() == QEvent::FocusOut && !(m_lineEdit.hasFocus() || m_treeView.hasFocus())) {
        clearLineEdit();
        hide();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

void HUDDialog::updateViewGeometry()
{
    if (!m_mainWindow)
        return;

    const QSize centralSize = m_mainWindow->size();

    // width: 2.4 of editor, height: 1/2 of editor
    const QSize viewMaxSize(centralSize.width() / 2.4, centralSize.height() / 2);

    // Position should be central over window
    const int xPos = std::max(0, (centralSize.width() - viewMaxSize.width()) / 2);
    const int yPos = std::max(0, (centralSize.height() - viewMaxSize.height()) * 1 / 4);
    const QPoint p(xPos, yPos);
    move(p + m_mainWindow->pos());

    this->setFixedSize(viewMaxSize);
}

void HUDDialog::clearLineEdit()
{
    const QSignalBlocker block(m_lineEdit);
    m_lineEdit.clear();
}

void HUDDialog::setModel(QAbstractItemModel *model, FilterType type, int filterKeyCol, int filterRole, int scoreRole)
{
    m_model = model;
    m_proxy->setSourceModel(model);
    m_proxy->setFilterKeyColumn(filterKeyCol);
    m_proxy->setFilterRole(filterRole);
    auto proxy = static_cast<FuzzyFilterModel *>(m_proxy.data());
    proxy->setFilterType(type);
    proxy->setScoreRole(scoreRole);
}

void HUDDialog::setFilteringEnabled(bool enabled)
{
    if (!enabled) {
        disconnect(&m_lineEdit, &QLineEdit::textChanged, this, nullptr);
        m_treeView.setModel(m_model);
    } else {
        Q_ASSERT(m_proxy);
        if (m_treeView.model() != m_proxy) {
            Q_ASSERT(m_proxy->sourceModel());
            m_treeView.setModel(m_proxy);
        }
        connect(&m_lineEdit, &QLineEdit::textChanged, this, [this](const QString &txt) {
            static_cast<FuzzyFilterModel *>(m_proxy.data())->setFilterString(txt);
            m_delegate->setFilterString(txt);
            m_treeView.viewport()->update();
            m_treeView.setCurrentIndex(m_treeView.model()->index(0, 0));
        });
    }
}
