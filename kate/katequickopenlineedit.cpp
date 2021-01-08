/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "katequickopenlineedit.h"

#include <QStyleOptionFocusRect>
#include <QStylePainter>
#include <QtDebug>
#include <QWindow>
#include <QMenu>

SwitchModeButton::SwitchModeButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::CursorShape::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    m_icon = QIcon::fromTheme(QStringLiteral("settings-configure"));
}

void SwitchModeButton::paintEvent(QPaintEvent *)
{
    const QPixmap iconPixmap = m_icon.pixmap(sizeHint(), QIcon::Normal);
    QStylePainter painter(this);
    QRect pixmapRect(QPoint(0,0), iconPixmap.size() / devicePixelRatio());
    pixmapRect.moveCenter(rect().center());

    painter.drawPixmap(pixmapRect, iconPixmap);
}

QuickOpenLineEdit::QuickOpenLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    m_button = new SwitchModeButton(this);
    m_menu = new QMenu(this);

    auto act = m_menu->addAction(QStringLiteral("Filter By Path"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked){
        m_mode.setFlag(FilterMode::FilterByPath, checked);
        emit filterModeChanged(m_mode);
    });
    act->setChecked(true);

    act = m_menu->addAction(QStringLiteral("Filter By Name"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked){
        m_mode.setFlag(FilterMode::FilterByName, checked);
        emit filterModeChanged(m_mode);
    });
    act->setChecked(true);

    m_menu->addSeparator();

    QActionGroup* actGp = new QActionGroup(this);
    actGp->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);

    act = m_menu->addAction(QStringLiteral("All Projects"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked){
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::AllProjects);
    });

    actGp->addAction(act);

    act = m_menu->addAction(QStringLiteral("Current Project"));
    connect(act, &QAction::toggled, this, [this](bool checked){
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::CurrentProject);
    });
    act->setCheckable(true);

    actGp->addAction(act);

    connect(m_button, &SwitchModeButton::clicked, this, &QuickOpenLineEdit::openMenu);
}

void QuickOpenLineEdit::updateViewGeometry()
{
    QMargins margins(0, 0, m_button->sizeHint().width(), 0);
    setTextMargins(margins);

    auto iconOffset = textMargins().right() + 8;
    m_button->setGeometry(rect().adjusted(width() - iconOffset, 0, 0, 0));
}

void QuickOpenLineEdit::resizeEvent(QResizeEvent*)
{
    updateViewGeometry();
}

void QuickOpenLineEdit::openMenu()
{
    int y = pos().y() + height();
    int x = pos().x() + width() - m_menu->sizeHint().width();
    m_menu->exec(mapToGlobal(QPoint(x, y)));
}
