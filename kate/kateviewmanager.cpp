/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

//BEGIN Includes
#include "kateviewmanager.h"

#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewspace.h"
#include "kateupdatedisabler.h"

#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <KTextEditor/Attribute>

#include <KActionCollection>
#include <KToolBar>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KXMLGUIFactory>

#include <config.h>

#ifdef KActivities_FOUND
#include <KActivities/ResourceInstance>
#endif

#include <QFileDialog>
#include <QStyle>

//END Includes

static const qint64 FileSizeAboveToAskUserIfProceedWithOpen = 10 * 1024 * 1024; // 10MB should suffice

KateViewManager::KateViewManager(QWidget *parentW, KateMainWindow *parent)
    : QSplitter(parentW)
    , m_mainWindow(parent)
    , m_blockViewCreationAndActivation(false)
    , m_activeViewRunning(false)
    , m_minAge(0)
    , m_guiMergedView(nullptr)
{
    // while init
    m_init = true;

    // important, set them up, as we use them in other methodes
    setupActions();

    KateViewSpace *vs = new KateViewSpace(this, nullptr);
    addWidget(vs);

    vs->setActive(true);
    m_viewSpaceList.append(vs);

    connect(this, SIGNAL(viewChanged(KTextEditor::View*)), this, SLOT(slotViewChanged()));

    connect(KateApp::self()->documentManager(), SIGNAL(documentCreatedViewManager(KTextEditor::Document*)), this, SLOT(documentCreated(KTextEditor::Document*)));

    /**
     * before document is really deleted: cleanup all views!
     */
    connect(KateApp::self()->documentManager(), SIGNAL(documentWillBeDeleted(KTextEditor::Document*))
        , this, SLOT(documentWillBeDeleted(KTextEditor::Document*)));

    /**
     * handle document deletion transactions
     * disable view creation in between
     * afterwards ensure we have views ;)
     */
    connect(KateApp::self()->documentManager(), SIGNAL(aboutToDeleteDocuments(const QList<KTextEditor::Document *> &))
        , this, SLOT(aboutToDeleteDocuments(const QList<KTextEditor::Document *> &)));
    connect(KateApp::self()->documentManager(), SIGNAL(documentsDeleted(const QList<KTextEditor::Document *> &))
        , this, SLOT(documentsDeleted(const QList<KTextEditor::Document *> &)));

    // register all already existing documents
    m_blockViewCreationAndActivation = true;

    const QList<KTextEditor::Document *> &docs = KateApp::self()->documentManager()->documentList();
    foreach(KTextEditor::Document * doc, docs) {
        documentCreated(doc);
    }

    m_blockViewCreationAndActivation = false;

    // init done
    m_init = false;
}

KateViewManager::~KateViewManager()
{
    /**
     * remove the single client that is registered at the factory, if any
     */
    if (m_guiMergedView) {
        mainWindow()->guiFactory()->removeClient(m_guiMergedView);
        m_guiMergedView = nullptr;
    }
}

