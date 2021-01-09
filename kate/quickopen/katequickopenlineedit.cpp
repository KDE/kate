/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "katequickopenlineedit.h"

#include <QContextMenuEvent>
#include <QMenu>

#include <KLocalizedString>

QuickOpenLineEdit::QuickOpenLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setPlaceholderText(i18n("Quick Open Search (configure via context menu)"));
}

void QuickOpenLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    // standard stuff like copy & paste...
    QMenu *contextMenu = createStandardContextMenu();

    QMenu* menu = new QMenu(QStringLiteral("Filter..."));

    // our configuration actions
    menu->addSeparator();
    auto act = menu->addAction(QStringLiteral("Filter By Path"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked){
        m_mode.setFlag(FilterMode::FilterByPath, checked);
        emit filterModeChanged(m_mode);
    });
    act->setChecked(true);

    act = menu->addAction(QStringLiteral("Filter By Name"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked){
        m_mode.setFlag(FilterMode::FilterByName, checked);
        emit filterModeChanged(m_mode);
    });
    act->setChecked(true);

    menu->addSeparator();

    QActionGroup* actGp = new QActionGroup(this);
    actGp->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);

    act = menu->addAction(QStringLiteral("All Projects"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked){
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::AllProjects);
    });
    act->setChecked(true);

    actGp->addAction(act);

    act = menu->addAction(QStringLiteral("Current Project"));
    connect(act, &QAction::toggled, this, [this](bool checked){
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::CurrentProject);
    });
    act->setCheckable(true);

    actGp->addAction(act);

    contextMenu->addMenu(menu);

    contextMenu->exec(event->globalPos());
    delete contextMenu;
}
