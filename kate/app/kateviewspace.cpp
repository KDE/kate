/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2005 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "kateviewspace.h"

#include <ktexteditor/sessionconfiginterface.h>

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katedocmanager.h"
#include "kateapp.h"
#include "katesessionmanager.h"
#include "katedebug.h"
#include "katetabbar.h"
#include "kactioncollection.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QToolButton>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QHelpEvent>
#include <QToolTip>

//BEGIN KateViewSpace
KateViewSpace::KateViewSpace(KateViewManager *viewManager,
                             QWidget *parent, const char *name)
    : QWidget(parent)
    , m_viewManager(viewManager)
{
    setObjectName(QString::fromLatin1(name));
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);

    //BEGIN tab bar
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setSpacing(0);
    hLayout->setMargin(0);

    // add tab bar
    m_tabBar = new KateTabBar(this);
    connect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);
    connect(m_tabBar, &KateTabBar::moreTabsRequested, this, &KateViewSpace::addTabs);
    connect(m_tabBar, &KateTabBar::lessTabsRequested, this, &KateViewSpace::removeTabs);
    hLayout->addWidget(m_tabBar);

    // add vertical split view space
    QToolButton *split = new QToolButton(this);
    split->setAutoRaise(true);
    split->setPopupMode(QToolButton::InstantPopup);
    split->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_split_vert")));
    split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_split_horiz")));
    split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_close_current_space")));
    split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_close_others")));
    split->setWhatsThis(i18n("Control view space splitting"));
    split->installEventFilter(this); // on click, active this view space
    hLayout->addWidget(split);

    // add quick open
    QToolButton *quickOpen = new QToolButton(this);
    quickOpen->setAutoRaise(true);
    quickOpen->setDefaultAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_quick_open")));
    quickOpen->installEventFilter(this); // on click, active this view space
    hLayout->addWidget(quickOpen);

    // FIXME: better additional size
    m_tabBar->setMinimumHeight(int (QFontMetrics(font()).height() * 1.2));

    layout->addLayout(hLayout);
    //END tab bar

    stack = new QStackedWidget(this);
    stack->setFocus();
    stack->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    layout->addWidget(stack);

    mIsActiveSpace = false;

    m_group.clear();

    // connect signal to hide/show statusbar
    connect(m_viewManager->mainWindow(), SIGNAL(statusBarToggled()), this, SLOT(statusBarToggled()));

    // init the statusbar...
    statusBarToggled();
}

bool KateViewSpace::eventFilter(QObject *obj, QEvent *event)
{
    QToolButton *button = qobject_cast<QToolButton *>(obj);

    // maybe a tool tip of a QToolButton: show shortcuts
    if (button && event->type() == QEvent::ToolTip) {
        QHelpEvent *e = static_cast<QHelpEvent *>(event);
        if (button->defaultAction()) {
            QToolTip::showText(e->globalPos(),
                               button->toolTip() + QStringLiteral(" (%1)").arg(button->defaultAction()->shortcut().toString()), button);
            return true;
        }
    }

    // on mouse press on view space bar tool buttons: activate this space
    if (button && ! isActiveSpace() && event->type() == QEvent::MouseButtonPress) {
        m_viewManager->setActiveSpace(this);
        m_viewManager->activateView(currentView()->document());
    }
    return false;
}

void KateViewSpace::statusBarToggled()
{
    Q_FOREACH(KTextEditor::Document * doc, m_lruDocList) {
        if (m_docToView.contains(doc)) {
            m_docToView[doc]->setStatusBarEnabled(m_viewManager->mainWindow()->showStatusBar());
        }
    }
}

KTextEditor::View *KateViewSpace::createView(KTextEditor::Document *doc)
{
    // should only be called if a view does not yet exist
    Q_ASSERT(! m_docToView.contains(doc));

    /**
     * Create a fresh view
     */
    KTextEditor::View *v = doc->createView(stack, m_viewManager->mainWindow()->wrapper());

    // set status bar to right state
    v->setStatusBarEnabled(m_viewManager->mainWindow()->showStatusBar());

    // restore the config of this view if possible
    if (!m_group.isEmpty()) {
        QString fn = v->document()->url().toString();
        if (! fn.isEmpty()) {
            QString vgroup = QString::fromLatin1("%1 %2").arg(m_group).arg(fn);

            KateSession::Ptr as = KateApp::self()->sessionManager()->activeSession();
            if (as->config() && as->config()->hasGroup(vgroup)) {
                KConfigGroup cg(as->config(), vgroup);

                if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(v)) {
                    iface->readSessionConfig(cg);
                }
            }
        }
    }

    // register document, it is shown below through showView() then
    if (! m_lruDocList.contains(doc)) {
        registerDocument(doc);
        Q_ASSERT(m_lruDocList.contains(doc));
    }

    // insert View into stack
    stack->addWidget(v);
    m_docToView[doc] = v;
    showView(v);

    return v;
}

