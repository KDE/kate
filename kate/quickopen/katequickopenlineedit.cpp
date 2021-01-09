/*  SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "katequickopenlineedit.h"

#include <QContextMenuEvent>
#include <QMenu>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

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
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup cg(cfg, "General");

    const int cfgFilterMode = cg.readEntry("Quickopen Filter Mode", (FilterMode::FilterByName | FilterMode::FilterByPath));
    const bool cfgListMode = cg.readEntry("Quickopen List Mode", true);

    menu->addSeparator();
    auto act1 = menu->addAction(i18n("Filter By Path"));
    act1->setCheckable(true);
    connect(act1, &QAction::toggled, this, [this](bool checked) {
        m_mode.setFlag(FilterMode::FilterByPath, checked);
        emit filterModeChanged(m_mode);
    });

    auto act2 = menu->addAction(i18n("Filter By Name"));
    act2->setCheckable(true);
    connect(act2, &QAction::toggled, this, [this](bool checked) {
        m_mode.setFlag(FilterMode::FilterByName, checked);
        emit filterModeChanged(m_mode);
    });

    if (cfgFilterMode == FilterMode::FilterByName) {
        act1->setChecked(true);
    } else if (cfgFilterMode == FilterMode::FilterByPath) {
        act2->setChecked(true);
    } else {
        act1->setChecked(true);
        act2->setChecked(true);
    }

    menu->addSeparator();

    QActionGroup *actGp = new QActionGroup(this);
    actGp->setExclusionPolicy(QActionGroup::ExclusionPolicy::Exclusive);

    auto act = menu->addAction(i18n("All Projects"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::AllProjects);
    });
    act->setChecked(!cfgListMode);

    actGp->addAction(act);

    act = menu->addAction(i18n("Current Project"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked)
            emit listModeChanged(KateQuickOpenModelList::CurrentProject);
    });
    act->setChecked(cfgListMode);

    actGp->addAction(act);
}
