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
#include "kateviewmanager.moc"

#include "katemainwindow.h"
#include "katedocmanager.h"
#include "kateviewspace.h"

#include <kate/mainwindow.h>

#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <KTextEditor/Attribute>
#include <KTextEditor/HighlightInterface>

#include <KAction>
#include <KActionCollection>
#include <KCmdLineArgs>
#include <kdebug.h>
#include <KEncodingFileDialog>
#include <KIconLoader>
#include <KGlobal>
#include <KStringHandler>
#include <KLocale>
#include <KToolBar>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KConfig>
#include <KConfigGroup>
#include <kstandardaction.h>
#include <KStandardDirs>
#include <KGlobalSettings>
#include <kstandardshortcut.h>

#ifdef KActivities_FOUND
#include <KActivities/ResourceInstance>
#endif

#include <QApplication>
#include <QObject>
#include <QFileInfo>
#include <QToolButton>
#include <QTimer>
#include <QMenu>

//END Includes

KateViewManager::KateViewManager (QWidget *parentW, KateMainWindow *parent)
    : QSplitter  (parentW)
    , m_mainWindow(parent)
    , m_blockViewCreationAndActivation (false)
    , m_activeViewRunning (false)
    , m_minAge (0)
{
  // while init
  m_init = true;

  // important, set them up, as we use them in other methodes
  setupActions ();

  guiMergedView = 0;

  // resize mode
  setOpaqueResize( KGlobalSettings::opaqueResize() );

  KateViewSpace* vs = new KateViewSpace( this, 0 );
  addWidget (vs);

  vs->setActive( true );
  m_viewSpaceList.append(vs);

  connect( this, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()) );
  connect(KateDocManager::self(), SIGNAL(initialDocumentReplaced()), this, SIGNAL(viewChanged()));

  connect(KateDocManager::self(), SIGNAL(documentCreated(KTextEditor::Document*)), this, SLOT(documentCreated(KTextEditor::Document*)));
  connect(KateDocManager::self(), SIGNAL(documentDeleted(KTextEditor::Document*)), this, SLOT(documentDeleted(KTextEditor::Document*)));

  // register all already existing documents
  m_blockViewCreationAndActivation = true;
  const QList<KTextEditor::Document*> &docs = KateDocManager::self()->documentList ();
  foreach (KTextEditor::Document *doc, docs)
    documentCreated (doc);
  m_blockViewCreationAndActivation = false;

  // init done
  m_init = false;
}

KateViewManager::~KateViewManager ()
{
  // make sure all xml gui clients are removed to avoid warnings on exit
  foreach (KTextEditor::View* view, m_viewList)
    mainWindow()->guiFactory()->removeClient(view);
}

