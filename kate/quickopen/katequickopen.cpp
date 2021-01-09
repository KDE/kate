/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2007, 2009 Joseph Wenninger <jowenn@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katequickopen.h"
#include "katequickopenmodel.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <KAboutData>
#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QBoxLayout>
#include <QCoreApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTreeView>

#include <kfts_fuzzy_match.h>

class QuickOpenFilterProxyModel : public QSortFilterProxyModel
{
public:
    QuickOpenFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    void changeMode(FilterModes m)
    {
        mode = m;
    }

protected:
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override
    {
        int l = sourceLeft.data(KateQuickOpenModel::Score).toInt();
        int r = sourceRight.data(KateQuickOpenModel::Score).toInt();
        return l < r;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (pattern.isEmpty())
            return true;
        const QString fileName = sourceModel()->index(sourceRow, 0, sourceParent).data().toString();
        const auto nameAndPath = fileName.splitRef(QStringLiteral("{[split]}"));

        const auto name = nameAndPath.at(0);
        const auto path = nameAndPath.at(1);
        int score = 0;

        bool res = false;
        if (mode == FilterMode::FilterByName) {
            res = filterByName(name, score);
        } else if (mode == FilterMode::FilterByPath) {
            res = filterByPath(path, score);
        } else {
            int scorep = 0, scoren = 0;
            bool resp = filterByPath(path, scorep);
            bool resn = filterByName(name, scoren);

            // store the score for sorting later
            score = scoren + scorep;
            res = resp || resn;
        }

        auto idx = sourceModel()->index(sourceRow, 0, sourceParent);
        sourceModel()->setData(idx, score, KateQuickOpenModel::Score);

        return res;
    }

public Q_SLOTS:
    void setFilterText(const QString &text)
    {
        beginResetModel();
        pattern = text;
        endResetModel();
    }

private:
    inline bool filterByPath(const QStringRef &path, int &score) const
    {
        return kfts::fuzzy_match(pattern, path, score);
    }

    inline bool filterByName(const QStringRef &name, int &score) const
    {
        return kfts::fuzzy_match(pattern, name, score);
    }

private:
    QString pattern;
    FilterModes mode;
};

class QuickOpenStyleDelegate : public QStyledItemDelegate
{
public:
    QuickOpenStyleDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        QTextDocument doc;

        QString str = index.data().toString();

        auto namePath = str.split(QStringLiteral("{[split]}"));
        QString name = namePath.at(0);
        QString path = namePath.at(1);

        const QString nameColor = option.palette.color(QPalette::Link).name();

        if (mode == FilterMode::FilterByName) {
            kfts::to_fuzzy_matched_display_string(m_filterString, name, QStringLiteral("<b style=\"color:%1;\">").arg(nameColor), QStringLiteral("</b>"));
        } else if (mode == FilterMode::FilterByPath) {
            kfts::to_fuzzy_matched_display_string(m_filterString, path, QStringLiteral("<b>"), QStringLiteral("</b>"));
        } else {
            kfts::to_fuzzy_matched_display_string(m_filterString, name, QStringLiteral("<b style=\"color:%1;\">").arg(nameColor), QStringLiteral("</b>"));
            kfts::to_fuzzy_matched_display_string(m_filterString, path, QStringLiteral("<b>"), QStringLiteral("</b>"));
        }

        const auto pathFontsize = option.font.pointSize();
        doc.setHtml(QStringLiteral("<span style=\"font-size: %1pt;\">").arg(pathFontsize) + name + QStringLiteral("</span>") + QStringLiteral(" &nbsp;") +
                    QStringLiteral("<span style=\"color:gray; font-size:%1pt;\">").arg(pathFontsize - 1) + path + QStringLiteral("</span>"));
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
        if (index.column() == 0) {
            painter->translate(25, 0);
        }
        doc.drawContents(painter);

        painter->restore();
    }

    void changeMode(FilterModes m)
    {
        mode = m;
    }

public Q_SLOTS:
    void setFilterString(const QString &text)
    {
        m_filterString = text;
    }

private:
    QString m_filterString;
    FilterModes mode;
};

Q_DECLARE_METATYPE(QPointer<KTextEditor::Document>)

