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

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katedocmanager.h"
#include "kateapp.h"
#include "katesessionmanager.h"
#include "katedebug.h"
#include "katetabbar.h"
#include "kactioncollection.h"
#include "kateupdatedisabler.h"

#include <KAcceleratorManager>
#include <KConfigGroup>
#include <KLocalizedString>
// remove #ifdef, once Kate depends on KF 5.24
#include <kio_version.h>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 24, 0)
#include <KIO/OpenFileManagerWindowJob>
#endif

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QHelpEvent>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>
#include <QToolTip>
#include <QWhatsThis>

//BEGIN KateViewSpace
KateViewSpace::KateViewSpace(KateViewManager *viewManager,
                             QWidget *parent, const char *name)
    : QWidget(parent)
    , m_viewManager(viewManager)
    , m_isActiveSpace(false)
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
    connect(m_tabBar, &KateTabBar::closeTabRequested, this, &KateViewSpace::closeTabRequest, Qt::QueuedConnection);
    connect(m_tabBar, &KateTabBar::contextMenuRequest, this, &KateViewSpace::showContextMenu, Qt::QueuedConnection);
    connect(m_tabBar, &KateTabBar::newTabRequested, this, &KateViewSpace::createNewDocument);
    connect(m_tabBar, SIGNAL(activateViewSpaceRequested()), this, SLOT(makeActive()));
    hLayout->addWidget(m_tabBar);

    // add quick open
    m_quickOpen = new QToolButton(this);
    m_quickOpen->setAutoRaise(true);
    KAcceleratorManager::setNoAccel(m_quickOpen);
    m_quickOpen->installEventFilter(this); // on click, active this view space
    hLayout->addWidget(m_quickOpen);

    // forward tab bar quick open action to globa quick open action
    QAction * bridge = new QAction(QIcon::fromTheme(QStringLiteral("tab-duplicate")),
                                   i18nc("indicator for more documents", "+%1", 100), this);
    m_quickOpen->setDefaultAction(bridge);
    QAction * quickOpen = m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_quick_open"));
    Q_ASSERT(quickOpen);
    bridge->setToolTip(quickOpen->toolTip());
    bridge->setWhatsThis(i18n("Click here to switch to the Quick Open view."));
    connect(bridge, SIGNAL(triggered()), quickOpen, SLOT(trigger()));

    // add vertical split view space
    m_split = new QToolButton(this);
    m_split->setAutoRaise(true);
    m_split->setPopupMode(QToolButton::InstantPopup);
    m_split->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_split_vert")));
    m_split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_split_horiz")));
    m_split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_close_current_space")));
    m_split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_close_others")));
    m_split->addAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_hide_others")));
    m_split->setWhatsThis(i18n("Control view space splitting"));
    m_split->installEventFilter(this); // on click, active this view space
    hLayout->addWidget(m_split);

    layout->addLayout(hLayout);
    //END tab bar

    stack = new QStackedWidget(this);
    stack->setFocus();
    stack->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding));
    layout->addWidget(stack);

    m_group.clear();

    // connect signal to hide/show statusbar
    connect(m_viewManager->mainWindow(), SIGNAL(statusBarToggled()), this, SLOT(statusBarToggled()));
    connect(m_viewManager->mainWindow(), SIGNAL(tabBarToggled()), this, SLOT(tabBarToggled()));

    // init the bars...
    statusBarToggled();
    tabBarToggled();

    // make sure we show correct number of hidden documents
    updateQuickOpen();
    connect(KateApp::self()->documentManager(), SIGNAL(documentCreated(KTextEditor::Document*)), this, SLOT(updateQuickOpen()));
    connect(KateApp::self()->documentManager(), SIGNAL(documentsDeleted(const QList<KTextEditor::Document*>&)), this, SLOT(updateQuickOpen()));
}

bool KateViewSpace::eventFilter(QObject *obj, QEvent *event)
{
    QToolButton *button = qobject_cast<QToolButton *>(obj);

    // quick open button: show tool tip with shortcut
    if (button == m_quickOpen && event->type() == QEvent::ToolTip) {
        QHelpEvent *e = static_cast<QHelpEvent *>(event);
        QAction *quickOpen = m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_quick_open"));
        Q_ASSERT(quickOpen);
        QToolTip::showText(e->globalPos(),
                           button->toolTip() + QStringLiteral(" (%1)").arg(quickOpen->shortcut().toString()), button);
        return true;
    }

    // quick open button: What's This
    if (button == m_quickOpen && event->type() == QEvent::WhatsThis) {
        QHelpEvent *e = static_cast<QHelpEvent *>(event);
        const int hiddenDocs = hiddenDocuments();
        QString helpText = (hiddenDocs == 0)
                         ? i18n("Click here to switch to the Quick Open view.")
                         : i18np("Currently, there is one more document open. To see all open documents, switch to the Quick Open view by clicking here.",
                                 "Currently, there are %1 more documents open. To see all open documents, switch to the Quick Open view by clicking here.",
                                 hiddenDocs);
        QWhatsThis::showText(e->globalPos(), helpText, m_quickOpen);
        return true;
    }

    // on mouse press on view space bar tool buttons: activate this space
    if (button && ! isActiveSpace() && event->type() == QEvent::MouseButtonPress) {
        m_viewManager->setActiveSpace(this);
        if (currentView()) {
            m_viewManager->activateView(currentView()->document());
        }
    }
    return false;
}

