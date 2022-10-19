/*
    SPDX-FileCopyrightText: 2011-21 Kåre Särs <kare.sars@iki.fi>
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "ResultsTreeView.h"

#include "Results.h"

#include <QPushButton>

#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Editor>

ResultsTreeView::ResultsTreeView(QWidget *parent)
    : QTreeView(parent)
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
        auto topRight = viewport()->geometry().topRight();
        topRight.rx() -= 4;
        topRight.ry() += 4;
        auto btnGeometry = m_detachButton->geometry();
        btnGeometry.moveTopRight(topRight);
        m_detachButton->setGeometry(btnGeometry);
    });

    m_detachButton->setIcon(QIcon::fromTheme(QStringLiteral("draw-arrow")));
    m_detachButton->resize(m_detachButton->minimumSizeHint());
    connect(m_detachButton, &QAbstractButton::clicked, this, [this] {
        m_detachButton->setEnabled(false);
        m_detachButton->setVisible(false);
        Q_EMIT detachClicked();
    });
    m_detachButton->setVisible(false);

    auto *e = KTextEditor::Editor::instance();
    connect(e, &KTextEditor::Editor::configChanged, this, updateColors);
    updateColors(e);
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
QStyleOptionViewItem ResultsTreeView::viewOptions() const
{
    auto options = QTreeView::viewOptions();
    // We set this here so that the "expand triangle" in the treeview
    // is always colored in a visible color. This is important for
    // styles like fusion, where it can be dark on dark
    options.palette.setColor(QPalette::WindowText, m_fg);
    return options;
}
#else
void ResultsTreeView::initViewItemOption(QStyleOptionViewItem *option) const
{
    QTreeView::initViewItemOption(option);
    // We set this here so that the "expand triangle" in the treeview
    // is always colored in a visible color. This is important for
    // styles like fusion, where it can be dark on dark
    option->palette.setColor(QPalette::WindowText, m_fg);
}
#endif

void ResultsTreeView::resizeEvent(QResizeEvent *e)
{
    Q_EMIT geometryChanged();
    QTreeView::resizeEvent(e);
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
void ResultsTreeView::enterEvent(QEvent *event)
#else
void ResultsTreeView::enterEvent(QEnterEvent *event)
#endif
{
    auto *res = qobject_cast<Results *>(parent());
    if (!res) {
        qWarning() << Q_FUNC_INFO << "Unexpected null parent() Results";
        QTreeView::enterEvent(event);
        return;
    }
    m_detachButton->setVisible(!res->isEmpty() && !res->isDetachedToMainWindow);
    QTreeView::enterEvent(event);
}

void ResultsTreeView::leaveEvent(QEvent *e)
{
    m_detachButton->hide();
    QTreeView::leaveEvent(e);
}
