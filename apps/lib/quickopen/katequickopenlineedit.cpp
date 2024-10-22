/*
    SPDX-FileCopyrightText: 2020 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "katequickopenlineedit.h"

#include <QActionGroup>
#include <QContextMenuEvent>
#include <QMenu>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

static const char CONFIG_QUICKOPEN_LISTMODE[] = {"Quickopen List Mode"};
static const char CONFIG_QUICKOPEN_FILTERMODE[] = {"Quickopen Filter Mode"};

QuickOpenLineEdit::QuickOpenLineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    // ensure config is read (menu only created upon demand)
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup cg(cfg, QStringLiteral("General"));

    const bool cfgListMode = cg.readEntry(CONFIG_QUICKOPEN_LISTMODE, true);
    m_listMode = cfgListMode ? KateQuickOpenModelList::CurrentProject : KateQuickOpenModelList::AllProjects;

    updatePlaceholderText(m_listMode);

    m_filterMode = (FilterMode)cg.readEntry(CONFIG_QUICKOPEN_FILTERMODE, (int)Fuzzy);
}

QuickOpenLineEdit::~QuickOpenLineEdit()
{
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup cg(cfg, QStringLiteral("General"));

    cg.writeEntry(CONFIG_QUICKOPEN_LISTMODE, m_listMode == KateQuickOpenModelList::CurrentProject);
    cg.writeEntry(CONFIG_QUICKOPEN_FILTERMODE, (int)m_filterMode);
}

void QuickOpenLineEdit::updatePlaceholderText(KateQuickOpenModelList mode)
{
    if (mode == KateQuickOpenModelList::AllProjects) {
        setPlaceholderText(
            i18nc("The text is shown as a placeholder in an input line edit, keep it short", "Search files in all projects (right click to configure)"));
    } else {
        setPlaceholderText(
            i18nc("The text is shown as a placeholder in an input line edit, keep it short", "Search files in current project (right click to configure)"));
    }
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
    const bool cfgListMode = m_listMode == CurrentProject;

    menu->addSeparator();

    QActionGroup *actGp = new QActionGroup(this);

    auto act = menu->addAction(i18n("All Projects"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            m_listMode = AllProjects;
            Q_EMIT listModeChanged(KateQuickOpenModelList::AllProjects);
            updatePlaceholderText(m_listMode);
        }
    });
    act->setChecked(!cfgListMode);

    actGp->addAction(act);

    act = menu->addAction(i18n("Current Project"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            m_listMode = CurrentProject;
            Q_EMIT listModeChanged(KateQuickOpenModelList::CurrentProject);
            updatePlaceholderText(m_listMode);
        }
    });
    act->setChecked(cfgListMode);
    actGp->addAction(act);

    // filter mode
    menu->addSeparator();

    actGp = new QActionGroup(this);
    act = menu->addAction(i18n("Fuzzy Filtering"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            m_filterMode = Fuzzy;
            Q_EMIT filterModeChanged();
        }
    });
    act->setChecked(m_filterMode == Fuzzy);
    actGp->addAction(act);

    act = menu->addAction(i18n("Wildcard Filtering"));
    act->setCheckable(true);
    connect(act, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            m_filterMode = Wildcard;
            Q_EMIT filterModeChanged();
        }
    });
    act->setChecked(m_filterMode == Wildcard);
    actGp->addAction(act);
}

#include "moc_katequickopenlineedit.cpp"
