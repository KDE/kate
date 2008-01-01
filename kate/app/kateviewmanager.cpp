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

#include <KAction>
#include <KActionCollection>
#include <KCmdLineArgs>
#include <kdebug.h>
#include <KDirOperator>
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
    , m_pendingViewCreation(false)
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

  connect(KateDocManager::self(), SIGNAL(documentCreated(KTextEditor::Document *)), this, SLOT(documentCreated(KTextEditor::Document *)));
  connect(KateDocManager::self(), SIGNAL(documentDeleted(KTextEditor::Document *)), this, SLOT(documentDeleted(KTextEditor::Document *)));

  // init done
  m_init = false;
}

KateViewManager::~KateViewManager ()
{
}

void KateViewManager::activateDocument(const QModelIndex &index)
{
  QVariant v = index.data(KateDocManager::DocumentRole);
  if (!v.isValid()) return;
  activateView(v.value<KTextEditor::Document*>());
}

void KateViewManager::setupActions ()
{
  QAction *a;

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

  m_closeView->setWhatsThis(i18n("Close the currently active splitted view"));

  goNext = m_mainWindow->actionCollection()->addAction( "go_next" );
  goNext->setText( i18n("Next View") );
  goNext->setShortcut( Qt::Key_F8 );
  connect(goNext, SIGNAL(triggered()), this, SLOT(activateNextView()));

  goNext->setWhatsThis(i18n("Make the next split view the active one."));

  goPrev = m_mainWindow->actionCollection()->addAction( "go_prev" );
  goPrev->setText( i18n("Previous View") );
  goPrev->setShortcut( Qt::SHIFT + Qt::Key_F8 );
  connect(goPrev, SIGNAL(triggered()), this, SLOT(activatePrevView()));

  goPrev->setWhatsThis(i18n("Make the previous split view the active one."));
}

void KateViewManager::updateViewSpaceActions ()
{
  m_closeView->setEnabled (viewSpaceCount() > 1);
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
                                      cv->document()->encoding(),
                                      cv->document()->url().url(),
                                      QString(), m_mainWindow, i18n("Open File"));

    KTextEditor::Document *lastID = 0;
    for (KUrl::List::Iterator i = r.URLs.begin(); i != r.URLs.end(); ++i)
      lastID = openUrl( *i, r.encoding, false );

    if (lastID)
      activateView (lastID);
  }
}

void KateViewManager::slotDocumentClose ()
{
  // no active view, do nothing
  if (!activeView()) return;

  // prevent close document if only one view alive and the document of
  // it is not modified and empty !!!
  if ( (KateDocManager::self()->documents() == 1)
       && !activeView()->document()->isModified()
       && activeView()->document()->url().isEmpty()
       && activeView()->document()->documentEnd() == KTextEditor::Cursor::start() )
  {
    activeView()->document()->closeUrl();
    return;
  }

  // close document
  KateDocManager::self()->closeDocument ((KTextEditor::Document*)activeView()->document());
}

KTextEditor::Document *KateViewManager::openUrl (const KUrl &url, const QString& encoding, bool activate, bool isTempFile)
{
  KTextEditor::Document *doc = KateDocManager::self()->openUrl (url, encoding, isTempFile);

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

// VIEWSPACE

void KateViewManager::documentCreated (KTextEditor::Document *doc)
{
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

  // popup menu
  QMenu *menu = qobject_cast<QMenu*> (mainWindow()->factory()->container("ktexteditor_popup", mainWindow()));
  if (menu)
    view->setContextMenu (menu);

  connect(view, SIGNAL(dropEventPass(QDropEvent *)), mainWindow(), SLOT(slotDropEvent(QDropEvent *)));
  connect(view, SIGNAL(focusIn(KTextEditor::View *)), this, SLOT(activateSpace(KTextEditor::View *)));

  activeViewSpace()->addView( view );
  activateView( view );

  return true;
}

bool KateViewManager::deleteView (KTextEditor::View *view, bool delViewSpace)
{
  if (!view) return true;

  KateViewSpace *viewspace = (KateViewSpace *)view->parentWidget()->parentWidget();

  viewspace->removeView (view);

  mainWindow()->guiFactory ()->removeClient (view);

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
  for ( QList<KateViewSpace*>::const_iterator it = m_viewSpaceList.begin();
        it != m_viewSpaceList.end(); ++it )
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

  for ( QList<KTextEditor::View*>::const_iterator it = m_viewList.begin();
        it != m_viewList.end(); ++it )
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

  KateViewSpace* vs = (KateViewSpace*)v->parentWidget()->parentWidget();

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
  else if (m_pendingViewCreation)
  {
    m_pendingViewCreation = false;
    disconnect(m_pendingDocument, SIGNAL(documentNameChanged(Document *)), this, SLOT(slotPendingDocumentNameChanged()));
    createView(m_pendingDocument);
  }
}

void KateViewManager::activateView ( KTextEditor::View *view )
{
  if (!view) return;

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
    mainWindow()->toolBar()->hide();

    if (guiMergedView)
      mainWindow()->guiFactory()->removeClient( guiMergedView );

    guiMergedView = view;

    if (!m_blockViewCreationAndActivation)
      mainWindow()->guiFactory()->addClient( view );

    mainWindow()->toolBar()->show();
    mainWindow()->setUpdatesEnabled( true );

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

  for ( QList<KTextEditor::View*>::const_iterator it = m_viewList.begin();
        it != m_viewList.end(); ++it)
  {
    if ( (*it)->document() == doc )
      closeList.append( *it );
  }

  while ( !closeList.isEmpty() )
    deleteView( closeList.takeFirst(), true );

  if (m_blockViewCreationAndActivation) return;
  QTimer::singleShot(0, this, SIGNAL(viewChanged()));
}

void KateViewManager::slotPendingDocumentNameChanged()
{
  QString c = m_pendingDocument->documentName();
  setWindowTitle(KStringHandler::lsqueeze(c, 32));
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

  // start recursion for the root splitter (Splitter 0)
  restoreSplitter( config.config(), config.name() + "-Splitter 0", this, config.name() );

  // finally, make the correct view from the last session active
  int lastViewSpace = config.readEntry("Active ViewSpace", 0);
  if( lastViewSpace > m_viewSpaceList.size() ) lastViewSpace = 0;
  if( lastViewSpace >= 0 && lastViewSpace < m_viewSpaceList.size())
  {
    setActiveSpace( m_viewSpaceList.at( lastViewSpace ) );
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

      vs->restoreConfig (this, configBase, *it);
      parent->addWidget( vs );
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


// kate: space-indent on; indent-width 2; replace-tabs on;
