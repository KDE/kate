/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002, 2003 Joseph Wenninger <jowenn@kde.org>

   GUIClient partly based on ktoolbarhandler.cpp: Copyright (C) 2002 Simon Hausmann <hausmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katemdi.h"

#include "katedebug.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KMessageBox>
#include <KXMLGUIFactory>
#include <KLocalizedString>
#include <KSharedConfig>
#include <kwindowconfig.h>
#include <KToolBar>

#include <QChildEvent>
#include <QEvent>
#include <QContextMenuEvent>
#include <QPixmap>
#include <QMenu>
#include <QShortcut>
#include <QSizePolicy>
#include <QStyle>
#include <QDomDocument>

namespace KateMDI
{

//BEGIN TOGGLETOOLVIEWACTION
//
ToggleToolViewAction::ToggleToolViewAction(const QString &text, ToolView *tv,
        QObject *parent)
    : KToggleAction(text, parent)
    , m_tv(tv)
{
    connect(this, &ToggleToolViewAction::toggled, this, &ToggleToolViewAction::slotToggled);
    connect(m_tv, &ToolView::toolVisibleChanged, this, &ToggleToolViewAction::toolVisibleChanged);

    setChecked(m_tv->toolVisible());
}

ToggleToolViewAction::~ToggleToolViewAction()
{}

void ToggleToolViewAction::toolVisibleChanged(bool)
{
    if (isChecked() != m_tv->toolVisible()) {
        setChecked(m_tv->toolVisible());
    }
}

void ToggleToolViewAction::slotToggled(bool t)
{
    if (t) {
        m_tv->mainWindow()->showToolView(m_tv);
        m_tv->setFocus();
    } else {
        m_tv->mainWindow()->hideToolView(m_tv);
    }
}

//END TOGGLETOOLVIEWACTION

//BEGIN GUICLIENT

static const QString actionListName = QStringLiteral("kate_mdi_view_actions");

// please don't use QStringLiteral since it can't be used with a concatenated string parameter on all platforms
static const QString guiDescription = QLatin1String(""
                                      "<!DOCTYPE kpartgui><kpartgui name=\"kate_mdi_view_actions\">"
                                      "<MenuBar>"
                                      "    <Menu name=\"view\">"
                                      "        <ActionList name=\"%1\" />"
                                      "    </Menu>"
                                      "</MenuBar>"
                                      "</kpartgui>");

GUIClient::GUIClient(MainWindow *mw)
    : QObject(mw)
    , KXMLGUIClient(mw)
    , m_mw(mw)
{
    connect(m_mw->guiFactory(), SIGNAL(clientAdded(KXMLGUIClient*)),
            this, SLOT(clientAdded(KXMLGUIClient*)));

    if (domDocument().documentElement().isNull()) {
        QString completeDescription = guiDescription.arg(actionListName);

        setXML(completeDescription, false /*merge*/);
    }

    m_toolMenu = new KActionMenu(i18n("Tool &Views"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_toolview_menu"), m_toolMenu);
    m_showSidebarsAction = new KToggleAction(i18n("Show Side&bars"), this);
    actionCollection()->addAction(QStringLiteral("kate_mdi_sidebar_visibility"), m_showSidebarsAction);
    actionCollection()->setDefaultShortcut(m_showSidebarsAction, Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_F);

    m_showSidebarsAction->setChecked(m_mw->sidebarsVisible());
    connect(m_showSidebarsAction, &KToggleAction::toggled, m_mw, &MainWindow::setSidebarsVisible);

    m_toolMenu->addAction(m_showSidebarsAction);
    QAction *sep_act = new QAction(this);
    sep_act->setSeparator(true);
    m_toolMenu->addAction(sep_act);

    // read shortcuts
    actionCollection()->setConfigGroup(QStringLiteral("Shortcuts"));
    actionCollection()->readSettings();

    actionCollection()->addAssociatedWidget(m_mw);
    foreach(QAction * action, actionCollection()->actions())
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

GUIClient::~GUIClient()
{}

void GUIClient::updateSidebarsVisibleAction()
{
    m_showSidebarsAction->setChecked(m_mw->sidebarsVisible());
}

void GUIClient::registerToolView(ToolView *tv)
{
    QString aname = QString::fromLatin1("kate_mdi_toolview_") + tv->id;

    // try to read the action shortcut
    QList<QKeySequence> shortcuts;

    KSharedConfigPtr cfg = KSharedConfig::openConfig();
    QString shortcutString = cfg->group("Shortcuts").readEntry(aname, QString());

    foreach(const QString & shortcut, shortcutString.split(QLatin1String(";"))) {
        shortcuts << QKeySequence::fromString(shortcut);
    }

    KToggleAction *a = new ToggleToolViewAction(i18n("Show %1", tv->text), tv, this);
    actionCollection()->setDefaultShortcuts(a, shortcuts);
    actionCollection()->addAction(aname, a);

    m_toolViewActions.append(a);
    m_toolMenu->addAction(a);

    m_toolToAction.insert(tv, a);

    updateActions();
}

void GUIClient::unregisterToolView(ToolView *tv)
{
    QAction *a = m_toolToAction[tv];

    if (!a) {
        return;
    }

    m_toolViewActions.removeAt(m_toolViewActions.indexOf(a));
    delete a;

    m_toolToAction.remove(tv);

    updateActions();
}

void GUIClient::clientAdded(KXMLGUIClient *client)
{
    if (client == this) {
        updateActions();
    }
}

void GUIClient::updateActions()
{
    if (!factory()) {
        return;
    }

    unplugActionList(actionListName);

    QList<QAction *> addList;
    addList.append(m_toolMenu);

    plugActionList(actionListName, addList);
}

//END GUICLIENT

//BEGIN TOOLVIEW

ToolView::ToolView(MainWindow *mainwin, Sidebar *sidebar, QWidget *parent)
    : QFrame(parent)
    , m_mainWin(mainwin)
    , m_sidebar(sidebar)
    , m_toolbar(nullptr)
    , m_toolVisible(false)
    , persistent(false)
{
    // try to fix resize policy
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setRetainSizeWhenHidden(true);
    setSizePolicy(policy);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    m_toolbar = new KToolBar(this);
    m_toolbar->setVisible(false);
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
}

QSize ToolView::sizeHint() const
{
    return size();
}

QSize ToolView::minimumSizeHint() const
{
    return QSize(160, 160);
}

ToolView::~ToolView()
{
    m_mainWin->toolViewDeleted(this);
}

void ToolView::setToolVisible(bool vis)
{
    if (m_toolVisible == vis) {
        return;
    }

    m_toolVisible = vis;
    emit toolVisibleChanged(m_toolVisible);
}

bool ToolView::toolVisible() const
{
    return m_toolVisible;
}

void ToolView::childEvent(QChildEvent *ev)
{
    // set the widget to be focus proxy if possible
    if ((ev->type() == QEvent::ChildAdded) && qobject_cast<QWidget *>(ev->child())) {
        QWidget *widget = qobject_cast<QWidget *>(ev->child());
        setFocusProxy(widget);
        layout()->addWidget(widget);
    }

    QFrame::childEvent(ev);
}

void ToolView::actionEvent(QActionEvent* event)
{
    QFrame::actionEvent(event);
    if (event->type() == QEvent::ActionAdded) {
        m_toolbar->addAction(event->action());
    } else if (event->type() == QEvent::ActionRemoved) {
        m_toolbar->removeAction(event->action());
    }
    m_toolbar->setVisible(!m_toolbar->actions().isEmpty());
}

//END TOOLVIEW

//BEGIN SIDEBAR

Sidebar::Sidebar(KMultiTabBar::KMultiTabBarPosition pos, MainWindow *mainwin, QWidget *parent)
    : KMultiTabBar(pos, parent)
    , m_mainWin(mainwin)
    , m_splitter(nullptr)
    , m_ownSplit(nullptr)
    , m_lastSize(0)
{
    hide();
}

Sidebar::~Sidebar()
{}

void Sidebar::setSplitter(QSplitter *sp)
{
    m_splitter = sp;
    m_ownSplit = new QSplitter((position() == KMultiTabBar::Top || position() == KMultiTabBar::Bottom) ? Qt::Horizontal : Qt::Vertical, m_splitter);
    m_ownSplit->setChildrenCollapsible(false);
    m_ownSplit->hide();
}

ToolView *Sidebar::addWidget(const QIcon &icon, const QString &text, ToolView *widget)
{
    static int id = 0;

    if (widget) {
        if (widget->sidebar() == this) {
            return widget;
        }

        widget->sidebar()->removeWidget(widget);
    }

    int newId = ++id;

    appendTab(icon, newId, text);

    if (!widget) {
        widget = new ToolView(m_mainWin, this, m_ownSplit);
        widget->hide();
        widget->icon = icon;
        widget->text = text;
    } else {
        widget->hide();
        widget->setParent(m_ownSplit);
        widget->m_sidebar = this;
    }

    // save its pos ;)
    widget->persistent = false;

    m_idToWidget.insert(newId, widget);
    m_widgetToId.insert(widget, newId);
    m_toolviews.push_back(widget);

    // widget => size, for correct size restoration after hide/show
    // starts with invalid size
    m_widgetToSize.insert(widget, QSize());

    show();

    connect(tab(newId), SIGNAL(clicked(int)), this, SLOT(tabClicked(int)));
    tab(newId)->installEventFilter(this);

    return widget;
}

bool Sidebar::removeWidget(ToolView *widget)
{
    if (!m_widgetToId.contains(widget)) {
        return false;
    }

    removeTab(m_widgetToId[widget]);

    m_idToWidget.remove(m_widgetToId[widget]);
    m_widgetToId.remove(widget);
    m_widgetToSize.remove(widget);
    m_toolviews.removeAt(m_toolviews.indexOf(widget));

    bool anyVis = false;
    QMapIterator<int, ToolView *> it(m_idToWidget);
    while (it.hasNext()) {
        it.next();
        if ((anyVis = it.value()->isVisible())) {
            break;
        }
    }

    if (m_idToWidget.isEmpty()) {
        m_ownSplit->hide();
        hide();
    } else if (!anyVis) {
        m_ownSplit->hide();
    }

    return true;
}

bool Sidebar::showWidget(ToolView *widget)
{
    if (!m_widgetToId.contains(widget)) {
        return false;
    }

    // hide other non-persistent views
    QMapIterator<int, ToolView *> it(m_idToWidget);
    while (it.hasNext()) {
        it.next();
        if ((it.value() != widget) && !it.value()->persistent) {
            hideWidget(it.value());
        }
    }

    setTab(m_widgetToId[widget], true);

    /**
     * resize to right size again and show, else artefacts
     */
    if (m_widgetToSize[widget].isValid()) {
        widget->resize(m_widgetToSize[widget]);
    }

    /**
     * resize to right size again and show, else artefacts
     * same as for widget, both needed
     */
    if (m_preHideSize.isValid()) {
        widget->resize(m_preHideSize);
        m_ownSplit->resize(m_preHideSize);
    }
    m_ownSplit->show();
    widget->show();

    /**
     * we are visible again!
     */
    widget->setToolVisible(true);
    return true;
}

bool Sidebar::hideWidget(ToolView *widget)
{
    if (!m_widgetToId.contains(widget)) {
        return false;
    }

    bool anyVis = false;

    updateLastSize();

    QMapIterator<int, ToolView *> it(m_idToWidget);
    while (it.hasNext()) {
        it.next();
        if (it.value() == widget) {
            // remember size and hide
            if (widget->isVisible()) {
                m_widgetToSize[widget] = widget->size();
            }
        } else if ((anyVis = it.value()->isVisible())) {
            break;
        }
    }

    widget->hide();

    // lower tab
    setTab(m_widgetToId[widget], false);

    if (!anyVis) {
        if (m_ownSplit->isVisible()) {
            m_preHideSize = m_ownSplit->size();
        }
        m_ownSplit->hide();
    }

    widget->setToolVisible(false);
    return true;
}

void Sidebar::tabClicked(int i)
{
    ToolView *w = m_idToWidget[i];

    if (!w) {
        return;
    }

    if (isTabRaised(i)) {
        showWidget(w);
        w->setFocus();
    } else {
        hideWidget(w);
    }
}

bool Sidebar::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::ContextMenu) {
        QContextMenuEvent *e = (QContextMenuEvent *) ev;
        KMultiTabBarTab *bt = dynamic_cast<KMultiTabBarTab *>(obj);
        if (bt) {
            //qCDebug(LOG_KATE) << "Request for popup";

            m_popupButton = bt->id();

            ToolView *w = m_idToWidget[m_popupButton];

            if (w) {
                QMenu *menu = new QMenu(this);

                if (!w->plugin.isNull()) {
                    if (w->plugin.data()->configPages() > 0) {
                        menu->addAction(i18n("Configure ..."))->setData(20);
                    }
                }

                menu->addSection(QIcon::fromTheme(QStringLiteral("view_remove")), i18n("Behavior"));

                menu->addAction(w->persistent ? QIcon::fromTheme(QStringLiteral("view-restore")) : QIcon::fromTheme(QStringLiteral("view-fullscreen")),
                                w->persistent ? i18n("Make Non-Persistent") : i18n("Make Persistent")) -> setData(10);

                menu->addSection(QIcon::fromTheme(QStringLiteral("move")), i18n("Move To"));

                if (position() != 0) {
                    menu->addAction(QIcon::fromTheme(QStringLiteral("go-previous")), i18n("Left Sidebar"))->setData(0);
                }

                if (position() != 1) {
                    menu->addAction(QIcon::fromTheme(QStringLiteral("go-next")), i18n("Right Sidebar"))->setData(1);
                }

                if (position() != 2) {
                    menu->addAction(QIcon::fromTheme(QStringLiteral("go-up")), i18n("Top Sidebar"))->setData(2);
                }

                if (position() != 3) {
                    menu->addAction(QIcon::fromTheme(QStringLiteral("go-down")), i18n("Bottom Sidebar"))->setData(3);
                }

                connect(menu, &QMenu::triggered, this, &Sidebar::buttonPopupActivate);

                menu->exec(e->globalPos());
                delete menu;

                return true;
            }
        }
    }

    return false;
}

void Sidebar::setVisible(bool visible)
{
    // visible==true means show-request
    if (visible && (m_idToWidget.isEmpty() || !m_mainWin->sidebarsVisible())) {
        return;
    }

    KMultiTabBar::setVisible(visible);
}

void Sidebar::buttonPopupActivate(QAction *a)
{
    int id = a->data().toInt();
    ToolView *w = m_idToWidget[m_popupButton];

    if (!w) {
        return;
    }

    // move ids
    if (id < 4) {
        // move + show ;)
        m_mainWin->moveToolView(w, (KMultiTabBar::KMultiTabBarPosition) id);
        m_mainWin->showToolView(w);
    }

    // toggle persistent
    if (id == 10) {
        w->persistent = !w->persistent;
    }

    // configure actionCollection
    if (id == 20) {
        if (!w->plugin.isNull()) {
            if (w->plugin.data()->configPages() > 0) {
                emit sigShowPluginConfigPage(w->plugin.data(), 0);
            }
        }
    }
}

void Sidebar::updateLastSize()
{
    QList<int> s = m_splitter->sizes();

    int i = 0;
    if ((position() == KMultiTabBar::Right || position() == KMultiTabBar::Bottom)) {
        i = 2;
    }

    // little threshold
    if (s[i] > 2) {
        m_lastSize = s[i];
    }
}

class TmpToolViewSorter
{
public:
    ToolView *tv;
    unsigned int pos;
};

void Sidebar::restoreSession(KConfigGroup &config)
{
    // get the last correct placed toolview
    int firstWrong = 0;
    for (; firstWrong < m_toolviews.size(); ++firstWrong) {
        ToolView *tv = m_toolviews[firstWrong];

        int pos = config.readEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Sidebar-Position").arg(tv->id), firstWrong);

        if (pos != firstWrong) {
            break;
        }
    }

    // we need to reshuffle, ahhh :(
    if (firstWrong < m_toolviews.size()) {
        // first: collect the items to reshuffle
        QList<TmpToolViewSorter> toSort;
        for (int i = firstWrong; i < m_toolviews.size(); ++i) {
            TmpToolViewSorter s;
            s.tv = m_toolviews[i];
            s.pos = config.readEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Sidebar-Position").arg(m_toolviews[i]->id), i);
            toSort.push_back(s);
        }

        // now: sort the stuff we need to reshuffle
        for (int m = 0; m < toSort.size(); ++m)
            for (int n = m + 1; n < toSort.size(); ++n)
                if (toSort[n].pos < toSort[m].pos) {
                    TmpToolViewSorter tmp = toSort[n];
                    toSort[n] = toSort[m];
                    toSort[m] = tmp;
                }

        // then: remove this items from the button bar
        // do this backwards, to minimize the relayout efforts
        for (int i = m_toolviews.size() - 1; i >= (int)firstWrong; --i) {
            removeTab(m_widgetToId[m_toolviews[i]]);
        }

        // insert the reshuffled things in order :)
        for (int i = 0; i < toSort.size(); ++i) {
            ToolView *tv = toSort[i].tv;

            m_toolviews[firstWrong + i] = tv;

            // readd the button
            int newId = m_widgetToId[tv];
            appendTab(tv->icon, newId, tv->text);
            connect(tab(newId), SIGNAL(clicked(int)), this, SLOT(tabClicked(int)));
            tab(newId)->installEventFilter(this);

            // reshuffle in splitter: move to last
            m_ownSplit->addWidget(tv);
        }
    }

    // update last size if needed
    updateLastSize();

    // restore the own splitter sizes
    QList<int> s = config.readEntry(QString::fromLatin1("Kate-MDI-Sidebar-%1-Splitter").arg(position()), QList<int>());
    m_ownSplit->setSizes(s);

    // show only correct toolviews, remember persistent values ;)
    bool anyVis = false;
    for (int i = 0; i < m_toolviews.size(); ++i) {
        ToolView *tv = m_toolviews[i];

        tv->persistent = config.readEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Persistent").arg(tv->id), false);
        tv->setToolVisible(config.readEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Visible").arg(tv->id), false));

        if (!anyVis) {
            anyVis = tv->toolVisible();
        }

        setTab(m_widgetToId[tv], tv->toolVisible());

        if (tv->toolVisible()) {
            tv->show();
        } else {
            tv->hide();
        }
    }

    if (anyVis) {
        m_ownSplit->show();
    } else {
        m_ownSplit->hide();
    }
}

void Sidebar::saveSession(KConfigGroup &config)
{
    // store the own splitter sizes
    QList<int> s = m_ownSplit->sizes();
    config.writeEntry(QString::fromLatin1("Kate-MDI-Sidebar-%1-Splitter").arg(position()), s);

    // store the data about all toolviews in this sidebar ;)
    for (int i = 0; i < m_toolviews.size(); ++i) {
        ToolView *tv = m_toolviews[i];

        config.writeEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Position").arg(tv->id), int(tv->sidebar()->position()));
        config.writeEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Sidebar-Position").arg(tv->id), i);
        config.writeEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Visible").arg(tv->id), tv->toolVisible());
        config.writeEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Persistent").arg(tv->id), tv->persistent);
    }
}

//END SIDEBAR

//BEGIN MAIN WINDOW

MainWindow::MainWindow(QWidget *parentWidget)
    : KParts::MainWindow(parentWidget, Qt::Window)
    , m_sidebarsVisible(true)
    , m_restoreConfig(nullptr)
    , m_guiClient(new GUIClient(this))
{
    // init the internal widgets
    QFrame *hb = new QFrame(this);
    QHBoxLayout *hlayout = new QHBoxLayout(hb);
    hlayout->setMargin(0);
    hlayout->setSpacing(0);

    setCentralWidget(hb);

    m_sidebars[KMultiTabBar::Left] = new Sidebar(KMultiTabBar::Left, this, hb);
    hlayout->addWidget(m_sidebars[KMultiTabBar::Left]);

    m_hSplitter = new QSplitter(Qt::Horizontal, hb);
    hlayout->addWidget(m_hSplitter);

    m_sidebars[KMultiTabBar::Left]->setSplitter(m_hSplitter);

    QFrame *vb = new QFrame(m_hSplitter);
    QVBoxLayout *vlayout = new QVBoxLayout(vb);
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    m_hSplitter->setCollapsible(m_hSplitter->indexOf(vb), false);
    m_hSplitter->setStretchFactor(m_hSplitter->indexOf(vb), 1);

    m_sidebars[KMultiTabBar::Top] = new Sidebar(KMultiTabBar::Top, this, vb);
    vlayout->addWidget(m_sidebars[KMultiTabBar::Top]);

    m_vSplitter = new QSplitter(Qt::Vertical, vb);
    vlayout->addWidget(m_vSplitter);

    m_sidebars[KMultiTabBar::Top]->setSplitter(m_vSplitter);

    m_centralWidget = new QWidget(m_vSplitter);
    m_centralWidget->setLayout(new QVBoxLayout);
    m_centralWidget->layout()->setSpacing(0);
    m_centralWidget->layout()->setMargin(0);

    m_vSplitter->setCollapsible(m_vSplitter->indexOf(m_centralWidget), false);
    m_vSplitter->setStretchFactor(m_vSplitter->indexOf(m_centralWidget), 1);

    m_sidebars[KMultiTabBar::Bottom] = new Sidebar(KMultiTabBar::Bottom, this, vb);
    vlayout->addWidget(m_sidebars[KMultiTabBar::Bottom]);
    m_sidebars[KMultiTabBar::Bottom]->setSplitter(m_vSplitter);

    m_sidebars[KMultiTabBar::Right] = new Sidebar(KMultiTabBar::Right, this, hb);
    hlayout->addWidget(m_sidebars[KMultiTabBar::Right]);
    m_sidebars[KMultiTabBar::Right]->setSplitter(m_hSplitter);

    for (int i = 0; i < 4; i++) {
        connect(m_sidebars[i], SIGNAL(sigShowPluginConfigPage(KTextEditor::Plugin*,uint)), this, SIGNAL(sigShowPluginConfigPage(KTextEditor::Plugin*,uint)));
    }

}

MainWindow::~MainWindow()
{
    // cu toolviews
    qDeleteAll(m_toolviews);

    // seems like we really should delete this by hand ;)
    delete m_centralWidget;

    // cleanup the sidebars
    for (unsigned int i = 0; i < 4; ++i) {
        delete m_sidebars[i];
    }
}

QWidget *MainWindow::centralWidget() const
{
    return m_centralWidget;
}

ToolView *MainWindow::createToolView(KTextEditor::Plugin *plugin, const QString &identifier, KMultiTabBar::KMultiTabBarPosition pos, const QIcon &icon, const QString &text)
{
    if (m_idToWidget[identifier]) {
        return nullptr;
    }

    // try the restore config to figure out real pos
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        pos = (KMultiTabBar::KMultiTabBarPosition) cg.readEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Position").arg(identifier), int(pos));
    }

    ToolView *v  = m_sidebars[pos]->addWidget(icon, text, nullptr);
    v->id = identifier;
    v->plugin = plugin;

    m_idToWidget.insert(identifier, v);
    m_toolviews.push_back(v);

    // register for menu stuff
    m_guiClient->registerToolView(v);

    return v;
}

ToolView *MainWindow::toolView(const QString &identifier) const
{
    return m_idToWidget[identifier];
}

void MainWindow::toolViewDeleted(ToolView *widget)
{
    if (!widget) {
        return;
    }

    if (widget->mainWindow() != this) {
        return;
    }

    // unregister from menu stuff
    m_guiClient->unregisterToolView(widget);

    widget->sidebar()->removeWidget(widget);

    m_idToWidget.remove(widget->id);
    m_toolviews.removeAt(m_toolviews.indexOf(widget));

}

void MainWindow::setSidebarsVisible(bool visible)
{
    bool old_visible = m_sidebarsVisible;
    m_sidebarsVisible = visible;

    m_sidebars[0]->setVisible(visible);
    m_sidebars[1]->setVisible(visible);
    m_sidebars[2]->setVisible(visible);
    m_sidebars[3]->setVisible(visible);

    m_guiClient->updateSidebarsVisibleAction();

    // show information message box, if the users hides the sidebars
    if (old_visible && (!m_sidebarsVisible)) {
        KMessageBox::information(this,
                                 i18n("<qt>You are about to hide the sidebars. With "
                                      "hidden sidebars it is not possible to directly "
                                      "access the tool views with the mouse anymore, "
                                      "so if you need to access the sidebars again "
                                      "invoke <b>View &gt; Tool Views &gt; Show Sidebars</b> "
                                      "in the menu. It is still possible to show/hide "
                                      "the tool views with the assigned shortcuts.</qt>"),
                                 QString(), QStringLiteral("Kate hide sidebars notification message"));
    }
}

bool MainWindow::sidebarsVisible() const
{
    return m_sidebarsVisible;
}

void MainWindow::setToolViewStyle(KMultiTabBar::KMultiTabBarStyle style)
{
    m_sidebars[0]->setStyle(style);
    m_sidebars[1]->setStyle(style);
    m_sidebars[2]->setStyle(style);
    m_sidebars[3]->setStyle(style);
}

KMultiTabBar::KMultiTabBarStyle MainWindow::toolViewStyle() const
{
    // all sidebars have the same style, so just take Top
    return m_sidebars[KMultiTabBar::Top]->tabStyle();
}

bool MainWindow::moveToolView(ToolView *widget, KMultiTabBar::KMultiTabBarPosition pos)
{
    if (!widget || widget->mainWindow() != this) {
        return false;
    }

    // try the restore config to figure out real pos
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        pos = (KMultiTabBar::KMultiTabBarPosition) cg.readEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Position").arg(widget->id), int(pos));
    }

    m_sidebars[pos]->addWidget(widget->icon, widget->text, widget);

    return true;
}

bool MainWindow::showToolView(ToolView *widget)
{
    if (!widget || widget->mainWindow() != this) {
        return false;
    }

    // skip this if happens during restoring, or we will just see flicker
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        return true;
    }

