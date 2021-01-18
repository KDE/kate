#include "katecommandbar.h"
#include "commandmodel.h"

#include <QTreeView>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDebug>
#include <QAction>
#include <QPropertyAnimation>
#include <QPointer>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include <kfts_fuzzy_match.h>

class CommandBarFilterModel : public QSortFilterProxyModel
{
public:
    CommandBarFilterModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    Q_SLOT void setFilterString(const QString &string)
    {
        beginResetModel();
        m_pattern = string;
        endResetModel();
    }

protected:
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override
    {
        const int l = sourceLeft.data(CommandModel::Score).toInt();
        const int r = sourceRight.data(CommandModel::Score).toInt();
        return l < r;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (m_pattern.isEmpty())
            return true;

        int score = 0;
        const auto idx = sourceModel()->index(sourceRow, 0, sourceParent);
        const QString actionName = idx.data().toString();
        const bool res = kfts::fuzzy_match(m_pattern, actionName, score);
        sourceModel()->setData(idx, score, CommandModel::Score);
        return res;
    }

private:
    QString m_pattern;
};

class CommandBarStyleDelegate : public QStyledItemDelegate
{
public:
    CommandBarStyleDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        QTextDocument doc;

        QString str = index.data().toString();
        const QString nameColor = option.palette.color(QPalette::Link).name();
        kfts::to_fuzzy_matched_display_string(m_filterString, str, QStringLiteral("<b style=\"color:%1;\">").arg(nameColor), QStringLiteral("</b>"));

        doc.setHtml(QStringLiteral("<span>") + str + QStringLiteral("</span>"));
        doc.setDocumentMargin(2);

        painter->save();

        // paint background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(option.rect, option.palette.base());
        }

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        // draw text
        painter->translate(option.rect.x(), option.rect.y());
        // leave space for icon
        painter->translate(25, 0);
        doc.drawContents(painter);

        painter->restore();
    }

public Q_SLOTS:
    void setFilterString(const QString &text)
    {
        m_filterString = text;
    }

private:
    QString m_filterString;
};

KateCommandBar::KateCommandBar(QWidget *parent)
    : QMenu(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(4, 4, 4, 4);
    setLayout(layout);

    m_lineEdit = new QLineEdit(this);
    setFocusProxy(m_lineEdit);

    layout->addWidget(m_lineEdit);

    m_treeView = new QTreeView();
    layout->addWidget(m_treeView, 1);
    m_treeView->setTextElideMode(Qt::ElideLeft);
    m_treeView->setUniformRowHeights(true);

    m_model = new CommandModel(this);

    CommandBarStyleDelegate* delegate = new CommandBarStyleDelegate(this);
    m_treeView->setItemDelegateForColumn(0, delegate);

    m_proxyModel = new CommandBarFilterModel(this);
    m_proxyModel->setFilterRole(Qt::DisplayRole);
    m_proxyModel->setSortRole(CommandModel::Score);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(0);

    connect(m_lineEdit, &QLineEdit::returnPressed, this, &KateCommandBar::slotReturnPressed);
    connect(m_lineEdit, &QLineEdit::textChanged, m_proxyModel, &CommandBarFilterModel::setFilterString);
    connect(m_lineEdit, &QLineEdit::textChanged, delegate, &CommandBarStyleDelegate::setFilterString);
    connect(m_lineEdit, &QLineEdit::textChanged, this, [this](){
        m_treeView->viewport()->update();
        reselectFirst();
    });
    connect(m_treeView, &QTreeView::clicked, this, &KateCommandBar::slotReturnPressed);

    m_proxyModel->setSourceModel(m_model);
    m_treeView->setSortingEnabled(true);
    m_treeView->setModel(m_proxyModel);

    m_treeView->installEventFilter(this);
    m_lineEdit->installEventFilter(this);

    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setSelectionMode(QTreeView::SingleSelection);

    setHidden(true);
}

void KateCommandBar::updateBar(QList<QAction *> actions)
{
    m_model->refresh(actions);
    reselectFirst();

    updateViewGeometry();
    show();
    setFocus();
}

bool KateCommandBar::eventFilter(QObject *obj, QEvent *event)
{
    // catch key presses + shortcut overrides to allow to have ESC as application wide shortcut, too, see bug 409856
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == m_lineEdit) {
            const bool forward2list = (keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down) || (keyEvent->key() == Qt::Key_PageUp) || (keyEvent->key() == Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(m_treeView, event);
                return true;
            }

            if (keyEvent->key() == Qt::Key_Escape) {
                m_lineEdit->clear();
                keyEvent->accept();
                hide();
                return true;
            }
        } else {
            const bool forward2input = (keyEvent->key() != Qt::Key_Up) && (keyEvent->key() != Qt::Key_Down) && (keyEvent->key() != Qt::Key_PageUp) && (keyEvent->key() != Qt::Key_PageDown) && (keyEvent->key() != Qt::Key_Tab) &&
                (keyEvent->key() != Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(m_lineEdit, event);
                return true;
            }
        }
    }

    // hide on focus out, if neither input field nor list have focus!
    else if (event->type() == QEvent::FocusOut && !(m_lineEdit->hasFocus() || m_treeView->hasFocus())) {
        m_lineEdit->clear();
        hide();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

void KateCommandBar::slotReturnPressed()
{
    auto act = m_proxyModel->data(m_treeView->currentIndex(), Qt::UserRole).value<QAction*>();
    if (act)
        act->trigger();
    m_lineEdit->clear();
    hide();
}

void KateCommandBar::reselectFirst()
{
    QModelIndex index = m_proxyModel->index(0, 0);
    m_treeView->setCurrentIndex(index);
}

void KateCommandBar::updateViewGeometry()
{
    m_treeView->resizeColumnToContents(0);
    m_treeView->resizeColumnToContents(1);

    const QSize centralSize = parentWidget()->size();

    // width: 2.4 of editor, height: 1/2 of editor
    const QSize viewMaxSize(centralSize.width() / 2.4, centralSize.height() / 2);

    // Position should be central over window
    const int xPos = std::max(0, (centralSize.width() - viewMaxSize.width()) / 2);
    const int yPos = std::max(0, (centralSize.height() - viewMaxSize.height()) * 1 / 4);

    const QPoint p(xPos, yPos);
    move(p + parentWidget()->pos());

    this->setFixedSize(viewMaxSize);
}