void KateViewManager::setupActions()
{
    /**
     * view splitting
     */
    m_splitViewVert = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_split_vert"));
    m_splitViewVert->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_splitViewVert->setText(i18n("Split Ve&rtical"));
    m_mainWindow->actionCollection()->setDefaultShortcut(m_splitViewVert, Qt::CTRL + Qt::SHIFT + Qt::Key_L);
    connect(m_splitViewVert, SIGNAL(triggered()), this, SLOT(slotSplitViewSpaceVert()));

    m_splitViewVert->setWhatsThis(i18n("Split the currently active view vertically into two views."));

    m_splitViewHoriz = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_split_horiz"));
    m_splitViewHoriz->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    m_splitViewHoriz->setText(i18n("Split &Horizontal"));
    m_mainWindow->actionCollection()->setDefaultShortcut(m_splitViewHoriz, Qt::CTRL + Qt::SHIFT + Qt::Key_T);
    connect(m_splitViewHoriz, SIGNAL(triggered()), this, SLOT(slotSplitViewSpaceHoriz()));

    m_splitViewHoriz->setWhatsThis(i18n("Split the currently active view horizontally into two views."));

    m_closeView = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_close_current_space"));
    m_closeView->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    m_closeView->setText(i18n("Cl&ose Current View"));
    m_mainWindow->actionCollection()->setDefaultShortcut(m_closeView, Qt::CTRL + Qt::SHIFT + Qt::Key_R);
    connect(m_closeView, SIGNAL(triggered()), this, SLOT(slotCloseCurrentViewSpace()), Qt::QueuedConnection);

    m_closeView->setWhatsThis(i18n("Close the currently active split view"));

    m_closeOtherViews = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_close_others"));
    m_closeOtherViews->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    m_closeOtherViews->setText(i18n("Close Inactive Views"));
    connect(m_closeOtherViews, SIGNAL(triggered()), this, SLOT(slotCloseOtherViews()), Qt::QueuedConnection);

    m_closeOtherViews->setWhatsThis(i18n("Close every view but the active one"));

    m_hideOtherViews = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_hide_others"));
    m_hideOtherViews->setIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")));
    m_hideOtherViews->setText(i18n("Hide Inactive Views"));
    m_hideOtherViews->setCheckable(true);
    connect(m_hideOtherViews, SIGNAL(triggered(bool)), this, SLOT(slotHideOtherViews(bool)), Qt::QueuedConnection);

    m_hideOtherViews->setWhatsThis(i18n("Hide every view but the active one"));

    m_toggleSplitterOrientation = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_split_toggle"));
    m_toggleSplitterOrientation->setText(i18n("Toggle Orientation"));
    connect(m_toggleSplitterOrientation, SIGNAL(triggered()), this, SLOT(toggleSplitterOrientation()), Qt::QueuedConnection);

    m_toggleSplitterOrientation->setWhatsThis(i18n("Toggles the orientation of the current split view"));

    goNext = m_mainWindow->actionCollection()->addAction(QStringLiteral("go_next_split_view"));
    goNext->setText(i18n("Next Split View"));
    m_mainWindow->actionCollection()->setDefaultShortcut(goNext, Qt::Key_F8);
    connect(goNext, SIGNAL(triggered()), this, SLOT(activateNextView()));

    goNext->setWhatsThis(i18n("Make the next split view the active one."));

    goPrev = m_mainWindow->actionCollection()->addAction(QStringLiteral("go_prev_split_view"));
    goPrev->setText(i18n("Previous Split View"));
    m_mainWindow->actionCollection()->setDefaultShortcut(goPrev, Qt::SHIFT + Qt::Key_F8);
    connect(goPrev, SIGNAL(triggered()), this, SLOT(activatePrevView()));

    goPrev->setWhatsThis(i18n("Make the previous split view the active one."));

    QAction * a = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_split_move_right"));
    a->setText(i18n("Move Splitter Right"));
    connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterRight()));

    a->setWhatsThis(i18n("Move the splitter of the current view to the right"));

    a = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_split_move_left"));
    a->setText(i18n("Move Splitter Left"));
    connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterLeft()));

    a->setWhatsThis(i18n("Move the splitter of the current view to the left"));

    a = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_split_move_up"));
    a->setText(i18n("Move Splitter Up"));
    connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterUp()));

    a->setWhatsThis(i18n("Move the splitter of the current view up"));

    a = m_mainWindow->actionCollection()->addAction(QStringLiteral("view_split_move_down"));
    a->setText(i18n("Move Splitter Down"));
    connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterDown()));

    a->setWhatsThis(i18n("Move the splitter of the current view down"));
}

void KateViewManager::updateViewSpaceActions()
{
    m_closeView->setEnabled(m_viewSpaceList.count() > 1);
    m_closeOtherViews->setEnabled(m_viewSpaceList.count() > 1);
    m_toggleSplitterOrientation->setEnabled(m_viewSpaceList.count() > 1);
    goNext->setEnabled(m_viewSpaceList.count() > 1);
    goPrev->setEnabled(m_viewSpaceList.count() > 1);
}

void KateViewManager::slotDocumentNew()
{
    createView();
}

void KateViewManager::slotDocumentOpen()
{
    // try to start dialog in useful dir: either dir of current doc or last used one
    KTextEditor::View * const cv = activeView();
    QUrl startUrl = cv ? cv->document()->url() : QUrl();
    if (startUrl.isValid()) {
        m_lastOpenDialogUrl = startUrl;
    } else {
        startUrl = m_lastOpenDialogUrl;
    }
    const QList<QUrl> urls = QFileDialog::getOpenFileUrls(m_mainWindow, i18n("Open File"), startUrl);

    /**
     * emit size warning, for local files
     */
    QString fileListWithTooLargeFiles;
    Q_FOREACH(const QUrl & url, urls) {
        if (!url.isLocalFile()) {
            continue;
        }

        const auto size = QFile(url.toLocalFile()).size();
        if (size > FileSizeAboveToAskUserIfProceedWithOpen) {
            fileListWithTooLargeFiles += QString::fromLatin1("<li>%1 (%2MB)</li>").arg(url.fileName()).arg(size / 1024 / 1024);
        }
    }
    if (!fileListWithTooLargeFiles.isEmpty()) {
        const QString text = i18n("<p>You are attempting to open one or more large files:</p><ul>%1</ul><p>Do you want to proceed?</p><p><strong>Beware that kate may stop responding for some time when opening large files.</strong></p>");
        const auto ret = KMessageBox::warningYesNo(this, text.arg(fileListWithTooLargeFiles), i18n("Opening Large File"), KStandardGuiItem::cont(), KStandardGuiItem::stop());
        if (ret == KMessageBox::No) {
            return;
        }
    }

    // activate view of last opened document
    KateDocumentInfo docInfo;
    docInfo.openedByUser = true;
    if (KTextEditor::Document *lastID = openUrls(urls, QString(), false, docInfo)) {
        activateView(lastID);
    }
}