void KateViewSpace::removeView(KTextEditor::View *v)
{
    // remove view mappings
    Q_ASSERT(m_docToView.contains(v->document()));
    m_docToView.remove(v->document());
    Q_ASSERT(m_docToTabId.contains(v->document()));
    documentDestroyed(v->document());

    // remove from view space
    bool active = (v == currentView());

    stack->removeWidget(v);

    if (! active) {
        return;
    }

    // the last recently used view/document is always at the end of the list
    if (! m_lruDocList.isEmpty()) {
        KTextEditor::Document * doc = m_lruDocList.last();
        if (m_docToView.contains(doc)) {
            showView(doc);
        } else {
            m_viewManager->createView(doc, this);
        }
    }
}

bool KateViewSpace::showView(KTextEditor::Document *document)
{
    const int index = m_lruDocList.lastIndexOf(document);

    if (index < 0) {
        return false;
    }

    if (! m_docToView.contains(document)) {
        return false;
    }

    KTextEditor::View *kv = m_docToView[document];

    // move view to end of list
    m_lruDocList.removeAt(index);
    m_lruDocList.append(document);
    stack->setCurrentWidget(kv);
    kv->show();

    // raise tab in view space tab bar
    Q_ASSERT(m_docToTabId.contains(document));
    m_tabBar->setCurrentTab(m_docToTabId[document]);

    return true;
}

void KateViewSpace::changeView(int id)
{
    KTextEditor::Document *doc = m_docToTabId.key(id);
    Q_ASSERT(doc);

    // make sure we open the view in this view space
    if (! isActiveSpace()) {
        m_viewManager->setActiveSpace(this);
    }

    // tell the view manager to show the view
    m_viewManager->activateView(doc);
}

KTextEditor::View *KateViewSpace::currentView()
{
    // might be 0 if the stack contains no view
    return (KTextEditor::View *)stack->currentWidget();
}

bool KateViewSpace::isActiveSpace()
{
    return mIsActiveSpace;
}

void KateViewSpace::setActive(bool active, bool)
{
    mIsActiveSpace = active;
}

void KateViewSpace::insertTab(int index, KTextEditor::Document * doc)
{
    // doc should be in the lru list
    Q_ASSERT(m_lruDocList.contains(doc));

    // doc should not have a id
    Q_ASSERT(! m_docToTabId.contains(doc));

    const int id = m_tabBar->insertTab(index, doc->documentName());
    m_tabBar->setTabToolTip(id, doc->url().toDisplayString());
    m_docToTabId[doc] = id;

    connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)),
            this, SLOT(updateDocumentName(KTextEditor::Document*)));
}

int KateViewSpace::removeTab(KTextEditor::Document * doc)
{
    //
    // WARNING: removeTab() is also called from documentDestroyed().
    // Therefore, is may be that doc is half destroyed already.
    // Therefore, do not access any KTextEditor::Document functions here!
    // Only access QObject functions!
    //
    Q_ASSERT(m_docToTabId.contains(doc));

    const int id = m_docToTabId[doc];
    const int removeIndex = m_tabBar->removeTab(id);
    m_docToTabId.remove(doc);

    disconnect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)),
               this, SLOT(updateDocumentName(KTextEditor::Document*)));

    return removeIndex;
}

void KateViewSpace::removeTabs(int count)
{
    qDebug() << "________the tab bar wants LESS tabs:" << count;

    while (count > 0) {
        const int tabCount = m_tabBar->count();
        KTextEditor::Document * removeDoc = m_lruDocList[m_lruDocList.size() - tabCount];
        removeTab(removeDoc);
        Q_ASSERT(! m_docToTabId.contains(removeDoc));
        --count;
    }
}

void KateViewSpace::addTabs(int count)
{
    qDebug() << "________the tab bar wants MORE tabs:" << count;

    while (count > 0) {
        const int tabCount = m_tabBar->count();
        if (m_lruDocList.size() <= tabCount) {
            break;
        }
        insertTab(tabCount, m_lruDocList[m_lruDocList.size() - tabCount - 1]);
        --count;
    }
}