void KateViewManager::setupActions ()
{
  KAction *a;

  /**
   * view splitting
   */
  a = m_mainWindow->actionCollection()->addAction("view_split_vert");
  a->setIcon( KIcon("view-split-left-right") );
  a->setText( i18n("Split Ve&rtical") );
  a->setShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_L );
  connect(a, SIGNAL(triggered()), this, SLOT(slotSplitViewSpaceVert()));

  a->setWhatsThis(i18n("Split the currently active view vertically into two views."));

  a = m_mainWindow->actionCollection()->addAction("view_split_horiz");
  a->setIcon( KIcon("view-split-top-bottom") );
  a->setText( i18n("Split &Horizontal") );
  a->setShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_T );
  connect(a, SIGNAL(triggered()), this, SLOT(slotSplitViewSpaceHoriz()));

  a->setWhatsThis(i18n("Split the currently active view horizontally into two views."));

  m_closeView = m_mainWindow->actionCollection()->addAction("view_close_current_space");
  m_closeView->setIcon( KIcon("view-close") );
  m_closeView->setText( i18n("Cl&ose Current View") );
  m_closeView->setShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_R );
  connect(m_closeView, SIGNAL(triggered()), this, SLOT(slotCloseCurrentViewSpace()));

  m_closeView->setWhatsThis(i18n("Close the currently active split view"));

  m_closeOtherViews = m_mainWindow->actionCollection()->addAction("view_close_others");
  m_closeOtherViews->setIcon(KIcon("view-close"));
  m_closeOtherViews->setText(i18n("Close Inactive Views"));
  connect(m_closeOtherViews, SIGNAL(triggered()), this, SLOT(slotCloseOtherViews()));

  m_closeOtherViews->setWhatsThis(i18n("Close every view but the active one"));

  goNext = m_mainWindow->actionCollection()->addAction( "go_next_split_view" );
  goNext->setText( i18n("Next Split View") );
  goNext->setShortcut( Qt::Key_F8 );
  connect(goNext, SIGNAL(triggered()), this, SLOT(activateNextView()));

  goNext->setWhatsThis(i18n("Make the next split view the active one."));

  goPrev = m_mainWindow->actionCollection()->addAction( "go_prev_split_view" );
  goPrev->setText( i18n("Previous Split View") );
  goPrev->setShortcut( Qt::SHIFT + Qt::Key_F8 );
  connect(goPrev, SIGNAL(triggered()), this, SLOT(activatePrevView()));

  goPrev->setWhatsThis(i18n("Make the previous split view the active one."));

  a = m_mainWindow->actionCollection()->addAction("view_split_move_right");
  a->setText( i18n("Move Splitter Right") );
  connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterRight()));

  a->setWhatsThis(i18n("Move the splitter of the current view to the right"));

  a = m_mainWindow->actionCollection()->addAction("view_split_move_left");
  a->setText( i18n("Move Splitter Left") );
  connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterLeft()));

  a->setWhatsThis(i18n("Move the splitter of the current view to the left"));

  a = m_mainWindow->actionCollection()->addAction("view_split_move_up");
  a->setText( i18n("Move Splitter Up") );
  connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterUp()));

  a->setWhatsThis(i18n("Move the splitter of the current view up"));

  a = m_mainWindow->actionCollection()->addAction("view_split_move_down");
  a->setText( i18n("Move Splitter Down") );
  connect(a, SIGNAL(triggered()), this, SLOT(moveSplitterDown()));

  a->setWhatsThis(i18n("Move the splitter of the current view down"));

  /*
   * Status Bar Items menu
   */
  m_cursorPosToggle = new KToggleAction(i18n("Show Cursor Position"), this);
  m_mainWindow->actionCollection()->addAction( "show_cursor_pos", m_cursorPosToggle );
  connect(m_cursorPosToggle, SIGNAL(toggled(bool)),
          this, SIGNAL(cursorPositionItemVisibilityChanged(bool)));

  m_charCountToggle = new KToggleAction(i18n("Show Characters Count"), this);
  m_mainWindow->actionCollection()->addAction( "show_char_count", m_charCountToggle );
  connect(m_charCountToggle, SIGNAL(toggled(bool)),
          this, SIGNAL(charactersCountItemVisibilityChanged(bool)));

  m_insertModeToggle = new KToggleAction(i18n("Show Insertion Mode"), this);
  m_mainWindow->actionCollection()->addAction( "show_insert_mode", m_insertModeToggle );
  connect(m_insertModeToggle, SIGNAL(toggled(bool)),
          this, SIGNAL(insertModeItemVisibilityChanged(bool)));

  m_selectModeToggle = new KToggleAction(i18n("Show Selection Mode"), this);
  m_mainWindow->actionCollection()->addAction( "show_select_mode", m_selectModeToggle );
  connect(m_selectModeToggle, SIGNAL(toggled(bool)),
          this, SIGNAL(selectModeItemVisibilityChanged(bool)));

  m_encodingToggle = new KToggleAction(i18n("Show Encoding"), this);
  m_mainWindow->actionCollection()->addAction( "show_encoding", m_encodingToggle );
  connect(m_encodingToggle, SIGNAL(toggled(bool)),
          this, SIGNAL(encodingItemVisibilityChanged(bool)));

  m_docNameToggle = new KToggleAction(i18n("Show Document Name"), this);
  m_mainWindow->actionCollection()->addAction( "show_doc_name", m_docNameToggle );
  connect(m_docNameToggle, SIGNAL(toggled(bool)),
          this, SIGNAL(documentNameItemVisibilityChanged(bool)));
}

void KateViewManager::updateViewSpaceActions ()
{
  m_closeView->setEnabled (viewSpaceCount() > 1);
  m_closeOtherViews->setEnabled(viewSpaceCount() > 1);
  goNext->setEnabled (viewSpaceCount() > 1);
  goPrev->setEnabled (viewSpaceCount() > 1);
}

void KateViewManager::setViewActivationBlocked (bool block)
{
  m_blockViewCreationAndActivation = block;
}

void KateViewManager::slotDocumentNew ()
{
  createView ();
}

