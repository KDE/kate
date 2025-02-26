/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "quickdialog.h"
#include "drawing_utils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QMainWindow>
#include <QPainter>
#include <QStringListModel>
#include <QTextLayout>
#include <QToolBar>
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

    QList<QTextLayout::FormatRange> formats;

    QTextCharFormat fmt;
    fmt.setForeground(options.palette.link());
    fmt.setFontWeight(QFont::Bold);
    auto ranges = KFuzzyMatcher::matchedRanges(m_filterString, text);
    QList<QTextLayout::FormatRange> resFmts;
    std::transform(ranges.begin(), ranges.end(), std::back_inserter(resFmts), [fmt](const KFuzzyMatcher::Range &fr) {
        return QTextLayout::FormatRange{.start = fr.start, .length = fr.length, .format = fmt};
    });

    formats.append(resFmts);

    painter->save();

    options.text = QString(); // clear old text
    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);
    options.rect.adjust(4, 0, 0, 0);

    Utils::paintItemViewText(painter, text, options, formats);

    painter->restore();
}

HUDDialog::HUDDialog(QWidget *mainWindow)
    : QFrame(mainWindow)
    , m_mainWindow(mainWindow)
    , m_model(new QStringListModel(this))
    , m_proxy(new FuzzyFilterModel(this))
{
    Q_ASSERT(mainWindow);

    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setProperty("_breeze_force_frame", true);

    QGraphicsDropShadowEffect *e = new QGraphicsDropShadowEffect(this);
    e->setColor(palette().color(QPalette::Dark));
    e->setOffset(0, 4);
    e->setBlurRadius(24);
    setGraphicsEffect(e);

    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterRole(Qt::DisplayRole);
    m_proxy->setFilterKeyColumn(0);

    m_delegate = new HUDStyleDelegate(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins());

    setFocusProxy(&m_lineEdit);

    layout->addWidget(&m_lineEdit);

    layout->addWidget(&m_treeView, 1);

    m_treeView.setModel(m_proxy);
    m_treeView.setProperty("_breeze_borders_sides", QVariant::fromValue(QFlags(Qt::TopEdge)));
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

    m_lineEdit.setClearButtonEnabled(true);
    m_lineEdit.addAction(QIcon::fromTheme(QStringLiteral("search")), QLineEdit::LeadingPosition);
    m_lineEdit.setFrame(false);
    m_lineEdit.setTextMargins(QMargins() + style()->pixelMetric(QStyle::PM_ButtonMargin));

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
    deleteLater();
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

        if (keyEvent->key() == Qt::Key_Escape) {
            hide();
            deleteLater();
            return true;
        }
    }

    // hide on focus out, if neither input field nor list have focus!
    else if (event->type() == QEvent::FocusOut && !(m_lineEdit.hasFocus() || m_treeView.hasFocus())) {
        hide();
        deleteLater();
        return true;
    }

    // handle resizing
    if (m_mainWindow == obj && event->type() == QEvent::Resize) {
        updateViewGeometry();
    }

    return QWidget::eventFilter(obj, event);
}

QRect HUDDialog::getQuickOpenBoundingRect(QMainWindow *mainWindow)
{
    QRect boundingRect = mainWindow->contentsRect();

    // exclude the menu bar from the bounding rect
    if (const QWidget *menuWidget = mainWindow->menuWidget()) {
        if (!menuWidget->isHidden()) {
            boundingRect.setTop(boundingRect.top() + menuWidget->height());
        }
    }

    // exclude any undocked toolbar from the bounding rect
    const QList<QToolBar *> toolBars = mainWindow->findChildren<QToolBar *>();
    for (QToolBar *toolBar : toolBars) {
        if (toolBar->isHidden() || toolBar->isFloating()) {
            continue;
        }

        if (mainWindow->toolBarArea(toolBar) == Qt::TopToolBarArea) {
            boundingRect.setTop(std::max(boundingRect.top(), toolBar->geometry().bottom()));
        }
    }

    return boundingRect;
}

void HUDDialog::updateViewGeometry()
{
    const QRect boundingRect = getQuickOpenBoundingRect(qobject_cast<QMainWindow *>(m_mainWindow));

    static constexpr int minWidth = 500;
    const int maxWidth = boundingRect.width();
    const int preferredWidth = maxWidth / 2.4;

    static constexpr int minHeight = 250;
    const int maxHeight = boundingRect.height();
    const int preferredHeight = maxHeight / 2;

    const QSize size{std::min(maxWidth, std::max(preferredWidth, minWidth)), std::min(maxHeight, std::max(preferredHeight, minHeight))};

    // resize() doesn't work here, so use setFixedSize() instead
    setFixedSize(size);

    // set the position to the top-center of the parent
    // just below the menubar/toolbar (if any)
    const QPoint position{boundingRect.center().x() - (size.width() / 2), boundingRect.y() + 6};
    move(position);
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

#include "moc_quickdialog.cpp"