KateQuickOpen::KateQuickOpen(KateMainWindow *mainWindow)
    : QMenu(mainWindow)
    , m_mainWindow(mainWindow)
{
    // ensure the components have some proper frame
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(4, 4, 4, 4);
    setLayout(layout);

    m_inputLine = new QuickOpenLineEdit(this);
    setFocusProxy(m_inputLine);

    layout->addWidget(m_inputLine);

    m_listView = new QTreeView();
    layout->addWidget(m_listView, 1);
    m_listView->setTextElideMode(Qt::ElideLeft);

    m_base_model = new KateQuickOpenModel(m_mainWindow, this);

    m_model = new QuickOpenFilterProxyModel(this);
    m_model->setFilterRole(Qt::DisplayRole);
    m_model->setSortRole(KateQuickOpenModel::Score);
    m_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_model->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_model->setFilterKeyColumn(Qt::DisplayRole);

    m_styleDelegate = new QuickOpenStyleDelegate(this);
    m_listView->setItemDelegate(m_styleDelegate);

    connect(m_inputLine, &QuickOpenLineEdit::textChanged, m_model, &QuickOpenFilterProxyModel::setFilterText);
    connect(m_inputLine, &QuickOpenLineEdit::textChanged, m_styleDelegate, &QuickOpenStyleDelegate::setFilterString);
    connect(m_inputLine, &QuickOpenLineEdit::textChanged, this, [this]() {
        m_listView->viewport()->update();
    });
    connect(m_inputLine, &QuickOpenLineEdit::returnPressed, this, &KateQuickOpen::slotReturnPressed);
    connect(m_inputLine, &QuickOpenLineEdit::filterModeChanged, this, &KateQuickOpen::slotfilterModeChanged);
    connect(m_inputLine, &QuickOpenLineEdit::listModeChanged, this, &KateQuickOpen::slotListModeChanged);
    connect(m_model, &QSortFilterProxyModel::rowsInserted, this, &KateQuickOpen::reselectFirst);
    connect(m_model, &QSortFilterProxyModel::rowsRemoved, this, &KateQuickOpen::reselectFirst);

    connect(m_listView, &QTreeView::activated, this, &KateQuickOpen::slotReturnPressed);
    connect(m_listView, &QTreeView::clicked, this, &KateQuickOpen::slotReturnPressed); // for single click

    m_listView->setModel(m_model);
    m_listView->setSortingEnabled(true);
    m_model->setSourceModel(m_base_model);

    m_inputLine->installEventFilter(this);
    m_listView->installEventFilter(this);
    m_listView->setHeaderHidden(true);
    m_listView->setRootIsDecorated(false);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setHidden(true);

    m_filterMode = m_inputLine->filterMode();
}

bool KateQuickOpen::eventFilter(QObject *obj, QEvent *event)
{
    // catch key presses + shortcut overrides to allow to have ESC as application wide shortcut, too, see bug 409856
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == m_inputLine) {
            const bool forward2list = (keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down) || (keyEvent->key() == Qt::Key_PageUp) || (keyEvent->key() == Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(m_listView, event);
                return true;
            }

            if (keyEvent->key() == Qt::Key_Escape) {
                m_mainWindow->slotWindowActivated();
                m_inputLine->clear();
                keyEvent->accept();
                hide();
                return true;
            }
        } else {
            const bool forward2input = (keyEvent->key() != Qt::Key_Up) && (keyEvent->key() != Qt::Key_Down) && (keyEvent->key() != Qt::Key_PageUp) && (keyEvent->key() != Qt::Key_PageDown) && (keyEvent->key() != Qt::Key_Tab) &&
                (keyEvent->key() != Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(m_inputLine, event);
                return true;
            }
        }
    }

    // hide on focus out, if neither input field nor list have focus!
    else if (event->type() == QEvent::FocusOut && !(m_inputLine->hasFocus() || m_listView->hasFocus())) {
        m_mainWindow->slotWindowActivated();
        m_inputLine->clear();
        hide();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

void KateQuickOpen::reselectFirst()
{
    int first = 0;
    if (m_mainWindow->viewManager()->sortedViews().size() > 1 && m_model->rowCount() > 1)
        first = 1;

    QModelIndex index = m_model->index(first, 0);
    m_listView->setCurrentIndex(index);
}

void KateQuickOpen::update()
{
    m_base_model->refresh();
    reselectFirst();

    updateViewGeometry();
    show();
    setFocus();
}

void KateQuickOpen::slotReturnPressed()
{
    const auto index = m_listView->model()->index(m_listView->currentIndex().row(), 0);
    auto url = index.data(Qt::UserRole).toUrl();
    m_mainWindow->wrapper()->openUrl(url);
    hide();
    m_mainWindow->slotWindowActivated();
    m_inputLine->clear();
}

void KateQuickOpen::slotfilterModeChanged(FilterModes mode)
{
    m_filterMode = mode;
    m_model->changeMode(mode);
    m_styleDelegate->changeMode(mode);
    m_model->invalidate();
}

void KateQuickOpen::slotListModeChanged(KateQuickOpenModel::List mode)
{
    m_base_model->setListMode(mode);
}

void KateQuickOpen::updateViewGeometry()
{
    const QSize centralSize = m_mainWindow->size();

    // width: 2.4 of editor, height: 1/2 of editor
    const QSize viewMaxSize(centralSize.width() / 2.4, centralSize.height() / 2);

    const int rowHeight = m_listView->sizeHintForRow(0) == -1 ? 0 : m_listView->sizeHintForRow(0);

    const int width = viewMaxSize.width();

    const QSize viewSize(std::max(300, width), // never go below this
                         std::min(std::max(rowHeight * m_base_model->rowCount() + 2, rowHeight * 6), viewMaxSize.height()));

    // Position should be central over window
    const int xPos = std::max(0, (centralSize.width() - viewSize.width()) / 2);
    const int yPos = std::max(0, (centralSize.height() - viewSize.height()) * 1 / 4);

    const QPoint p(xPos, yPos);
    move(p + m_mainWindow->pos());

    QPointer<QPropertyAnimation> animation = new QPropertyAnimation(this, "size");
    animation->setDuration(150);
    animation->setStartValue(this->size());
    animation->setEndValue(viewSize);

    animation->start();
}