void KateViewSpace::statusBarToggled()
{
    KateUpdateDisabler updatesDisabled (m_viewManager->mainWindow());
    Q_FOREACH(KTextEditor::Document * doc, m_lruDocList) {
        if (m_docToView.contains(doc)) {
            m_docToView[doc]->setStatusBarEnabled(m_viewManager->mainWindow()->showStatusBar());
        }
    }
}

QVector<KTextEditor::Document*> KateViewSpace::lruDocumentList() const
{
    return m_lruDocList;
}

void KateViewSpace::mergeLruList(const QVector<KTextEditor::Document*> & lruList)
{
    // merge lruList documents that are not in m_lruDocList
    QVectorIterator<KTextEditor::Document*> it(lruList);
    it.toBack();
    while (it.hasPrevious()) {
        KTextEditor::Document *doc = it.previous();
        if (! m_lruDocList.contains(doc)) {
            registerDocument(doc, false);
        }
    }
}

void KateViewSpace::tabBarToggled()
{
    KateUpdateDisabler updatesDisabled (m_viewManager->mainWindow());
    m_tabBar->setVisible(m_viewManager->mainWindow()->showTabBar());
    m_split->setVisible(m_viewManager->mainWindow()->showTabBar());
    m_quickOpen->setVisible(m_viewManager->mainWindow()->showTabBar());
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
                v->readSessionConfig(cg);
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

    // ...and now: remove from view space
    stack->removeWidget(v);
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

    // in case a tab does not exist, add one
    if (! m_docToTabId.contains(document)) {
        // if space is available, add button
        if (m_tabBar->count() < m_tabBar->maxTabCount()) {
            // just insert
            insertTab(0, document);
        } else {
            // remove "oldest" button and replace with new one
            Q_ASSERT(m_lruDocList.size() > m_tabBar->count());

            // we need to subtract by 1 more, as we just added ourself to the end of the lru list!
            KTextEditor::Document * docToHide = m_lruDocList[m_lruDocList.size() - m_tabBar->maxTabCount() - 1];
            Q_ASSERT(m_docToTabId.contains(docToHide));
            removeTab(docToHide, false);

            // add new one always at the beginning
            insertTab(0, document);
        }
    }

    // follow current view
    Q_ASSERT(m_docToTabId.contains(document));
    m_tabBar->setCurrentTab(m_docToTabId.value(document, -1));

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
    return m_isActiveSpace;
}

void KateViewSpace::setActive(bool active)
{
    m_isActiveSpace = active;
    m_tabBar->setActive(active);
}

void KateViewSpace::makeActive(bool focusCurrentView)
{
    if (! isActiveSpace()) {
        m_viewManager->setActiveSpace(this);
        if (focusCurrentView && currentView()) {
            m_viewManager->activateView(currentView()->document());
        }
    }
    Q_ASSERT(isActiveSpace());
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
    updateDocumentState(doc);

    connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)),
            this, SLOT(updateDocumentName(KTextEditor::Document*)));
    connect(doc, &KTextEditor::Document::documentUrlChanged,
            this, &KateViewSpace::updateDocumentUrl);
    connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)),
            this, SLOT(updateDocumentState(KTextEditor::Document*)));
}

int KateViewSpace::removeTab(KTextEditor::Document * doc, bool documentDestroyed)
{
    //
    // WARNING: removeTab() is also called from documentDestroyed().
    // Therefore, is may be that doc is half destroyed already.
    // Therefore, do not access any KTextEditor::Document functions here!
    // Only access QObject functions!
    //
    Q_ASSERT(m_docToTabId.contains(doc));

    const int id = m_docToTabId.value(doc, -1);
    const int removeIndex = m_tabBar->removeTab(id);
    m_docToTabId.remove(doc);

    if (!documentDestroyed) {
        disconnect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)),
               this, SLOT(updateDocumentName(KTextEditor::Document*)));
        disconnect(doc, &KTextEditor::Document::documentUrlChanged,
                this, &KateViewSpace::updateDocumentUrl);
        disconnect(doc, SIGNAL(modifiedChanged(KTextEditor::Document*)),
              this, SLOT(updateDocumentState(KTextEditor::Document*)));
    }

    return removeIndex;
}

