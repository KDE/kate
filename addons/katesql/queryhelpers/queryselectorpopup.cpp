/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "queryselectorpopup.h"
#include "katesqlconstants.h"
#include "sqlqueryhighlighter.h"

#include <KLocalizedString>
#include <KTextEditor/Cursor>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KWindowSystem>

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QListView>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QScreen>
#include <QStringListModel>
#include <QVBoxLayout>

void QuerySelectorPopup::show(KTextEditor::View *view,
                              const QList<KTextEditor::Range> &queryRanges,
                              const QString &connection,
                              SQLQueryHighlighter *highlighter,
                              const QueryRunCallback &runCallback)
{
    auto *popup = new QuerySelectorPopup(view, queryRanges, connection, highlighter, runCallback);
    popup->QWidget::show();
    popup->m_listView->setFocus();
}

QuerySelectorPopup::QuerySelectorPopup(KTextEditor::View *view,
                                       const QList<KTextEditor::Range> &queryRanges,
                                       const QString &connection,
                                       SQLQueryHighlighter *highlighter,
                                       const QueryRunCallback &runCallback)
    : QFrame(view, Qt::Popup | Qt::FramelessWindowHint)
    , m_queryRanges(queryRanges)
    , m_listView(new QListView(this))
    , m_model(new QStringListModel(this))
    , m_view(view)
    , m_highlighter(highlighter)
    , m_connection(connection)
    , m_runSelectedQueryCallback(runCallback)

{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_listView);

    buildModel(queryRanges);
    applyStyle();
    connectSignals();
    positionPopup(view);

    m_listView->setCurrentIndex(m_model->index(0, 0));
    setFocusProxy(m_listView);
}

void QuerySelectorPopup::buildModel(const QList<KTextEditor::Range> &queryRanges)
{
    auto *doc = m_view->document();
    QStringList labels;

    for (int i = 0; i < queryRanges.size(); ++i) {
        QString queryText = doc->text(queryRanges[i]);
        queryText = queryText.simplified();
        const int maxLen = 120;
        if (queryText.length() > maxLen) {
            queryText = queryText.left(maxLen) + QStringLiteral("...");
        }

        if (i == queryRanges.size() - 1) {
            labels.append(i18nc("Full statement query option", "Full statement: %1", queryText));
        } else {
            int depth = queryRanges.size() - 1 - i;
            labels.append(i18nc("Subquery option with nesting level", "Subquery (depth %1): %2", depth, queryText));
        }
    }
    labels.append(i18nc("Run entire document option", "Entire document"));

    m_model->setStringList(labels);
    m_listView->setModel(m_model);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setMouseTracking(true);
}

void QuerySelectorPopup::applyStyle()
{
    const auto defaultPalette = qApp->palette();

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setColor(defaultPalette.color(QPalette::Dark));
    shadow->setOffset(0, 4);
    shadow->setBlurRadius(8);
    setGraphicsEffect(shadow);

    constexpr int borderWidth = KateSQLConstants::QueryPopupStyle::BorderWidth;
    constexpr int maxHeight = KateSQLConstants::QueryPopupStyle::MaximumHeight;

    constexpr int minWidth = KateSQLConstants::QueryPopupStyle::MinimumWidth;

    layout()->setContentsMargins(borderWidth, borderWidth, borderWidth, borderWidth);
    layout()->setSpacing(0);

    auto listPalette = m_listView->palette();
    listPalette.setColor(QPalette::ToolTipBase, defaultPalette.color(QPalette::ToolTipBase));
    m_listView->setPalette(listPalette);
    m_listView->setFrameShape(QFrame::NoFrame);

    QFont listFont = qApp->font();
    listFont.setPointSize(qRound(listFont.pointSize() * 1.3));
    m_listView->setFont(listFont);

    m_listView->setMinimumWidth(std::max(minWidth, m_view->width() / 3));

    const int rowCount = m_model->rowCount();
    const int rowHeight = m_listView->sizeHintForRow(0);
    const int totalHeight = rowCount * rowHeight + borderWidth * 2;
    this->setMaximumHeight(std::min(maxHeight, totalHeight));

    const int minCount = 2;
    const int minHeight = minCount * rowHeight + borderWidth * 2;
    this->setMinimumHeight(std::max(minHeight, std::min(totalHeight, maxHeight)));
    adjustSize();
}