void KateViewManager::slotDocumentClose(KTextEditor::Document *document)
{
// prevent close document if only one view alive and the document of
    // it is not modified and empty !!!
    if ((KateApp::self()->documentManager()->documentList().size() == 1)
            && !document->isModified()
            && document->url().isEmpty()
            && document->documentEnd() == KTextEditor::Cursor::start()) {
        document->closeUrl();
        return;
    }

    // close document
    KateApp::self()->documentManager()->closeDocument(document);
}

void KateViewManager::slotDocumentClose()
{
    // no active view, do nothing
    if (!activeView()) {
        return;
    }

    slotDocumentClose(activeView()->document());

}

KTextEditor::Document *KateViewManager::openUrl(const QUrl &url,
        const QString &encoding,
        bool activate,
        bool isTempFile,
        const KateDocumentInfo &docInfo)
{
    KTextEditor::Document *doc = KateApp::self()->documentManager()->openUrl(url, encoding, isTempFile, docInfo);

    if (!doc->url().isEmpty()) {
        m_mainWindow->fileOpenRecent()->addUrl(doc->url());
    }

    if (activate) {
        activateView(doc);
    }

    return doc;
}

KTextEditor::Document *KateViewManager::openUrls(const QList<QUrl> &urls,
        const QString &encoding,
        bool isTempFile,
        const KateDocumentInfo &docInfo)
{
    QList<KTextEditor::Document *> docs = KateApp::self()->documentManager()->openUrls(urls, encoding, isTempFile, docInfo);

    foreach(const KTextEditor::Document * doc, docs) {
        if (!doc->url().isEmpty()) {
            m_mainWindow->fileOpenRecent()->addUrl(doc->url());
        }
    }

    return docs.isEmpty() ? 0 : docs.last();
}

KTextEditor::View *KateViewManager::openUrlWithView(const QUrl &url, const QString &encoding)
{
    KTextEditor::Document *doc = KateApp::self()->documentManager()->openUrl(url, encoding);

    if (!doc) {
        return nullptr;
    }

    if (!doc->url().isEmpty()) {
        m_mainWindow->fileOpenRecent()->addUrl(doc->url());
    }

    activateView(doc);

    return activeView();
}

void KateViewManager::openUrl(const QUrl &url)
{
    openUrl(url, QString());
}

KateMainWindow *KateViewManager::mainWindow()
{
    return m_mainWindow;
}

void KateViewManager::documentCreated(KTextEditor::Document *doc)
{
    // forward to currently active view space
    activeViewSpace()->registerDocument(doc);

    // to update open recent files on saving
    connect(doc, SIGNAL(documentSavedOrUploaded(KTextEditor::Document*,bool)), this, SLOT(documentSavedOrUploaded(KTextEditor::Document*,bool)));

    if (m_blockViewCreationAndActivation) {
        return;
    }

    if (!activeView()) {
        activateView(doc);
    }

    /**
     * check if we have any empty viewspaces and give them a view
     */
    Q_FOREACH(KateViewSpace * vs, m_viewSpaceList) {
        if (!vs->currentView()) {
            createView(activeView()->document(), vs);
        }
    }
}

void KateViewManager::aboutToDeleteDocuments(const QList<KTextEditor::Document *> &)
{
    /**
     * block view creation until the transaction is done
     * this shall not stack!
     */
    Q_ASSERT (!m_blockViewCreationAndActivation);
    m_blockViewCreationAndActivation = true;

    /**
     * disable updates hard (we can't use KateUpdateDisabler here, we have delayed signal
     */
    mainWindow()->setUpdatesEnabled(false);
}

void KateViewManager::documentsDeleted(const QList<KTextEditor::Document *> &)
{
    /**
     * again allow view creation
     */
    m_blockViewCreationAndActivation = false;

    /**
     * try to have active view around!
     */
    if (!activeView() && !KateApp::self()->documentManager()->documentList().isEmpty()) {
        createView(KateApp::self()->documentManager()->documentList().last());
    }

    /**
     * if we have one now, show them in all viewspaces that got empty!
     */
    if (KTextEditor::View *const newActiveView = activeView()) {
        /**
         * check if we have any empty viewspaces and give them a view
         */
        Q_FOREACH(KateViewSpace * vs, m_viewSpaceList) {
            if (!vs->currentView()) {
                createView(newActiveView->document(), vs);
            }
        }

        emit viewChanged(newActiveView);
    }

    /**
     * enable updates hard (we can't use KateUpdateDisabler here, we have delayed signal
     */
    mainWindow()->setUpdatesEnabled(true);
}

