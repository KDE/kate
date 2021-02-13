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
}

void QuickOpenLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    // on demand construction
    if (!menu) {
        menu.reset(createStandardContextMenu());
        setupMenu();
    }
    menu->exec(event->globalPos());
}

void QuickOpenLineEdit::setupMenu()
{
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup cg(cfg, "General");

    const bool cfgListMode = cg.readEntry("Quickopen List Mode", true);

    menu->addSeparator();

    QActionGroup *actGp = new QActionGroup(this);

    auto act = menu->addAction(i18n("All Projects"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            Q_EMIT listModeChanged(KateQuickOpenModelList::AllProjects);
        }
    });
    act->setChecked(!cfgListMode);

    actGp->addAction(act);

    act = menu->addAction(i18n("Current Project"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            Q_EMIT listModeChanged(KateQuickOpenModelList::CurrentProject);
        }
    });
    act->setChecked(cfgListMode);
    m_listMode = cfgListMode ? KateQuickOpenModelList::CurrentProject : KateQuickOpenModelList::AllProjects;

    actGp->addAction(act);
}