void KateViewSpace::removeTabs(int count)
{
    const int start = count;

    /// remove @p count tabs from the tab bar, as they do not all fit
    while (count > 0) {
        const int tabCount = m_tabBar->count();
        KTextEditor::Document * removeDoc = m_lruDocList[m_lruDocList.size() - tabCount];
        removeTab(removeDoc, false);
        Q_ASSERT(! m_docToTabId.contains(removeDoc));
        --count;
    }

    // make sure quick open shows the correct number of hidden documents
    if (start != count) {
        updateQuickOpen();
    }
}

void KateViewSpace::addTabs(int count)
{
    const int start = count;

    /// @p count tabs still fit into the tab bar: add as man as possible
    while (count > 0) {
        const int tabCount = m_tabBar->count();
        if (m_lruDocList.size() <= tabCount) {
            break;
        }
        insertTab(tabCount, m_lruDocList[m_lruDocList.size() - tabCount - 1]);
        --count;
    }

    // make sure quick open shows the correct number of hidden documents
    if (start != count) {
        updateQuickOpen();
    }
}

void KateViewSpace::registerDocument(KTextEditor::Document *doc, bool append)
{
    // at this point, the doc should be completely unknown
    Q_ASSERT(! m_lruDocList.contains(doc));
    Q_ASSERT(! m_docToView.contains(doc));
    Q_ASSERT(! m_docToTabId.contains(doc));

    if (append) {
        m_lruDocList.append(doc);
    } else {
        // prepending == merge doc of closed viewspace
        m_lruDocList.prepend(doc);
    }
    connect(doc, SIGNAL(destroyed(QObject*)), this, SLOT(documentDestroyed(QObject*)));

    // if space is available, add button
    if (m_tabBar->count() < m_tabBar->maxTabCount()) {
        insertTab(0, doc);
        updateQuickOpen();
    } else if (append) {
        // remove "oldest" button and replace with new one
        Q_ASSERT(m_lruDocList.size() > m_tabBar->count());

        KTextEditor::Document * docToHide = m_lruDocList[m_lruDocList.size() - m_tabBar->maxTabCount() - 1];
        Q_ASSERT(m_docToTabId.contains(docToHide));
        removeTab(docToHide, false);

        // add new one at removed position
        insertTab(0, doc);
    }
}

void KateViewSpace::documentDestroyed(QObject *doc)
{
    // WARNING: this pointer is half destroyed
    KTextEditor::Document *invalidDoc = static_cast<KTextEditor::Document *>(doc);

    Q_ASSERT(m_lruDocList.contains(invalidDoc));
    m_lruDocList.remove(m_lruDocList.indexOf(invalidDoc));

    // disconnect entirely
    disconnect(doc, nullptr, this, nullptr);

    // case: there was no view created yet, but still a button was added
    if (m_docToTabId.contains(invalidDoc)) {
        removeTab(invalidDoc, true);
        // maybe show another tab button in its stead
        if (m_lruDocList.size() >= m_tabBar->maxTabCount()
            && m_tabBar->count() < m_tabBar->maxTabCount()
        ) {
            KTextEditor::Document * docToShow = m_lruDocList[m_lruDocList.size() - m_tabBar->count() - 1];
            Q_ASSERT(! m_docToTabId.contains(docToShow));

            // add tab that now fits into the bar
            insertTab(m_tabBar->count(), docToShow);
        }
    }

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

void KateViewSpace::updateDocumentUrl(KTextEditor::Document *doc)
{
    const int buttonId = m_docToTabId[doc];
    Q_ASSERT(buttonId >= 0);
    m_tabBar->setTabUrl(buttonId, doc->url());
}

void KateViewSpace::updateDocumentState(KTextEditor::Document *doc)
{
    QIcon icon;
    if (doc->isModified()) {
        icon = QIcon::fromTheme(QLatin1String("document-save"));
    }

    Q_ASSERT(m_docToTabId.contains(doc));
    const int buttonId = m_docToTabId[doc];
    m_tabBar->setTabIcon(buttonId, icon);
}

void KateViewSpace::closeTabRequest(int id)
{
    KTextEditor::Document *doc = m_docToTabId.key(id);
    Q_ASSERT(doc);
    KateApp::self()->documentManager()->closeDocument(doc);
}

void KateViewSpace::createNewDocument()
{
    // make sure we open the view in this view space
    if (! isActiveSpace()) {
        m_viewManager->setActiveSpace(this);
    }

    // create document
    KTextEditor::Document *doc = KateApp::self()->documentManager()->createDoc();

    // tell the view manager to show the document
    m_viewManager->activateView(doc);
}

void KateViewSpace::updateQuickOpen()
{
    const int hiddenDocs = hiddenDocuments();

    if (hiddenDocs == 0) {
        m_quickOpen->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_quickOpen->defaultAction()->setText(QString());
    } else {
        m_quickOpen->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_quickOpen->defaultAction()->setText(i18nc("indicator for more documents", "+%1",
                                              hiddenDocs));
    }
}

void KateViewSpace::focusPrevTab()
{
    const int id = m_tabBar->prevTab();
    if (id >= 0) {
        changeView(id);
    }
}

void KateViewSpace::focusNextTab()
{
    const int id = m_tabBar->nextTab();
    if (id >= 0) {
        changeView(id);
    }
}

int KateViewSpace::hiddenDocuments() const
{
    const int hiddenDocs = KateApp::self()->documents().count() - m_tabBar->count();
    Q_ASSERT(hiddenDocs >= 0);
    return hiddenDocs;
}

void KateViewSpace::showContextMenu(int id, const QPoint & globalPos)
{
    // right now, show no context menu on empty tab bar space
    if (id < 0) {
        return;
    }

    KTextEditor::Document *doc = m_docToTabId.key(id);
    Q_ASSERT(doc);

    QMenu menu(this);
    QAction *aCloseTab = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close")), i18n("&Close Document"));
    QAction *aCloseOthers = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close-other")), i18n("Close Other &Documents"));
    menu.addSeparator();
    QAction *aCopyPath = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy &Path"));
    QAction *aOpenFolder = menu.addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("&Open Containing Folder"));

    if (KateApp::self()->documentManager()->documentList().count() < 2) {
        aCloseOthers->setEnabled(false);
    }
    if (doc->url().isEmpty()) {
        aCopyPath->setEnabled(false);
        aOpenFolder->setEnabled(false);
    }

    QAction *choice = menu.exec(globalPos);

    if (choice == aCloseTab) {
        closeTabRequest(id);
    } else if (choice == aCloseOthers) {
        KateApp::self()->documentManager()->closeOtherDocuments(doc);
    } else if (choice == aCopyPath) {
        QApplication::clipboard()->setText(doc->url().toDisplayString(QUrl::PreferLocalFile));
    } else if (choice == aOpenFolder) {
#if KIO_VERSION >= QT_VERSION_CHECK(5, 24, 0)
        KIO::highlightInFileManager({doc->url()});
#else
        QDesktopServices::openUrl(doc->url().adjusted(QUrl::RemoveFilename));
#endif
    }
}