void KateViewManager::documentSavedOrUploaded(KTextEditor::Document *doc, bool)
{
    if (!doc->url().isEmpty()) {
        m_mainWindow->fileOpenRecent()->addUrl(doc->url());
    }
}

KTextEditor::View *KateViewManager::createView(KTextEditor::Document *doc, KateViewSpace *vs)
{
    if (m_blockViewCreationAndActivation) {
        return nullptr;
    }

    // create doc
    if (!doc) {
        doc = KateApp::self()->documentManager()->createDoc();
    }

    /**
     * create view, registers its XML gui itself
     * pass the view the correct main window
     */
    KTextEditor::View *view = (vs ? vs : activeViewSpace())->createView(doc);

    /**
     * remember this view, active == false, min age set
     * create activity resource
     */
    m_views[view].active = false;
    m_views[view].lruAge = m_minAge--;

#ifdef KActivities_FOUND
    m_views[view].activityResource = new KActivities::ResourceInstance(view->window()->winId(), view);
    m_views[view].activityResource->setUri(doc->url());
#endif

    // disable settings dialog action
    delete view->actionCollection()->action(QStringLiteral("set_confdlg"));
    delete view->actionCollection()->action(QStringLiteral("editor_options"));

    connect(view, SIGNAL(dropEventPass(QDropEvent*)), mainWindow(), SLOT(slotDropEvent(QDropEvent*)));
    connect(view, SIGNAL(focusIn(KTextEditor::View*)), this, SLOT(activateSpace(KTextEditor::View*)));

    viewCreated(view);

    if (!vs) {
        activateView(view);
    }

    return view;
}

bool KateViewManager::deleteView(KTextEditor::View *view)
{
    if (!view) {
        return true;
    }

    KateViewSpace *viewspace = static_cast<KateViewSpace *>(view->parentWidget()->parentWidget());

    viewspace->removeView(view);

    /**
     * deregister if needed
     */
    if (m_guiMergedView == view) {
        mainWindow()->guiFactory()->removeClient(m_guiMergedView);
        m_guiMergedView = nullptr;
    }

    // remove view from mapping and memory !!
    m_views.remove(view);
    delete view;
    return true;
}

KateViewSpace *KateViewManager::activeViewSpace()
{
    for (QList<KateViewSpace *>::const_iterator it = m_viewSpaceList.constBegin();
            it != m_viewSpaceList.constEnd(); ++it) {
        if ((*it)->isActiveSpace()) {
            return *it;
        }
    }

    // none active, so use the first we grab
    if (!m_viewSpaceList.isEmpty()) {
        m_viewSpaceList.first()->setActive(true);
        return m_viewSpaceList.first();
    }

    Q_ASSERT(false);
    return nullptr;
}

KTextEditor::View *KateViewManager::activeView()
{
    if (m_activeViewRunning) {
        return nullptr;
    }

    m_activeViewRunning = true;

    QHashIterator<KTextEditor::View *, ViewData> it(m_views);
    while (it.hasNext()) {
        it.next();
        if (it.value().active) {
            m_activeViewRunning = false;
            return it.key();
        }
    }

    // if we get to here, no view isActive()
    // first, try to get one from activeViewSpace()
    KateViewSpace *vs = activeViewSpace();
    if (vs && vs->currentView()) {
        activateView(vs->currentView());

        m_activeViewRunning = false;
        return vs->currentView();
    }

    // last attempt: just pick first
    if (!m_views.isEmpty()) {
        KTextEditor::View *v = m_views.begin().key();
        activateView(v);
        m_activeViewRunning = false;
        return v;
    }

    m_activeViewRunning = false;

    // no views exists!
    return nullptr;
}

void KateViewManager::setActiveSpace(KateViewSpace *vs)
{
    if (activeViewSpace()) {
        activeViewSpace()->setActive(false);
    }

    vs->setActive(true);
}

void KateViewManager::setActiveView(KTextEditor::View *view)
{
    if (activeView()) {
        m_views[activeView()].active = false;
    }

    if (view) {
        m_views[view].active = true;
    }
}

void KateViewManager::activateSpace(KTextEditor::View *v)
{
    if (!v) {
        return;
    }

    KateViewSpace *vs = static_cast<KateViewSpace *>(v->parentWidget()->parentWidget());

    if (!vs->isActiveSpace()) {
        setActiveSpace(vs);
        activateView(v);
    }
}

void KateViewManager::reactivateActiveView()
{
    KTextEditor::View *view = activeView();
    if (view) {
        m_views[view].active = false;
        activateView(view);
    }
}