    return widget->sidebar()->showWidget(widget);
}

bool MainWindow::hideToolView(ToolView *widget)
{
    if (!widget || widget->mainWindow() != this) {
        return false;
    }

    // skip this if happens during restoring, or we will just see flicker
    if (m_restoreConfig && m_restoreConfig->hasGroup(m_restoreGroup)) {
        return true;
    }

    const bool ret =  widget->sidebar()->hideWidget(widget);
    m_centralWidget->setFocus();
    return ret;
}

void MainWindow::startRestore(KConfigBase *config, const QString &group)
{
    // first save this stuff
    m_restoreConfig = config;
    m_restoreGroup = group;

    if (!m_restoreConfig || !m_restoreConfig->hasGroup(m_restoreGroup)) {
        // if no config around, set already now sane default sizes
        // otherwise, set later in ::finishRestore(), since it does not work
        // if set already now (see bug #164438)
        QList<int> hs = (QList<int>() << 200 << 100 << 200);
        QList<int> vs = (QList<int>() << 150 << 100 << 200);

        m_sidebars[0]->setLastSize(hs[0]);
        m_sidebars[1]->setLastSize(hs[2]);
        m_sidebars[2]->setLastSize(vs[0]);
        m_sidebars[3]->setLastSize(vs[2]);

        m_hSplitter->setSizes(hs);
        m_vSplitter->setSizes(vs);
        return;
    }

    // apply size once, to get sizes ready ;)
    KConfigGroup cg(m_restoreConfig, m_restoreGroup);
    KWindowConfig::restoreWindowSize(windowHandle(), cg);

    setToolViewStyle((KMultiTabBar::KMultiTabBarStyle)cg.readEntry("Kate-MDI-Sidebar-Style", (int)toolViewStyle()));
    // after reading m_sidebarsVisible, update the GUI toggle action
    m_sidebarsVisible = cg.readEntry("Kate-MDI-Sidebar-Visible", true);
    m_guiClient->updateSidebarsVisibleAction();
}