void KateViewSpace::saveConfig(KConfigBase *config, int myIndex , const QString &viewConfGrp)
{
//   qCDebug(LOG_KATE)<<"KateViewSpace::saveConfig("<<myIndex<<", "<<viewConfGrp<<") - currentView: "<<currentView()<<")";
    QString groupname = QString(viewConfGrp + QStringLiteral("-ViewSpace %1")).arg(myIndex);

    // aggregate all views in view space (LRU ordered)
    QVector<KTextEditor::View*> views;
    QStringList lruList;
    Q_FOREACH(KTextEditor::Document* doc, m_lruDocList) {
        lruList << doc->url().toString();
        if (m_docToView.contains(doc)) {
            views.append(m_docToView[doc]);
        }
    }

    KConfigGroup group(config, groupname);
    group.writeEntry("Documents", lruList);
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
            (*it)->writeSessionConfig(viewGroup);
        }

        ++idx;
    }
}

void KateViewSpace::restoreConfig(KateViewManager *viewMan, const KConfigBase *config, const QString &groupname)
{
    KConfigGroup group(config, groupname);

    // restore Document lru list so that all tabs from the last session reappear
    const QStringList lruList = group.readEntry("Documents", QStringList());
    for (int i = 0; i < lruList.size(); ++i) {
        auto doc = KateApp::self()->documentManager()->findDocument(QUrl(lruList[i]));
        if (doc) {
            const int index = m_lruDocList.indexOf(doc);
            if (index < 0) {
                registerDocument(doc);
                Q_ASSERT(m_lruDocList.contains(doc));
            } else {
                m_lruDocList.removeAt(index);
                m_lruDocList.append(doc);
            }
        }
    }

    // restore active view properties
    const QString fn = group.readEntry("Active View");
    if (!fn.isEmpty()) {
        KTextEditor::Document *doc = KateApp::self()->documentManager()->findDocument(QUrl(fn));

        if (doc) {
            // view config, group: "ViewSpace <n> url"
            QString vgroup = QString::fromLatin1("%1 %2").arg(groupname).arg(fn);
            KConfigGroup configGroup(config, vgroup);

            auto view = viewMan->createView(doc, this);
            if (view) {
                view->readSessionConfig(configGroup);
            }
        }
    }

    // avoid empty view space
    if (m_docToView.isEmpty()) {
        viewMan->createView (KateApp::self()->documentManager()->documentList().first(), this);
    }

    m_group = groupname; // used for restroing view configs later
}
//END KateViewSpace