void KateViewManager::activateView(KTextEditor::View *view)
{
    if (!view) {
        return;
    }

    Q_ASSERT (m_views.contains(view));
    if (!m_views[view].active) {
        // avoid flicker
        KateUpdateDisabler disableUpdates (mainWindow());

        if (!activeViewSpace()->showView(view)) {
            // since it wasn't found, give'em a new one
            createView(view->document());
            return;
        }

        setActiveView(view);

        bool toolbarVisible = mainWindow()->toolBar()->isVisible();
        if (toolbarVisible) {
            mainWindow()->toolBar()->hide();    // hide to avoid toolbar flickering
        }

        if (m_guiMergedView) {
            mainWindow()->guiFactory()->removeClient(m_guiMergedView);
            m_guiMergedView = nullptr;
        }


        if (!m_blockViewCreationAndActivation) {
            mainWindow()->guiFactory()->addClient(view);
            m_guiMergedView = view;
        }

        if (toolbarVisible) {
            mainWindow()->toolBar()->show();
        }

        // remember age of this view
        m_views[view].lruAge = m_minAge--;

        emit viewChanged(view);

#ifdef KActivities_FOUND
        // inform activity manager
        m_views[view].activityResource->setUri(view->document()->url());
        m_views[view].activityResource->notifyFocusedIn();
#endif
    }
}

KTextEditor::View *KateViewManager::activateView(KTextEditor::Document *d)
{
    // no doc with this id found
    if (!d) {
        return activeView();
    }

    // activate existing view if possible
    if (activeViewSpace()->showView(d)) {
        activateView(activeViewSpace()->currentView());
        return activeView();
    }

    // create new view otherwise
    createView(d);
    return activeView();
}

void KateViewManager::slotViewChanged()
{
    if (activeView() && !activeView()->hasFocus()) {
        activeView()->setFocus();
    }
}

