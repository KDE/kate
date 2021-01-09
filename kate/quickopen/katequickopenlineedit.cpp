/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "katequickopenlineedit.h"

#include <QContextMenuEvent>
#include <QMenu>

#include <KLocalizedString>

QuickOpenLineEdit::QuickOpenLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    setPlaceholderText(i18n("Quick Open Search (configure via context menu)"));
    menu.reset(createStandardContextMenu());
    setupMenu();
}

void QuickOpenLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    menu->exec(event->globalPos());
}

void QuickOpenLineEdit::setupMenu()
{
    menu->addSeparator();
    auto act = menu->addAction(QStringLiteral("Filter By Path"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        m_mode.setFlag(FilterMode::FilterByPath, checked);
        emit filterModeChanged(m_mode);
    });
    act->setChecked(true);

    act = menu->addAction(QStringLiteral("Filter By Name"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        m_mode.setFlag(FilterMode::FilterByName, checked);
        emit filterModeChanged(m_mode);
    });
    act->setChecked(true);

    menu->addSeparator();

    QActionGroup *actGp = new QActionGroup(this);
    actGp->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);

    act = menu->addAction(QStringLiteral("All Projects"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::AllProjects);
    });

    actGp->addAction(act);

    act = menu->addAction(QStringLiteral("Current Project"));
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::CurrentProject);
    });
    act->setCheckable(true);

    actGp->addAction(act);
}