void KateViewManager::slotDocumentOpen ()
{
  KTextEditor::View *cv = activeView();

  if (cv)
  {
    KEncodingFileDialog::Result r = KEncodingFileDialog::getOpenUrlsAndEncoding(
                                      KateDocManager::self()->editor()->defaultEncoding(),
                                      cv->document()->url().url(),
                                      QString(), m_mainWindow, i18n("Open File"));

    KateDocumentInfo docInfo;
    docInfo.openedByUser = true;

    KTextEditor::Document *lastID = 0;
    for (KUrl::List::Iterator i = r.URLs.begin(); i != r.URLs.end(); ++i)
      lastID = openUrl( *i, r.encoding, false, false, docInfo);

    if (lastID)
      activateView (lastID);
  }
}

void KateViewManager::slotDocumentClose(KTextEditor::Document *document) {
// prevent close document if only one view alive and the document of
  // it is not modified and empty !!!
  if ( (KateDocManager::self()->documents() == 1)
       && !document->isModified()
       && document->url().isEmpty()
       && document->documentEnd() == KTextEditor::Cursor::start() )
  {
    document->closeUrl();
    return;
  }

  // close document
  KateDocManager::self()->closeDocument (document);
}


void KateViewManager::slotDocumentClose ()
{
  // no active view, do nothing
  if (!activeView()) return;

  slotDocumentClose(activeView()->document());


}

KTextEditor::Document *KateViewManager::openUrl (const KUrl &url,
                                                 const QString& encoding,
                                                 bool activate,
                                                 bool isTempFile,
                                                 const KateDocumentInfo& docInfo)
{
  KTextEditor::Document *doc = KateDocManager::self()->openUrl (url, encoding, isTempFile, docInfo);

  if (!doc->url().isEmpty())
    m_mainWindow->fileOpenRecent->addUrl( doc->url() );

  if (activate)
    activateView( doc );

  return doc;
}

KTextEditor::View *KateViewManager::openUrlWithView (const KUrl &url, const QString& encoding)
{
  KTextEditor::Document *doc = KateDocManager::self()->openUrl (url, encoding);

  if (!doc)
    return 0;

  if (!doc->url().isEmpty())
    m_mainWindow->fileOpenRecent->addUrl( doc->url() );

  activateView( doc );

  return activeView ();
}

void KateViewManager::openUrl (const KUrl &url)
{
  openUrl (url, QString());
}

KateMainWindow *KateViewManager::mainWindow()
{
  return m_mainWindow;
}

bool KateViewManager::isCursorPositionVisible() const
{
  return m_cursorPosToggle->isChecked();
}

bool KateViewManager::isCharactersCountVisible() const
{
  return m_charCountToggle->isChecked();
}

bool KateViewManager::isInsertModeVisible() const
{
  return m_insertModeToggle->isChecked();
}

bool KateViewManager::isSelectModeVisible() const
{
  return m_selectModeToggle->isChecked();
}

bool KateViewManager::isEncodingVisible() const
{
  return m_encodingToggle->isChecked();
}

bool KateViewManager::isDocumentNameVisible() const
{
  return m_docNameToggle->isChecked();
}

// VIEWSPACE

void KateViewManager::documentCreated (KTextEditor::Document *doc)
{
  // to update open recent files on saving
  connect (doc, SIGNAL(documentSavedOrUploaded(KTextEditor::Document*,bool)), this, SLOT(documentSavedOrUploaded(KTextEditor::Document*,bool)));

  if (m_blockViewCreationAndActivation) return;

  if (!activeView())
    activateView (doc);
}

void KateViewManager::documentDeleted (KTextEditor::Document *)
{
  if (m_blockViewCreationAndActivation) return;

  // just for the case we close a document out of many and this was the active one
  // if all docs are closed, this will be handled by the documentCreated
  if (!activeView() && (KateDocManager::self()->documents() > 0))
    createView (KateDocManager::self()->document(KateDocManager::self()->documents() - 1));
}

void KateViewManager::documentSavedOrUploaded(KTextEditor::Document *doc, bool)
{
  if (!doc->url().isEmpty())
    m_mainWindow->fileOpenRecent->addUrl( doc->url() );
}