void KateViewManager::activateNextView()
{
    int i = m_viewSpaceList.indexOf(activeViewSpace()) + 1;

    if (i >= m_viewSpaceList.count()) {
        i = 0;
    }

    setActiveSpace(m_viewSpaceList.at(i));
    activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewManager::activatePrevView()
{
    int i = m_viewSpaceList.indexOf(activeViewSpace()) - 1;

    if (i < 0) {
        i = m_viewSpaceList.count() - 1;
    }

    setActiveSpace(m_viewSpaceList.at(i));
    activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewManager::documentWillBeDeleted(KTextEditor::Document *doc)
{
    /**
     * collect all views of that document that belong to this manager
     */
    QList<KTextEditor::View *> closeList;
    Q_FOREACH (KTextEditor::View *v, doc->views()) {
        if (m_views.contains(v)) {
            closeList.append(v);
        }
    }

    while (!closeList.isEmpty()) {
        deleteView(closeList.takeFirst());
    }
}

void KateViewManager::closeView(KTextEditor::View *view)
{
    /**
     * kill view we want to kill
     */
    deleteView(view);

    /**
     * try to have active view around!
     */
    if (!activeView() && !KateApp::self()->documentManager()->documentList().isEmpty()) {
        createView(KateApp::self()->documentManager()->documentList().last());
    }

    /**
     * if we have one now, show them in all viewspaces that got empty!
     */
    if (KTextEditor::View *const newActiveView = activeView()) {
        /**
         * check if we have any empty viewspaces and give them a view
         */
        Q_FOREACH(KateViewSpace * vs, m_viewSpaceList) {
            if (!vs->currentView()) {
                createView(newActiveView->document(), vs);
            }
        }

        emit viewChanged(newActiveView);
    }
}

void KateViewManager::splitViewSpace(KateViewSpace *vs,  // = 0
                                     Qt::Orientation o) // = Qt::Horizontal
{
    // emergency: fallback to activeViewSpace, and if still invalid, abort
    if (!vs) {
        vs = activeViewSpace();
    }
    if (!vs) {
        return;
    }

    // get current splitter, and abort if null
    QSplitter *currentSplitter = qobject_cast<QSplitter *>(vs->parentWidget());
    if (!currentSplitter) {
        return;
    }

    // avoid flicker
    KateUpdateDisabler disableUpdates (mainWindow());

    // index where to insert new splitter/viewspace
    const int index = currentSplitter->indexOf(vs);

    // create new viewspace
    KateViewSpace *vsNew = new KateViewSpace(this);

    // only 1 children -> we are the root container. So simply set the orientation
    // and add the new view space, then correct the sizes to 50%:50%
    if (currentSplitter->count() == 1) {
        if (currentSplitter->orientation() != o) {
            currentSplitter->setOrientation(o);
        }
        QList<int> sizes = currentSplitter->sizes();
        sizes << (sizes.first() - currentSplitter->handleWidth()) / 2;
        sizes[0] = sizes[1];
        currentSplitter->insertWidget(index, vsNew);
        currentSplitter->setSizes(sizes);
    } else {
        // create a new QSplitter and replace vs with the splitter. vs and newVS are
        // the new children of the new QSplitter
        QSplitter *newContainer = new QSplitter(o);
        QList<int> currentSizes = currentSplitter->sizes();

        newContainer->addWidget(vs);
        newContainer->addWidget(vsNew);
        currentSplitter->insertWidget(index, newContainer);
        newContainer->show();

        // fix sizes of children of old and new splitter
        currentSplitter->setSizes(currentSizes);
        QList<int> newSizes = newContainer->sizes();
        newSizes[0] = (newSizes[0] + newSizes[1] - newContainer->handleWidth()) / 2;
        newSizes[1] = newSizes[0];
        newContainer->setSizes(newSizes);
    }

    m_viewSpaceList.append(vsNew);
    activeViewSpace()->setActive(false);
    vsNew->setActive(true);
    vsNew->show();

    createView((KTextEditor::Document *)activeView()->document());

    updateViewSpaceActions();
}

void KateViewManager::closeViewSpace(KTextEditor::View *view)
{
    KateViewSpace *space;

    if (view) {
        space = static_cast<KateViewSpace *>(view->parentWidget()->parentWidget());
    } else {
        space = activeViewSpace();
    }
    removeViewSpace(space);
}

void KateViewManager::toggleSplitterOrientation()
{
    KateViewSpace *vs = activeViewSpace();
    if (!vs) {
        return;
    }

    // get current splitter, and abort if null
    QSplitter *currentSplitter = qobject_cast<QSplitter *>(vs->parentWidget());
    if (!currentSplitter || (currentSplitter->count() == 1)) {
        return;
    }

    // avoid flicker
    KateUpdateDisabler disableUpdates (mainWindow());

    // toggle orientation
    if (currentSplitter->orientation() == Qt::Horizontal) {
        currentSplitter->setOrientation(Qt::Vertical);
    } else {
        currentSplitter->setOrientation(Qt::Horizontal);
    }
}

bool KateViewManager::viewsInSameViewSpace(KTextEditor::View *view1,
                                           KTextEditor::View *view2)
{
    if (!view1 || !view2) {
        return false;
    }
    if (m_viewSpaceList.size() == 1) {
        return true;
    }

    KateViewSpace *vs1 = static_cast<KateViewSpace *>(view1->parentWidget()->parentWidget());
    KateViewSpace *vs2 = static_cast<KateViewSpace *>(view2->parentWidget()->parentWidget());
    return vs1 && (vs1 == vs2);
}

void KateViewManager::removeViewSpace(KateViewSpace *viewspace)
{
    // abort if viewspace is 0
    if (!viewspace) {
        return;
    }

    // abort if this is the last viewspace
    if (m_viewSpaceList.count() < 2) {
        return;
    }

    // get current splitter
    QSplitter *currentSplitter = qobject_cast<QSplitter *>(viewspace->parentWidget());

    // no splitter found, bah
    if (!currentSplitter) {
        return;
    }

    //
    // 1. get LRU document list from current viewspace
    // 2. delete current view space
    // 3. add LRU documents from deleted viewspace to new active viewspace
    //

    // backup LRU list
    const QVector<KTextEditor::Document*> lruDocumntsList = viewspace->lruDocumentList();

    // avoid flicker
    KateUpdateDisabler disableUpdates (mainWindow());

    // delete views of the viewspace
    while (viewspace->currentView()) {
        deleteView(viewspace->currentView());
    }

    // cu viewspace
    m_viewSpaceList.removeAt(m_viewSpaceList.indexOf(viewspace));
    delete viewspace;

    // at this point, the splitter has exactly 1 child
    Q_ASSERT(currentSplitter->count() == 1);

    // if we are not the root splitter, move the child one level up and delete
    // the splitter then.
    if (currentSplitter != this) {
        // get parent splitter
        QSplitter *parentSplitter = qobject_cast<QSplitter *>(currentSplitter->parentWidget());

        // only do magic if found ;)
        if (parentSplitter) {
            int index = parentSplitter->indexOf(currentSplitter);

            // save current splitter size, as the removal of currentSplitter looses the info
            QList<int> parentSizes = parentSplitter->sizes();
            parentSplitter->insertWidget(index, currentSplitter->widget(0));
            delete currentSplitter;
            // now restore the sizes again
            parentSplitter->setSizes(parentSizes);
        }
    } else if (QSplitter *splitter = qobject_cast<QSplitter *>(currentSplitter->widget(0))) {
        // we are the root splitter and have only one child, which is also a splitter
        // -> eliminate the redundant splitter and move both children into the root splitter
        QList<int> sizes = splitter->sizes();
        // adapt splitter orientation to the splitter we are about to delete
        currentSplitter->setOrientation(splitter->orientation());
        currentSplitter->addWidget(splitter->widget(0));
        currentSplitter->addWidget(splitter->widget(0));
        delete splitter;
        currentSplitter->setSizes(sizes);
    }

    // merge docuemnts of closed view space
    activeViewSpace()->mergeLruList(lruDocumntsList);

    // find the view that is now active.
    KTextEditor::View *v = activeViewSpace()->currentView();
    if (v) {
        activateView(v);
    }

    updateViewSpaceActions();

    emit viewChanged(v);
}

void KateViewManager::slotCloseOtherViews()
{
    // avoid flicker
    KateUpdateDisabler disableUpdates(mainWindow());

    const KateViewSpace *active = activeViewSpace();
    foreach(KateViewSpace  * v, m_viewSpaceList) {
        if (active != v) {
            removeViewSpace(v);
        }
    }
}

void KateViewManager::slotHideOtherViews(bool hideOthers)
{
    // avoid flicker
    KateUpdateDisabler disableUpdates(mainWindow());

    const KateViewSpace *active = activeViewSpace();
    foreach(KateViewSpace * v, m_viewSpaceList) {
        if (active != v) {
            v->setVisible(!hideOthers);
        }
    }

    // disable the split actions, if we are in single-view-mode
    m_splitViewVert->setDisabled(hideOthers);
    m_splitViewHoriz->setDisabled(hideOthers);
    m_closeView->setDisabled(hideOthers);
    m_closeOtherViews->setDisabled(hideOthers);
    m_toggleSplitterOrientation->setDisabled(hideOthers);
}

/**
 * session config functions
 */
void KateViewManager::saveViewConfiguration(KConfigGroup &config)
{
    // set Active ViewSpace to 0, just in case there is none active (would be
    // strange) and config somehow has previous value set
    config.writeEntry("Active ViewSpace", 0);

    m_splitterIndex = 0;
    saveSplitterConfig(this, config.config(), config.name());
}

void KateViewManager::restoreViewConfiguration(const KConfigGroup &config)
{
    /**
     * remove the single client that is registered at the factory, if any
     */
    if (m_guiMergedView) {
        mainWindow()->guiFactory()->removeClient(m_guiMergedView);
        m_guiMergedView = nullptr;
    }

    /**
     * delete viewspaces, they will delete the views
     */
    qDeleteAll(m_viewSpaceList);
    m_viewSpaceList.clear();

    /**
     * delete mapping of now deleted views
     */
    m_views.clear();

    // reset lru history, too!
    m_minAge = 0;

    // start recursion for the root splitter (Splitter 0)
    restoreSplitter(config.config(), config.name() + QStringLiteral("-Splitter 0"), this, config.name());

    // finally, make the correct view from the last session active
    int lastViewSpace = config.readEntry("Active ViewSpace", 0);
    if (lastViewSpace > m_viewSpaceList.size()) {
        lastViewSpace = 0;
    }
    if (lastViewSpace >= 0 && lastViewSpace < m_viewSpaceList.size()) {
        setActiveSpace(m_viewSpaceList.at(lastViewSpace));
        // activate correct view (wish #195435, #188764)
        activateView(m_viewSpaceList.at(lastViewSpace)->currentView());
        // give view the focus to avoid focus stealing by toolviews / plugins
        m_viewSpaceList.at(lastViewSpace)->currentView()->setFocus();
    }

    // emergency
    if (m_viewSpaceList.empty()) {
        // kill bad children
        while (count()) {
            delete widget(0);
        }

        KateViewSpace *vs = new KateViewSpace(this, nullptr);
        addWidget(vs);
        vs->setActive(true);
        m_viewSpaceList.append(vs);

        /**
         * activate at least one document!
         */
        activateView(KateApp::self()->documentManager()->documentList().last());
        if (!vs->currentView()) {
            createView(activeView()->document(), vs);
        }
    }

    updateViewSpaceActions();
}

QString KateViewManager::saveSplitterConfig(QSplitter *s, KConfigBase *configBase, const QString &viewConfGrp)
{
    /**
     * avoid to export invisible view spaces
     * else they will stick around for ever in sessions
     * bug 358266 - code initially done during load
     * bug 381433 - moved code to save
     */

    /**
     * create new splitter name, might be not used
     */
    const auto grp = QString(viewConfGrp + QStringLiteral("-Splitter %1")).arg(m_splitterIndex);
    ++m_splitterIndex;

    // a QSplitter has two children, either QSplitters and/or KateViewSpaces
    // special case: root splitter might have only one child (just for info)
    QStringList childList;
    const auto sizes = s->sizes();
    for (int it = 0; it < s->count(); ++it) {
        // skip empty sized invisible ones, if not last one, we need one thing at least
        if ((sizes[it] == 0) && ((it + 1 < s->count()) || !childList.empty()))
            continue;

        // For KateViewSpaces, ask them to save the file list.
        auto obj = s->widget(it);
        if (auto kvs = qobject_cast<KateViewSpace *>(obj)) {
            childList.append(QString(viewConfGrp + QStringLiteral("-ViewSpace %1")).arg(m_viewSpaceList.indexOf(kvs)));
            kvs->saveConfig(configBase, m_viewSpaceList.indexOf(kvs), viewConfGrp);
            // save active viewspace
            if (kvs->isActiveSpace()) {
                KConfigGroup viewConfGroup(configBase, viewConfGrp);
                viewConfGroup.writeEntry("Active ViewSpace", m_viewSpaceList.indexOf(kvs));
            }
        }
        // for QSplitters, recurse
        else if (auto splitter = qobject_cast<QSplitter *>(obj)) {
            childList.append(saveSplitterConfig(splitter, configBase, viewConfGrp));
        }
    }

    // if only one thing, skip splitter config export, if not top splitter
    if ((s != this) && (childList.size() == 1))
        return childList.at(0);

    // Save sizes, orient, children for this splitter
    KConfigGroup config(configBase, grp);
    config.writeEntry("Sizes", sizes);
    config.writeEntry("Orientation", int(s->orientation()));
    config.writeEntry("Children", childList);
    return grp;
}

void KateViewManager::restoreSplitter(const KConfigBase *configBase, const QString &group,
                                      QSplitter *parent, const QString &viewConfGrp)
{
    KConfigGroup config(configBase, group);

    parent->setOrientation((Qt::Orientation)config.readEntry("Orientation", int(Qt::Horizontal)));

    QStringList children = config.readEntry("Children", QStringList());
    for (QStringList::Iterator it = children.begin(); it != children.end(); ++it) {
        // for a viewspace, create it and open all documents therein.
        if ((*it).startsWith(viewConfGrp + QStringLiteral("-ViewSpace"))) {
            KateViewSpace *vs = new KateViewSpace(this, nullptr);
            m_viewSpaceList.append(vs);
            // make active so that the view created in restoreConfig has this
            // new view space as parent.
            setActiveSpace(vs);

            parent->addWidget(vs);
            vs->restoreConfig(this, configBase, *it);
            vs->show();
        } else {
            // for a splitter, recurse.
            restoreSplitter(configBase, *it, new QSplitter(parent), viewConfGrp);
        }
    }

    // set sizes
    parent->setSizes(config.readEntry("Sizes", QList<int>()));
    parent->show();
}

void KateViewManager::moveSplitter(Qt::Key key, int repeats)
{
    if (repeats < 1) {
        return;
    }

    KateViewSpace *vs = activeViewSpace();
    if (!vs) {
        return;
    }

    QSplitter *currentSplitter = qobject_cast<QSplitter *>(vs->parentWidget());

    if (!currentSplitter) {
        return;
    }
    if (currentSplitter->count() == 1) {
        return;
    }

    int move = 4 * repeats;
    // try to use font height in pixel to move splitter
    {
        KTextEditor::Attribute::Ptr attrib(vs->currentView()->defaultStyleAttribute(KTextEditor::dsNormal));
        QFontMetrics fm(attrib->font());
        move = fm.height() * repeats;
    }

    QWidget *currentWidget = (QWidget *)vs;
    bool foundSplitter = false;

    // find correct splitter to be moved
    while (currentSplitter && currentSplitter->count() != 1) {

        if (currentSplitter->orientation() == Qt::Horizontal
                && (key == Qt::Key_Right || key == Qt::Key_Left)) {
            foundSplitter = true;
        }

        if (currentSplitter->orientation() == Qt::Vertical
                && (key == Qt::Key_Up || key == Qt::Key_Down)) {
            foundSplitter = true;
        }

        // if the views within the current splitter can be resized, resize them
        if (foundSplitter) {
            QList<int> currentSizes = currentSplitter->sizes();
            int index = currentSplitter->indexOf(currentWidget);

            if ((index == 0 && (key == Qt::Key_Left || key == Qt::Key_Up))
                    || (index == 1  && (key == Qt::Key_Right || key == Qt::Key_Down))) {
                currentSizes[index] -= move;
            }

            if ((index == 0  && (key == Qt::Key_Right || key == Qt::Key_Down))
                    || (index == 1 && (key == Qt::Key_Left || key == Qt::Key_Up))) {
                currentSizes[index] += move;
            }

            if (index == 0 && (key == Qt::Key_Right || key == Qt::Key_Down)) {
                currentSizes[index + 1] -= move;
            }

            if (index == 0 && (key == Qt::Key_Left || key == Qt::Key_Up)) {
                currentSizes[index + 1] += move;
            }

            if (index == 1 && (key == Qt::Key_Right || key == Qt::Key_Down)) {
                currentSizes[index - 1] += move;
            }

            if (index == 1 && (key == Qt::Key_Left || key == Qt::Key_Up)) {
                currentSizes[index - 1] -= move;
            }

            currentSplitter->setSizes(currentSizes);
            break;
        }

        currentWidget = (QWidget *)currentSplitter;
        // the parent of the current splitter will become the current splitter
        currentSplitter = qobject_cast<QSplitter *>(currentSplitter->parentWidget());
    }
}

