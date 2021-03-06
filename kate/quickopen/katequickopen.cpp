/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2007, 2009 Joseph Wenninger <jowenn@kde.org>
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

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
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

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

class QuickOpenFilterProxyModel final : public QSortFilterProxyModel
{
public:
    QuickOpenFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

protected:
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override
    {
        auto sm = sourceModel();
        const int l = static_cast<KateQuickOpenModel *>(sm)->idxScore(sourceLeft);
        const int r = static_cast<KateQuickOpenModel *>(sm)->idxScore(sourceRight);
        return l < r;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &) const override
    {
        if (pattern.isEmpty()) {
            return true;
        }

        auto sm = static_cast<KateQuickOpenModel *>(sourceModel());
        if (!sm->isValid(sourceRow)) {
            return false;
        }

        const QString &name = sm->idxToFileName(sourceRow);

        int score = 0;
        bool res = false;
        int scorep = 0, scoren = 0;
        bool resn = filterByName(name, scoren);

        // only match file path if filename got a match
        bool resp = false;
        if (resn || pathLike) {
            const QString &path = sm->idxToFilePath(sourceRow);
            resp = filterByPath(path, scorep);
        }

        // store the score for sorting later
        score = scoren + scorep;
        res = resp || resn;

        sm->setScoreForIndex(sourceRow, score);

        return res;
    }

public Q_SLOTS:
    void setFilterText(const QString &text)
    {
        beginResetModel();
        pattern = text;
        pathLike = pattern.contains(QLatin1Char('/'));
        endResetModel();
    }

private:
    inline bool filterByPath(const QString &path, int &score) const
    {
        return kfts::fuzzy_match(pattern, path, score);
    }

    inline bool filterByName(const QString &name, int &score) const
    {
        return kfts::fuzzy_match(pattern, name, score);
    }

private:
    QString pattern;
    bool pathLike = false;
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

        QString name = index.data(KateQuickOpenModel::FileName).toString();
        QString path = index.data(KateQuickOpenModel::FilePath).toString();

        path.remove(QStringLiteral("/") + name);

        const QString nameColor = option.palette.color(QPalette::Link).name();

        QTextCharFormat fmt;
        fmt.setForeground(options.palette.link().color());
        fmt.setFontWeight(QFont::Bold);

        const int nameLen = name.length();
        // space between name and path
        constexpr int space = 1;
        QVector<QTextLayout::FormatRange> formats;

        // collect formats
        int pos = m_filterString.lastIndexOf(QLatin1Char('/'));
        if (pos > -1) {
            ++pos;
            auto pattern = m_filterString.midRef(pos);
            auto nameFormats = kfts::get_fuzzy_match_formats(pattern, name, 0, fmt);
            formats.append(nameFormats);
        } else {
            auto nameFormats = kfts::get_fuzzy_match_formats(m_filterString, name, 0, fmt);
            formats.append(nameFormats);
        }
        QTextCharFormat boldFmt;
        boldFmt.setFontWeight(QFont::Bold);
        boldFmt.setFontPointSize(options.font.pointSize() - 1);
        auto pathFormats = kfts::get_fuzzy_match_formats(m_filterString, name, nameLen + space, boldFmt);
        QTextCharFormat gray;
        gray.setForeground(Qt::gray);
        gray.setFontPointSize(options.font.pointSize() - 1);
        formats.append({nameLen + space, path.length(), gray});
        formats.append(pathFormats);

        painter->save();

        // paint background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(option.rect, option.palette.base());
        }

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        // space for icon
        painter->translate(25, 0);

        // draw text
        kfts::paintItemViewText(painter, QString(name + QStringLiteral(" ") + path), options, formats);

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
    m_listView->setUniformRowHeights(true);

    m_base_model = new KateQuickOpenModel(this);

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
        reselectFirst(); // hacky way
    });
    connect(m_inputLine, &QuickOpenLineEdit::returnPressed, this, &KateQuickOpen::slotReturnPressed);
    connect(m_inputLine, &QuickOpenLineEdit::listModeChanged, this, &KateQuickOpen::slotListModeChanged);

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

    slotListModeChanged(m_inputLine->listMode());

    // fill stuff
    update(mainWindow);
}

KateQuickOpen::~KateQuickOpen()
{
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup cg(cfg, "General");

    cg.writeEntry("Quickopen List Mode", m_base_model->listMode() == KateQuickOpenModelList::CurrentProject);
}

bool KateQuickOpen::eventFilter(QObject *obj, QEvent *event)
{
    // catch key presses + shortcut overrides to allow to have ESC as application wide shortcut, too, see bug 409856
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == m_inputLine) {
            const bool forward2list = (keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down) || (keyEvent->key() == Qt::Key_PageUp)
                || (keyEvent->key() == Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(m_listView, event);
                return true;
            }

            if (keyEvent->key() == Qt::Key_Escape) {
                m_mainWindow->slotWindowActivated();
                {
                    m_inputLine->blockSignals(true);
                    m_inputLine->clear();
                    m_inputLine->blockSignals(false);
                }
                keyEvent->accept();
                hide();
                return true;
            }
        } else {
            const bool forward2input = (keyEvent->key() != Qt::Key_Up) && (keyEvent->key() != Qt::Key_Down) && (keyEvent->key() != Qt::Key_PageUp)
                && (keyEvent->key() != Qt::Key_PageDown) && (keyEvent->key() != Qt::Key_Tab) && (keyEvent->key() != Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(m_inputLine, event);
                return true;
            }
        }
    }

    // hide on focus out, if neither input field nor list have focus!
    else if (event->type() == QEvent::FocusOut && !(m_inputLine->hasFocus() || m_listView->hasFocus())) {
        m_mainWindow->slotWindowActivated();
        {
            m_inputLine->blockSignals(true);
            m_inputLine->clear();
            m_inputLine->blockSignals(false);
        }
        hide();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

void KateQuickOpen::reselectFirst()
{
    int first = 0;
    if (m_mainWindow->viewManager()->sortedViews().size() > 1 && m_model->rowCount() > 1) {
        first = 1;
    }

    QModelIndex index = m_model->index(first, 0);
    m_listView->setCurrentIndex(index);
}

void KateQuickOpen::update(KateMainWindow *mainWindow)
{
    m_base_model->refresh(mainWindow);
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

    // block signals for input line so that we dont trigger filtering again
    const QSignalBlocker blocker(m_inputLine);
    m_inputLine->clear();
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

    // fix position and size
    const QPoint p(xPos, yPos);
    move(p + m_mainWindow->pos());
    setFixedSize(viewSize);
}