void KateViewSpace::registerDocument(KTextEditor::Document *doc)
{
    // at this point, the doc should be completely unknown
    Q_ASSERT(! m_lruDocList.contains(doc));
    Q_ASSERT(! m_docToView.contains(doc));
    Q_ASSERT(! m_docToTabId.contains(doc));

    m_lruDocList.append(doc);
    connect(doc, SIGNAL(destroyed(QObject*)), this, SLOT(documentDestroyed(QObject*)));

    // if space is available, add button
    if (m_tabBar->count() < m_tabBar->maxTabCount()) {
        insertTab(m_tabBar->count(), doc);
    } else {
        // remove "oldest" button and replace with new one
        Q_ASSERT(m_lruDocList.size() > m_tabBar->count());

        KTextEditor::Document * docToHide = m_lruDocList[m_tabBar->count() - 1];
        Q_ASSERT(m_docToTabId.contains(docToHide));
        const int insertIndex = removeTab(docToHide);

        // add new one at removed position
        insertTab(insertIndex, doc);
    }
}

void KateViewSpace::documentDestroyed(QObject *doc)
{
    // WARNING: this pointer is half destroyed
    KTextEditor::Document *invalidDoc = static_cast<KTextEditor::Document *>(doc);

    Q_ASSERT(m_lruDocList.contains(invalidDoc));
    m_lruDocList.remove(m_lruDocList.indexOf(invalidDoc));

    // case: there was no view created yet, but still a button was added
    if (m_docToTabId.contains(invalidDoc)) {
        const int insertIndex = removeTab(invalidDoc);
        // maybe show another tab button in its stead
        if (m_lruDocList.size() >= m_tabBar->maxTabCount()
            && m_tabBar->count() < m_tabBar->maxTabCount()
        ) {
            KTextEditor::Document * docToShow = m_lruDocList[m_tabBar->count() - 1];
            Q_ASSERT(! m_docToTabId.contains(docToShow));

            // add tab that now fits into the bar
            insertTab(insertIndex, docToShow);
        }
    }

    // disconnect entirely
    disconnect(doc, 0, this, 0);

    // at this point, the doc should be completely unknown
    Q_ASSERT(! m_lruDocList.contains(invalidDoc));
    Q_ASSERT(! m_docToView.contains(invalidDoc));
    Q_ASSERT(! m_docToTabId.contains(invalidDoc));
}

void KateViewSpace::updateDocumentName(KTextEditor::Document *doc)
{
    const int buttonId = m_docToTabId[doc];
    Q_ASSERT(buttonId >= 0);
    m_tabBar->setTabText(buttonId, doc->documentName());
    m_tabBar->setTabToolTip(buttonId, doc->url().toDisplayString());
}

void KateViewSpace::saveConfig(KConfigBase *config, int myIndex , const QString &viewConfGrp)
{
//   qCDebug(LOG_KATE)<<"KateViewSpace::saveConfig("<<myIndex<<", "<<viewConfGrp<<") - currentView: "<<currentView()<<")";
    QString groupname = QString(viewConfGrp + QStringLiteral("-ViewSpace %1")).arg(myIndex);

    // aggregate all views in view space (LRU ordered)
    QVector<KTextEditor::View*> views;
    Q_FOREACH(KTextEditor::Document* doc, m_lruDocList) {
        if (m_docToView.contains(doc)) {
            views.append(m_docToView[doc]);
        }
    }

    KConfigGroup group(config, groupname);
    group.writeEntry("Count", views.count());

    if (currentView()) {
        group.writeEntry("Active View", currentView()->document()->url().toString());
    }

    // Save file list, including cursor position in this instance.
    int idx = 0;
    for (QVector<KTextEditor::View *>::iterator it = views.begin(); it != views.end(); ++it) {
        if (!(*it)->document()->url().isEmpty()) {
            group.writeEntry(QString::fromLatin1("View %1").arg(idx), (*it)->document()->url().toString());

            // view config, group: "ViewSpace <n> url"
            QString vgroup = QString::fromLatin1("%1 %2").arg(groupname).arg((*it)->document()->url().toString());
            KConfigGroup viewGroup(config, vgroup);

            if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(*it)) {
                iface->writeSessionConfig(viewGroup);
            }
        }

        ++idx;
    }
}

void KateViewSpace::restoreConfig(KateViewManager *viewMan, const KConfigBase *config, const QString &groupname)
{
    KConfigGroup group(config, groupname);
    QString fn = group.readEntry("Active View");

    if (!fn.isEmpty()) {
        KTextEditor::Document *doc = KateApp::self()->documentManager()->findDocument(QUrl(fn));

        if (doc) {
            // view config, group: "ViewSpace <n> url"
            QString vgroup = QString::fromLatin1("%1 %2").arg(groupname).arg(fn);
            KConfigGroup configGroup(config, vgroup);

            viewMan->createView(doc, this);

            KTextEditor::View *v = viewMan->activeView();

            if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(v)) {
                iface->readSessionConfig(configGroup);
            }
        }
    }

    // avoid empty view space
    if (m_docToView.isEmpty()) {
        viewMan->createView(KateApp::self()->documentManager()->document(0), this);
    }

    m_group = groupname; // used for restroing view configs later
}
//END KateViewSpace