bool KateViewManager::createView ( KTextEditor::Document *doc )
{
  if (m_blockViewCreationAndActivation) return false;

  // create doc
  if (!doc)
    doc = KateDocManager::self()->createDoc ();

  // create view, registers its XML gui itself
  KTextEditor::View *view = (KTextEditor::View *) doc->createView (activeViewSpace()->stack);

  m_viewList.append (view);
  m_activeStates[view] = false;

  // disable settings dialog action
  delete view->actionCollection()->action( "set_confdlg" );
  delete view->actionCollection()->action( "editor_options" );

  //view->setContextMenu(view->defaultContextMenu());

  connect(view, SIGNAL(dropEventPass(QDropEvent*)), mainWindow(), SLOT(slotDropEvent(QDropEvent*)));
  connect(view, SIGNAL(focusIn(KTextEditor::View*)), this, SLOT(activateSpace(KTextEditor::View*)));

  activeViewSpace()->addView( view );

  viewCreated(view);

#ifdef KActivities_FOUND
  if (!m_activityResources.contains(view)) {
    m_activityResources[view] = new KActivities::ResourceInstance(view->window()->winId(), view);
  }
  m_activityResources[view]->setUri(doc->url());
#endif

  activateView( view );

  return true;
}

bool KateViewManager::deleteView (KTextEditor::View *view, bool delViewSpace)
{
  if (!view) return true;

  KateViewSpace *viewspace = static_cast<KateViewSpace *>(view->parentWidget()->parentWidget());

  viewspace->removeView (view);

  mainWindow()->guiFactory ()->removeClient (view);

#ifdef KActivities_FOUND
  m_activityResources.remove(view);
#endif

  // kill LRU mapping
  m_lruViews.remove (view);

  // remove view from list and memory !!
  m_viewList.removeAt ( m_viewList.indexOf (view) );
  m_activeStates.remove (view);
  delete view;
  view = 0L;

  if (delViewSpace)
    if ( viewspace->viewCount() == 0 )
      removeViewSpace( viewspace );

  return true;
}

KateViewSpace* KateViewManager::activeViewSpace ()
{
  for ( QList<KateViewSpace*>::const_iterator it = m_viewSpaceList.constBegin();
        it != m_viewSpaceList.constEnd(); ++it )
  {
    if ( (*it)->isActiveSpace() )
      return *it;
  }

  // none active, so use the first we grab
  if ( !m_viewSpaceList.isEmpty() )
  {
    m_viewSpaceList.first()->setActive( true );
    return m_viewSpaceList.first();
  }

  return 0L;
}

KTextEditor::View* KateViewManager::activeView ()
{
  if (m_activeViewRunning)
    return 0L;

  m_activeViewRunning = true;

  for ( QList<KTextEditor::View*>::const_iterator it = m_viewList.constBegin();
        it != m_viewList.constEnd(); ++it )
  {
    if ( m_activeStates[*it] )
    {
      m_activeViewRunning = false;
      return *it;
    }
  }

  // if we get to here, no view isActive()
  // first, try to get one from activeViewSpace()
  KateViewSpace* vs = activeViewSpace();
  if ( vs && vs->currentView() )
  {
    activateView (vs->currentView());

    m_activeViewRunning = false;
    return vs->currentView();
  }

  // last attempt: just pick first
  if ( !m_viewList.isEmpty() )
  {
    activateView (m_viewList.first());

    m_activeViewRunning = false;
    return m_viewList.first();
  }

  m_activeViewRunning = false;

  // no views exists!
  return 0L;
}

void KateViewManager::setActiveSpace ( KateViewSpace* vs )
{
  if (activeViewSpace())
    activeViewSpace()->setActive( false );

  vs->setActive( true, viewSpaceCount() > 1 );
}

void KateViewManager::setActiveView ( KTextEditor::View* view )
{
  if (activeView())
    m_activeStates[activeView()] = false;

  m_activeStates[view] = true;
}

void KateViewManager::activateSpace (KTextEditor::View* v)
{
  if (!v) return;

  KateViewSpace* vs = static_cast<KateViewSpace*>(v->parentWidget()->parentWidget());

  if (!vs->isActiveSpace())
  {
    setActiveSpace (vs);
    activateView(v);
  }
}

void KateViewManager::reactivateActiveView()
{
  KTextEditor::View *view = activeView();
  if (view)
  {
    m_activeStates[view] = false;
    activateView(view);
  }
}