void MainWindow::finishRestore()
{
    if (!m_restoreConfig) {
        return;
    }

    if (m_restoreConfig->hasGroup(m_restoreGroup)) {
        // apply all settings, like toolbar pos and more ;)
        KConfigGroup cg(m_restoreConfig, m_restoreGroup);
        applyMainWindowSettings(cg);

        // reshuffle toolviews only if needed
        for (int i = 0; i < m_toolviews.size(); ++i) {
            KMultiTabBar::KMultiTabBarPosition newPos = (KMultiTabBar::KMultiTabBarPosition) cg.readEntry(QString::fromLatin1("Kate-MDI-ToolView-%1-Position").arg(m_toolviews[i]->id), int(m_toolviews[i]->sidebar()->position()));

            if (m_toolviews[i]->sidebar()->position() != newPos) {
                moveToolView(m_toolviews[i], newPos);
            }
        }

        // restore the sidebars
        for (unsigned int i = 0; i < 4; ++i) {
            m_sidebars[i]->restoreSession(cg);
        }

        // restore splitter sizes
        QList<int> hs = (QList<int>() << 200 << 100 << 200);
        QList<int> vs = (QList<int>() << 150 << 100 << 200);

        // get main splitter sizes ;)
        hs = cg.readEntry("Kate-MDI-H-Splitter", hs);
        vs = cg.readEntry("Kate-MDI-V-Splitter", vs);

        m_sidebars[0]->setLastSize(hs[0]);
        m_sidebars[1]->setLastSize(hs[2]);
        m_sidebars[2]->setLastSize(vs[0]);
        m_sidebars[3]->setLastSize(vs[2]);

        m_hSplitter->setSizes(hs);
        m_vSplitter->setSizes(vs);
    }

    // clear this stuff, we are done ;)
    m_restoreConfig = nullptr;
    m_restoreGroup.clear();
}