void QuerySelectorPopup::positionPopup(KTextEditor::View *view)
{
    KTextEditor::Cursor anchorCursor = view->cursorPosition();
    QPoint screenPos = view->cursorToCoordinate(anchorCursor);
    if (!screenPos.isNull()) {
        screenPos = view->mapToGlobal(screenPos);
    } else {
        screenPos = view->mapToGlobal(view->rect().center());
    }

    QRect boundingGeom;
    if (KWindowSystem::isPlatformWayland()) {
        boundingGeom = view->window()->geometry();
    } else {
        QScreen *screen = QGuiApplication::screenAt(screenPos);
        if (screen) {
            boundingGeom = screen->availableGeometry();
        }
    }

    if (!boundingGeom.isNull()) {
        if (screenPos.x() + width() > boundingGeom.right()) {
            screenPos.setX(boundingGeom.right() - width());
        }
        if (screenPos.y() + height() > boundingGeom.bottom()) {
            screenPos.setY(boundingGeom.bottom() - height());
        }
        screenPos.setX(std::max(boundingGeom.left(), screenPos.x()));
        screenPos.setY(std::max(boundingGeom.top(), screenPos.y()));
    }
    move(screenPos);
}

void QuerySelectorPopup::connectSignals()
{
    // Preview on hover
    connect(m_listView, &QListView::entered, this, [this](const QModelIndex &index) {
        updatePreview(index.row());
    });

    // Preview on keyboard navigation
    connect(m_listView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &) {
        updatePreview(current.row());
    });

    // Accept on click
    connect(m_listView, &QListView::clicked, this, [this]() {
        acceptSelection();
    });

    // event filter for Enter/Escape
    m_listView->installEventFilter(this);
}

void QuerySelectorPopup::updatePreview(int row)
{
    if (row == m_previewIndex) {
        return;
    }
    m_previewIndex = row;

    if (row < m_queryRanges.size()) {
        if (m_highlighter) {
            m_highlighter->setPreviewRange(m_queryRanges[row]);
        }
    } else {
        // "Entire document" — no specific range to preview, clear highlight
        if (m_highlighter) {
            m_highlighter->clearPreview();
        }
    }
}

void QuerySelectorPopup::acceptSelection()
{
    int row = m_listView->currentIndex().row();
    if (row < 0) {
        return;
    }

    auto *doc = m_view->document();

    if (m_highlighter) {
        m_highlighter->setPreviewRange(KTextEditor::Range::invalid());
    }

    if (row < m_queryRanges.size()) {
        QString text = doc->text(m_queryRanges[row]).trimmed();
        if (!text.isEmpty()) {
            m_runSelectedQueryCallback(text, m_connection, false);
        }
    } else {
        // "Entire document" — Can be a 4gb mysqldump file
        // should use streaming to avoid loading the entire document into a QString.
        m_runSelectedQueryCallback({}, m_connection, true);
    }
    close();

    m_view->setFocus();
}

void QuerySelectorPopup::rejectPopup()
{
    if (m_highlighter) {
        m_highlighter->setPreviewRange(KTextEditor::Range::invalid());
    }
    close();
}

bool QuerySelectorPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            acceptSelection();
            return true;
        }
        if (ke->key() == Qt::Key_Escape) {
            rejectPopup();
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void QuerySelectorPopup::closeEvent(QCloseEvent *event)
{
    if (m_highlighter) {
        m_highlighter->setPreviewRange(KTextEditor::Range::invalid());
    }
    QFrame::closeEvent(event);
}

void QuerySelectorPopup::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    const auto defaultPalette = qApp->palette();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int borderRadius = KateSQLConstants::QueryPopupStyle::BorderRadius;
    QColor borderColor = defaultPalette.color(QPalette::Highlight);
    painter.setPen(Qt::NoPen);
    painter.setBrush(borderColor);
    painter.drawRoundedRect(rect(), borderRadius, borderRadius);

    const int borderWidth = KateSQLConstants::QueryPopupStyle::BorderWidth;
    QRect inner = rect().adjusted(borderWidth, borderWidth, -borderWidth, -borderWidth);
    painter.setBrush(defaultPalette.color(QPalette::Base));
    painter.drawRoundedRect(inner, borderRadius - borderWidth, borderRadius - borderWidth);
}

#include "moc_queryselectorpopup.cpp"