void KateViewManager::activateView ( KTextEditor::View *view )
{
  if (!view) return;

#ifdef KActivities_FOUND
  if (m_activityResources.contains(view)) {
    m_activityResources[view]->setUri(view->document()->url());
    m_activityResources[view]->notifyFocusedIn();
  }
#endif

  if (!m_activeStates[view])
  {
    if ( !activeViewSpace()->showView(view) )
    {
      // since it wasn't found, give'em a new one
      createView( view->document() );
      return;
    }

    setActiveView (view);

    mainWindow()->setUpdatesEnabled( false );
    bool toolbarVisible = mainWindow()->toolBar()->isVisible();
    if (toolbarVisible)
      mainWindow()->toolBar()->hide(); // hide to avoid toolbar flickering

    if (guiMergedView)
      mainWindow()->guiFactory()->removeClient( guiMergedView );

    guiMergedView = view;

    if (!m_blockViewCreationAndActivation)
      mainWindow()->guiFactory()->addClient( view );

    if (toolbarVisible)
      mainWindow()->toolBar()->show();
    mainWindow()->setUpdatesEnabled( true );

    // remember age of this view
    m_lruViews[view] = m_minAge--;

    emit viewChanged();
  }
}

KTextEditor::View *KateViewManager::activateView( KTextEditor::Document *d )
{
  // no doc with this id found
  if (!d)
    return activeView ();

  // activate existing view if possible
  if ( activeViewSpace()->showView(d) )
  {
    activateView( activeViewSpace()->currentView() );
    return activeView ();
  }

  // create new view otherwise
  createView (d);
  return activeView ();
}

int KateViewManager::viewCount () const
{
  return m_viewList.count();
}

int KateViewManager::viewSpaceCount () const
{
  return m_viewSpaceList.count();
}

void KateViewManager::slotViewChanged()
{
  if ( activeView() && !activeView()->hasFocus())
    activeView()->setFocus();
}