void MainWindow::saveSession(KConfigGroup &config)
{
    saveMainWindowSettings(config);

    // save main splitter sizes ;)
    QList<int> hs = m_hSplitter->sizes();
    QList<int> vs = m_vSplitter->sizes();

    if (hs[0] <= 2 && !m_sidebars[0]->splitterVisible()) {
        hs[0] = m_sidebars[0]->lastSize();
    }
    if (hs[2] <= 2 && !m_sidebars[1]->splitterVisible()) {
        hs[2] = m_sidebars[1]->lastSize();
    }
    if (vs[0] <= 2 && !m_sidebars[2]->splitterVisible()) {
        vs[0] = m_sidebars[2]->lastSize();
    }
    if (vs[2] <= 2 && !m_sidebars[3]->splitterVisible()) {
        vs[2] = m_sidebars[3]->lastSize();
    }

    config.writeEntry("Kate-MDI-H-Splitter", hs);
    config.writeEntry("Kate-MDI-V-Splitter", vs);

    // save sidebar style
    config.writeEntry("Kate-MDI-Sidebar-Style", (int)toolViewStyle());
    config.writeEntry("Kate-MDI-Sidebar-Visible", m_sidebarsVisible);

    // save the sidebars
    for (unsigned int i = 0; i < 4; ++i) {
        m_sidebars[i]->saveSession(config);
    }
}

//END MAIN WINDOW

} // namespace KateMDI

