/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "ResultsTreeView.h"

#include "Results.h"

#include <QPushButton>

#include <KLocalizedString>
#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>

ResultsTreeView::ResultsTreeView(QWidget *parent)
    : QTreeView(parent)
    , m_expandResultsButton(new QPushButton(this))
    , m_filterButton(new QPushButton(this))
    , m_detachButton(new QPushButton(this))
{
    auto updateColors = [this](KTextEditor::Editor *e) {
        if (!e) {
            return;
        }

        const auto theme = e->theme();
        auto base = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor));
        auto highlight = QColor::fromRgba(theme.editorColor(KSyntaxHighlighting::Theme::TextSelection));
        m_fg = QColor::fromRgba(theme.textColor(KSyntaxHighlighting::Theme::Normal));

        auto pal = palette();
        pal.setColor(QPalette::Base, base);
        pal.setColor(QPalette::Text, m_fg);
        pal.setColor(QPalette::Highlight, highlight);
        setPalette(pal);
    };

    connect(this, &ResultsTreeView::geometryChanged, this, [this] {
        updateFloatingButtonsGeometry();
    });

    m_detachButton->setIcon(QIcon::fromTheme(QStringLiteral("draw-arrow")));
    m_detachButton->setToolTip(i18n("Detach"));
    m_detachButton->resize(m_detachButton->minimumSizeHint());
    connect(m_detachButton, &QAbstractButton::clicked, this, [this] {
        m_detachButton->setEnabled(false);
        m_detachButton->setVisible(false);
        Q_EMIT detachClicked();
    });
    m_detachButton->setVisible(false);

    const auto btnSize = m_detachButton->size();

    m_filterButton->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    m_filterButton->setToolTip(i18n("Filter results"));
    m_filterButton->setCheckable(true);
    m_filterButton->resize(btnSize);
    m_filterButton->setVisible(false);

    m_expandResultsButton->setIcon(QIcon::fromTheme(QStringLiteral("view-list-tree"), QIcon::fromTheme(QStringLiteral("format-indent-more"))));
    m_expandResultsButton->setToolTip(i18n("Expand results"));
    m_expandResultsButton->setCheckable(true);
    m_expandResultsButton->resize(btnSize);
    m_expandResultsButton->setVisible(false);

    updateFloatingButtonsGeometry();

    auto *e = KTextEditor::Editor::instance();
    connect(e, &KTextEditor::Editor::configChanged, this, updateColors);
    updateColors(e);
}

void ResultsTreeView::updateFloatingButtonsGeometry()
{
    auto topRight = viewport()->geometry().topRight();
    topRight.rx() -= 4;
    topRight.ry() += 4;

    auto *res = qobject_cast<Results *>(parent());
    const bool canDetach = res && !res->isDetachedToMainWindow;

    int rightX = topRight.x();
    const int topY = topRight.y();

    if (canDetach) {
        auto detachGeom = m_detachButton->geometry();
        detachGeom.moveTopRight(QPoint(rightX, topY));
        m_detachButton->setGeometry(detachGeom);
        rightX = detachGeom.left() - 2;
    }

    auto filterGeom = m_filterButton->geometry();
    filterGeom.moveTopRight(QPoint(rightX, topY));
    m_filterButton->setGeometry(filterGeom);
    rightX = filterGeom.left() - 2;

    auto expandGeom = m_expandResultsButton->geometry();
    expandGeom.moveTopRight(QPoint(rightX, topY));
    m_expandResultsButton->setGeometry(expandGeom);
}

void ResultsTreeView::updateButtonsVisibility()
{
    auto *res = qobject_cast<Results *>(parent());
    if (!res) {
        return;
    }
    const bool hasResults = !res->isEmpty();
    const bool hovered = underMouse();

    m_detachButton->setVisible(hovered && hasResults && !res->isDetachedToMainWindow);
    m_expandResultsButton->setVisible(hovered && hasResults);
    m_filterButton->setVisible((hovered || m_filterButton->isChecked()) && hasResults);
}

void ResultsTreeView::initViewItemOption(QStyleOptionViewItem *option) const
{
    QTreeView::initViewItemOption(option);
    // We set this here so that the "expand triangle" in the treeview
    // is always colored in a visible color. This is important for
    // styles like fusion, where it can be dark on dark
    option->palette.setColor(QPalette::WindowText, m_fg);
}

void ResultsTreeView::resizeEvent(QResizeEvent *e)
{
    Q_EMIT geometryChanged();
    QTreeView::resizeEvent(e);
}

void ResultsTreeView::enterEvent(QEnterEvent *event)
{
    updateButtonsVisibility();
    QTreeView::enterEvent(event);
}

void ResultsTreeView::leaveEvent(QEvent *e)
{
    const auto pos = mapFromGlobal(QCursor::pos());
    if (rect().contains(pos)) {
        QTreeView::leaveEvent(e);
        return;
    }
    updateButtonsVisibility();
    QTreeView::leaveEvent(e);
}

#include "moc_ResultsTreeView.cpp"