void KateViewManager::activateNextView()
{
  int i = m_viewSpaceList.indexOf (activeViewSpace()) + 1;

  if (i >= m_viewSpaceList.count())
    i = 0;

  setActiveSpace (m_viewSpaceList.at(i));
  activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewManager::activatePrevView()
{
  int i = m_viewSpaceList.indexOf (activeViewSpace()) - 1;

  if (i < 0)
    i = m_viewSpaceList.count() - 1;

  setActiveSpace (m_viewSpaceList.at(i));
  activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewManager::closeViews(KTextEditor::Document *doc)
{
  QList<KTextEditor::View*> closeList;

  for ( QList<KTextEditor::View*>::const_iterator it = m_viewList.constBegin();
        it != m_viewList.constEnd(); ++it)
  {
    if ( (*it)->document() == doc )
      closeList.append( *it );
  }

  while ( !closeList.isEmpty() )
    deleteView( closeList.takeFirst(), true );

  if (m_blockViewCreationAndActivation) return;
  QTimer::singleShot(0, this, SIGNAL(viewChanged()));
}

void KateViewManager::splitViewSpace( KateViewSpace* vs, // = 0
    Qt::Orientation o )// = Qt::Horizontal
{
  // emergency: fallback to activeViewSpace, and if still invalid, abort
  if (!vs) vs = activeViewSpace();
  if (!vs) return;

  // get current splitter, and abort if null
  QSplitter *currentSplitter = qobject_cast<QSplitter*>(vs->parentWidget());
  if (!currentSplitter) return;

  // index where to insert new splitter/viewspace
  const int index = currentSplitter->indexOf( vs );

  // create new viewspace
  KateViewSpace* vsNew = new KateViewSpace( this );

  // only 1 children -> we are the root container. So simply set the orientation
  // and add the new view space, then correct the sizes to 50%:50%
  if (currentSplitter->count() == 1)
  {
    if( currentSplitter->orientation() != o )
      currentSplitter->setOrientation( o );
    QList<int> sizes = currentSplitter->sizes();
    sizes << (sizes.first() - currentSplitter->handleWidth() ) / 2;
    sizes[0] = sizes[1];
    currentSplitter->insertWidget( index, vsNew );
    currentSplitter->setSizes( sizes );
  }
  else
  {
    // create a new QSplitter and replace vs with the splitter. vs and newVS are
    // the new children of the new QSplitter
    QSplitter* newContainer = new QSplitter( o );
    newContainer->setOpaqueResize( KGlobalSettings::opaqueResize() );
    QList<int> currentSizes = currentSplitter->sizes();

    newContainer->addWidget( vs );
    newContainer->addWidget( vsNew );
    currentSplitter->insertWidget( index, newContainer );
    newContainer->show();

    // fix sizes of children of old and new splitter
    currentSplitter->setSizes( currentSizes );
    QList<int> newSizes = newContainer->sizes();
    newSizes[0] = (newSizes[0] + newSizes[1] - newContainer->handleWidth()) / 2;
    newSizes[1] = newSizes[0];
    newContainer->setSizes( newSizes );
  }

  m_viewSpaceList.append( vsNew );
  activeViewSpace()->setActive( false );
  vsNew->setActive( true, true );
  vsNew->show();

  createView ((KTextEditor::Document*)activeView()->document());

  updateViewSpaceActions ();
}

void KateViewManager::removeViewSpace (KateViewSpace *viewspace)
{
  // abort if viewspace is 0
  if (!viewspace) return;

  // abort if this is the last viewspace
  if (m_viewSpaceList.count() < 2) return;

  // get current splitter
  QSplitter *currentSplitter = qobject_cast<QSplitter*>(viewspace->parentWidget());

  // no splitter found, bah
  if (!currentSplitter)
    return;

  // delete views of the viewspace
  while (viewspace->viewCount() > 0 && viewspace->currentView())
  {
    deleteView( viewspace->currentView(), false );
  }

  // cu viewspace
  m_viewSpaceList.removeAt( m_viewSpaceList.indexOf( viewspace ) );
  delete viewspace;

  // at this point, the splitter has exactly 1 child
  Q_ASSERT( currentSplitter->count() == 1 );

  // if we are not the root splitter, move the child one level up and delete
  // the splitter then.
  if (currentSplitter != this)
  {
    // get parent splitter
    QSplitter *parentSplitter = qobject_cast<QSplitter*>(currentSplitter->parentWidget());

    // only do magic if found ;)
    if (parentSplitter)
    {
      int index = parentSplitter->indexOf (currentSplitter);

      // save current splitter size, as the removal of currentSplitter looses the info
      QList<int> parentSizes = parentSplitter->sizes();
      parentSplitter->insertWidget (index, currentSplitter->widget (0));
      if (qVersion() == QLatin1String("4.6.2")) currentSplitter->hide(); // bug in Qt v4.6.2, prevents crash (bug:232140), line can be removed once we are sure that no one uses Qt 4.6.2 anymore.
      delete currentSplitter;
      // now restore the sizes again
      parentSplitter->setSizes(parentSizes);
    }
  }
  else if( QSplitter* splitter = qobject_cast<QSplitter*>(currentSplitter->widget(0)) )
  {
    // we are the root splitter and have only one child, which is also a splitter
    // -> eliminate the redundant splitter and move both children into the root splitter
    QList<int> sizes = splitter->sizes();
    // adapt splitter orientation to the splitter we are about to delete
    currentSplitter->setOrientation(splitter->orientation());
    currentSplitter->addWidget( splitter->widget(0) );
    currentSplitter->addWidget( splitter->widget(0) );
    delete splitter;
    currentSplitter->setSizes( sizes );
  }

  // find the view that is now active.
  KTextEditor::View* v = activeViewSpace()->currentView();
  if ( v )
    activateView( v );

  updateViewSpaceActions ();

  emit viewChanged();
}

void KateViewManager::slotCloseCurrentViewSpace()
{
  removeViewSpace(activeViewSpace());
}

void KateViewManager::slotCloseOtherViews()
{
  KateViewSpace* active = activeViewSpace();
  foreach (KateViewSpace  *v, m_viewSpaceList) {
    if (active != v) {
      removeViewSpace(v);
    }
  }
}


/**
 * session config functions
 */

void KateViewManager::saveViewConfiguration(KConfigGroup& config)
{
  // set Active ViewSpace to 0, just in case there is none active (would be
  // strange) and config somehow has previous value set
  config.writeEntry("Active ViewSpace", 0);

  m_splitterIndex = 0;
  saveSplitterConfig(this, config.config(), config.name());

  // Status Bar Items
  config.writeEntry("Cursor Position Visible", m_cursorPosToggle->isChecked());
  config.writeEntry("Characters Count Visible", m_charCountToggle->isChecked());
  config.writeEntry("Insertion Mode Visible", m_insertModeToggle->isChecked());
  config.writeEntry("Selection Mode Visible", m_selectModeToggle->isChecked());
  config.writeEntry("Encoding Visible", m_encodingToggle->isChecked());
  config.writeEntry("Document Name Visible", m_docNameToggle->isChecked());
}

void KateViewManager::restoreViewConfiguration (const KConfigGroup& config)
{
  // remove all views and viewspaces + remove their xml gui clients
  for (int i = 0; i < m_viewList.count(); ++i)
    mainWindow()->guiFactory ()->removeClient (m_viewList.at(i));

  qDeleteAll( m_viewList );
  m_viewList.clear();
  qDeleteAll( m_viewSpaceList );
  m_viewSpaceList.clear();
  m_activeStates.clear();

  // reset lru history, too!
  m_lruViews.clear();
  m_minAge = 0;

  // start recursion for the root splitter (Splitter 0)
  restoreSplitter( config.config(), config.name() + "-Splitter 0", this, config.name() );

  // finally, make the correct view from the last session active
  int lastViewSpace = config.readEntry("Active ViewSpace", 0);
  if( lastViewSpace > m_viewSpaceList.size() ) lastViewSpace = 0;
  if( lastViewSpace >= 0 && lastViewSpace < m_viewSpaceList.size())
  {
    setActiveSpace( m_viewSpaceList.at( lastViewSpace ) );
    // activate correct view (wish #195435, #188764)
    activateView( m_viewSpaceList.at( lastViewSpace )->currentView() );
    // give view the focus to avoid focus stealing by toolviews / plugins
    m_viewSpaceList.at( lastViewSpace )->currentView()->setFocus();
  }

  // emergency
  if (m_viewSpaceList.empty())
  {
    // kill bad children
    while (count())
      delete widget (0);

    KateViewSpace* vs = new KateViewSpace( this, 0 );
    addWidget (vs);
    vs->setActive( true );
    m_viewSpaceList.append(vs);
  }

  // Status Bar Items
  bool cursorPosVisible = config.readEntry("Cursor Position Visible", true);
  m_cursorPosToggle->setChecked(cursorPosVisible);
  emit cursorPositionItemVisibilityChanged(cursorPosVisible);

  bool charCountVisible = config.readEntry("Characters Count Visible", false);
  m_charCountToggle->setChecked(charCountVisible);
  emit charactersCountItemVisibilityChanged(charCountVisible);

  bool insertModeVisible = config.readEntry("Insertion Mode Visible", true);
  m_insertModeToggle->setChecked(insertModeVisible);
  emit insertModeItemVisibilityChanged(insertModeVisible);

  bool selectModeVisible = config.readEntry("Selection Mode Visible", true);
  m_selectModeToggle->setChecked(selectModeVisible);
  emit selectModeItemVisibilityChanged(selectModeVisible);

  bool encodingVisible = config.readEntry("Encoding Visible", true);
  m_encodingToggle->setChecked(encodingVisible);
  emit encodingItemVisibilityChanged(encodingVisible);

  bool docNameVisible = config.readEntry("Document Name Visible", true);
  m_docNameToggle->setChecked(docNameVisible);
  emit documentNameItemVisibilityChanged(docNameVisible);

  updateViewSpaceActions();
}

void KateViewManager::saveSplitterConfig( QSplitter* s, KConfigBase* configBase, const QString& viewConfGrp )
{
  QString grp = QString(viewConfGrp + "-Splitter %1").arg(m_splitterIndex);
  KConfigGroup config( configBase, grp );

  // Save sizes, orient, children for this splitter
  config.writeEntry( "Sizes", s->sizes() );
  config.writeEntry( "Orientation", int(s->orientation()) );

  QStringList childList;
  // a QSplitter has two children, either QSplitters and/or KateViewSpaces
  // special case: root splitter might have only one child (just for info)
  for (int it = 0; it < s->count(); ++it)
  {
    QWidget *obj = s->widget(it);
    KateViewSpace* kvs = qobject_cast<KateViewSpace*>(obj);

    QString n;  // name for child list, see below
    // For KateViewSpaces, ask them to save the file list.
    if ( kvs )
    {
      n = QString(viewConfGrp + "-ViewSpace %1").arg( m_viewSpaceList.indexOf(kvs) );
      kvs->saveConfig ( configBase, m_viewSpaceList.indexOf(kvs), viewConfGrp);
      // save active viewspace
      if ( kvs->isActiveSpace() )
      {
        KConfigGroup viewConfGroup(configBase, viewConfGrp);
        viewConfGroup.writeEntry("Active ViewSpace", m_viewSpaceList.indexOf(kvs) );
      }
    }
    // for QSplitters, recurse
    else if ( QSplitter* splitter = qobject_cast<QSplitter*>(obj) )
    {
      ++m_splitterIndex;
      n = QString(viewConfGrp + "-Splitter %1").arg( m_splitterIndex );
      saveSplitterConfig( splitter, configBase, viewConfGrp);
    }

    childList.append( n );
  }

  config.writeEntry("Children", childList);
}

void KateViewManager::restoreSplitter( const KConfigBase* configBase, const QString &group,
    QSplitter* parent, const QString& viewConfGrp)
{
  KConfigGroup config( configBase, group );

  parent->setOrientation((Qt::Orientation)config.readEntry("Orientation", int(Qt::Horizontal)));

  QStringList children = config.readEntry( "Children", QStringList() );
  for (QStringList::Iterator it = children.begin(); it != children.end(); ++it)
  {
    // for a viewspace, create it and open all documents therein.
    if ( (*it).startsWith(viewConfGrp + "-ViewSpace") )
    {
      KateViewSpace* vs = new KateViewSpace( this, 0 );
      m_viewSpaceList.append( vs );
      // make active so that the view created in restoreConfig has this
      // new view space as parent.
      setActiveSpace( vs );

      parent->addWidget( vs );
      vs->restoreConfig (this, configBase, *it);
      vs->show();
    }
    else
    {
      // for a splitter, recurse.
      restoreSplitter( configBase, *it, new QSplitter( parent ), viewConfGrp );
    }
  }

  // set sizes
  parent->setSizes( config.readEntry("Sizes", QList<int>()) );
  parent->show();
}

void KateViewManager::moveSplitter(Qt::Key key, int repeats)
{
  if (repeats < 1) return;

  KateViewSpace *vs = activeViewSpace();
  if (!vs) return;

  QSplitter *currentSplitter = qobject_cast<QSplitter*>(vs->parentWidget());

  if (!currentSplitter) return;
  if (currentSplitter->count() == 1) return;

  int move = 4 * repeats;
  // try to use font height in pixel to move splitter
  KTextEditor::HighlightInterface *hi = qobject_cast<KTextEditor::HighlightInterface*>(vs->currentView()->document());
  if (hi) {
    KTextEditor::Attribute::Ptr attrib(hi->defaultStyle(KTextEditor::HighlightInterface::dsNormal));
    QFontMetrics fm(attrib->font());
    move = fm.height() * repeats;
  }

  QWidget* currentWidget = (QWidget*)vs;
  bool foundSplitter = false;

  // find correct splitter to be moved
  while ( currentSplitter && currentSplitter->count() != 1 ) {

    if ( currentSplitter->orientation() == Qt::Horizontal
      && ( key == Qt::Key_Right || key == Qt::Key_Left ))
      foundSplitter = true;

    if ( currentSplitter->orientation() == Qt::Vertical
      && ( key == Qt::Key_Up || key == Qt::Key_Down ))
      foundSplitter = true;

    // if the views within the current splitter can be resized, resize them
    if (foundSplitter) {
      QList<int> currentSizes = currentSplitter->sizes();
      int index = currentSplitter->indexOf(currentWidget);

      if (( index == 0 && ( key == Qt::Key_Left || key == Qt::Key_Up ))
         ||( index == 1  && ( key == Qt::Key_Right || key == Qt::Key_Down )))
        currentSizes[index] -= move;

      if (( index == 0  && ( key == Qt::Key_Right || key == Qt::Key_Down ))
         ||( index == 1 && ( key == Qt::Key_Left || key == Qt::Key_Up )))
        currentSizes[index] += move;

      if ( index == 0 && ( key == Qt::Key_Right || key == Qt::Key_Down ))
        currentSizes[index+1] -= move;

      if ( index == 0 && ( key == Qt::Key_Left || key == Qt::Key_Up ))
        currentSizes[index+1] += move;

      if( index == 1 && ( key == Qt::Key_Right || key == Qt::Key_Down ))
        currentSizes[index-1] += move;

      if( index == 1 && ( key == Qt::Key_Left || key == Qt::Key_Up ))
        currentSizes[index-1] -= move;

      currentSplitter->setSizes(currentSizes);
      break;
    }

    currentWidget = (QWidget*)currentSplitter;
    // the parent of the current splitter will become the current splitter
    currentSplitter = qobject_cast<QSplitter*>(currentSplitter->parentWidget());
  }
}


// kate: space-indent on; indent-width 2; replace-tabs on;
