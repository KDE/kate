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
#include "kateviewspacecontainer.h"
#include "katetabwidget.h"

#include <kate/mainwindow.h>

#include <KAction>
#include <KActionCollection>
#include <KCmdLineArgs>
#include <kdebug.h>
#include <KDirOperator>
#include <KEncodingFileDialog>
#include <KIconLoader>
#include <KGlobal>
#include <KLocale>
#include <KToolBar>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KConfig>
#include <kstandardaction.h>
#include <KStandardDirs>
#include <KGlobalSettings>
#include <kstandardshortcut.h>

#include <QApplication>
#include <QObject>
#include <QStringList>
#include <QFileInfo>
#include <QToolButton>
#include <QToolTip>
//END Includes

KateViewManager::KateViewManager (KateMainWindow *parent)
    : QObject  (parent)
    , m_currentContainer (0)
    , m_mainWindow(parent)
{
  // while init
  m_init = true;

  // some stuff for the tabwidget
  m_mainWindow->tabWidget()->setTabReorderingEnabled( true );

  // important, set them up, as we use them in other methodes
  setupActions ();

  guiMergedView = 0;

  connect(m_mainWindow->tabWidget(), SIGNAL(currentChanged(QWidget*)), this, SLOT(tabChanged(QWidget*)));
  slotNewTab();
  tabChanged(m_mainWindow->tabWidget()->currentWidget());

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
   * tabbing
   */
  a = m_mainWindow->actionCollection()->addAction( "view_new_tab" );
  a->setIcon( KIcon("tab-new") );
  a->setText( i18n("New Tab") );
  connect(a, SIGNAL(triggered()), this, SLOT(slotNewTab()));

  m_closeTab = m_mainWindow->actionCollection()->addAction("view_close_tab");
  m_closeTab->setIcon( KIcon("tab-remove") );
  m_closeTab->setText( i18n("Close Current Tab") );
  connect(m_closeTab, SIGNAL(triggered()), this, SLOT(slotCloseTab()));

  m_activateNextTab = m_mainWindow->actionCollection()->addAction("view_next_tab");
  m_activateNextTab->setText( i18n( "Activate Next Tab" ) );
  m_activateNextTab->setShortcuts( QApplication::isRightToLeft() ? KStandardShortcut::tabPrev() : KStandardShortcut::tabNext() );
  connect(m_activateNextTab, SIGNAL(triggered()), this, SLOT(activateNextTab()));

  m_activatePrevTab = m_mainWindow->actionCollection()->addAction("view_prev_tab");
  m_activatePrevTab->setText( i18n( "Activate Previous Tab" ) );
  m_activatePrevTab->setShortcuts( QApplication::isRightToLeft() ? KStandardShortcut::tabNext() : KStandardShortcut::tabPrev() );
  connect(m_activatePrevTab, SIGNAL(triggered()), this, SLOT(activatePrevTab()));

  /**
   * view splitting
   */
  a = m_mainWindow->actionCollection()->addAction("view_split_vert");
  a->setIcon( KIcon("view-left-right") );
  a->setText( i18n("Split Ve&rtical") );
  a->setShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_L );
  connect(a, SIGNAL(triggered()), this, SLOT(slotSplitViewSpaceVert()));

  a->setWhatsThis(i18n("Split the currently active view vertically into two views."));

  a = m_mainWindow->actionCollection()->addAction("view_split_horiz");
  a->setIcon( KIcon("view-top-bottom") );
  a->setText( i18n("Split &Horizontal") );
  a->setShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_T );
  connect(a, SIGNAL(triggered()), this, SLOT(slotSplitViewSpaceHoriz()));

  a->setWhatsThis(i18n("Split the currently active view horizontally into two views."));

  m_closeView = m_mainWindow->actionCollection()->addAction("view_close_current_space");
  m_closeView->setIcon( KIcon("view_remove") );
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

  /**
   * buttons for tabbing
   */
  QToolButton *b = new QToolButton( m_mainWindow->tabWidget() );
  connect( b, SIGNAL( clicked() ),
           this, SLOT( slotNewTab() ) );
  b->setIcon( SmallIcon( "tab-new" ) );
  b->adjustSize();
  b->setToolTip( i18n("Open a new tab"));
  m_mainWindow->tabWidget()->setCornerWidget( b, Qt::TopLeftCorner );

  b = m_closeTabButton = new QToolButton( m_mainWindow->tabWidget() );
  connect( b, SIGNAL( clicked() ),
           this, SLOT( slotCloseTab() ) );
  b->setIcon( SmallIcon( "tab-remove" ) );
  b->adjustSize();
  b->setToolTip( i18n("Close the current tab"));
  m_mainWindow->tabWidget()->setCornerWidget( b, Qt::TopRightCorner );
}

void KateViewManager::updateViewSpaceActions ()
{
  m_closeView->setEnabled (m_currentContainer->viewSpaceCount() > 1);
  goNext->setEnabled (m_currentContainer->viewSpaceCount() > 1);
  goPrev->setEnabled (m_currentContainer->viewSpaceCount() > 1);
}

void KateViewManager::tabChanged(QWidget* widget)
{
  // get container
  KateViewSpaceContainer *container = qobject_cast<KateViewSpaceContainer*>(widget);

  // should be always valid
  Q_ASSERT(container);

  // switch current container
  m_currentContainer = container;
  m_currentContainer->reactivateActiveView();
  m_closeTab->setEnabled(m_mainWindow->tabWidget()->count() > 1);
  m_activateNextTab->setEnabled(m_mainWindow->tabWidget()->count() > 1);
  m_activatePrevTab->setEnabled(m_mainWindow->tabWidget()->count() > 1);
  m_closeTabButton->setEnabled (m_mainWindow->tabWidget()->count() > 1);

  updateViewSpaceActions ();
}

void KateViewManager::slotNewTab()
{
  KTextEditor::Document *doc = 0;

  if (m_currentContainer)
  {
    if (m_currentContainer->activeView())
      doc = m_currentContainer->activeView()->document();
  }

  KateViewSpaceContainer *container = new KateViewSpaceContainer(this);
  m_viewSpaceContainerList.append(container);
  m_mainWindow->tabWidget()->addTab (container, "");

  connect(container, SIGNAL(viewChanged()), this, SIGNAL(viewChanged()));
  connect(container, SIGNAL(viewChanged()), m_mainWindow->mainWindow(), SIGNAL(viewChanged()));

  // update title...
  connect(container, SIGNAL(changeMyTitle (QWidget *)), this, SLOT(changeMyTitle (QWidget *)));

  if (!m_init)
  {
    container->activateView(doc);
    m_mainWindow->slotWindowActivated ();
  }
}

void KateViewManager::slotCloseTab()
{
  // never close last container!
  if (m_viewSpaceContainerList.count() <= 1) return;

  int pos = m_viewSpaceContainerList.indexOf (m_currentContainer);

  if (pos == -1)
    return;

  if (guiMergedView)
    m_mainWindow->guiFactory()->removeClient (guiMergedView );

  delete m_viewSpaceContainerList.takeAt (pos);

  if (pos >= m_viewSpaceContainerList.count())
    pos = m_viewSpaceContainerList.count() - 1;

  tabChanged(m_viewSpaceContainerList[pos]);
}

void KateViewManager::changeMyTitle (QWidget *page)
{
  // get container
  KateViewSpaceContainer *container = qobject_cast<KateViewSpaceContainer*>(page);

  // should be always valid
  Q_ASSERT(container);

  // update title if possible
  if (container->activeView())
  {
    m_mainWindow->tabWidget()->setTabText (m_mainWindow->tabWidget()->indexOf (page)
                                           , container->activeView()->document()->documentName());
  }
}

void KateViewManager::activateNextTab()
{
  if( m_mainWindow->tabWidget()->count() <= 1 ) return;

  int iTab = m_mainWindow->tabWidget()->currentIndex();

  iTab++;

  if( iTab == m_mainWindow->tabWidget()->count() )
    iTab = 0;

  m_mainWindow->tabWidget()->setCurrentIndex( iTab );
}

void KateViewManager::activatePrevTab()
{
  if( m_mainWindow->tabWidget()->count() <= 1 ) return;

  int iTab = m_mainWindow->tabWidget()->currentIndex();

  iTab--;

  if( iTab == -1 )
    iTab = m_mainWindow->tabWidget()->count() - 1;

  m_mainWindow->tabWidget()->setCurrentIndex( iTab );
}

void KateViewManager::activateSpace (KTextEditor::View* v)
{
  m_currentContainer->activateSpace(v);
}

void KateViewManager::activateView ( KTextEditor::View *view )
{
  m_currentContainer->activateView(view);
}

KateViewSpace* KateViewManager::activeViewSpace ()
{
  return m_currentContainer->activeViewSpace();
}

KTextEditor::View* KateViewManager::activeView ()
{
  return m_currentContainer->activeView();
}

void KateViewManager::setActiveSpace ( KateViewSpace* vs )
{
  m_currentContainer->setActiveSpace(vs);
}

void KateViewManager::setActiveView ( KTextEditor::View* view )
{
  m_currentContainer->setActiveView(view);
}

KTextEditor::View *KateViewManager::activateView( KTextEditor::Document *doc )
{
  // activate view
  m_currentContainer->activateView(doc);

  // return the current activeView
  return activeView ();
}

uint KateViewManager::viewCount ()
{
  uint viewCount = 0;
  for (int i = 0;i < m_viewSpaceContainerList.count();i++)
  {
    viewCount += m_viewSpaceContainerList[i]->viewCount();
  }
  return viewCount;

}

uint KateViewManager::viewSpaceCount ()
{
  uint viewSpaceCount = 0;
  for (int i = 0;i < m_viewSpaceContainerList.count();i++)
  {
    viewSpaceCount += m_viewSpaceContainerList[i]->viewSpaceCount();
  }
  return viewSpaceCount;
}

void KateViewManager::setViewActivationBlocked (bool block)
{
  for (int i = 0;i < m_viewSpaceContainerList.count();i++)
    m_viewSpaceContainerList[i]->m_blockViewCreationAndActivation = block;
}

void KateViewManager::activateNextView()
{
  m_currentContainer->activateNextView();
}

void KateViewManager::activatePrevView()
{
  m_currentContainer->activatePrevView();
}

void KateViewManager::closeViews(KTextEditor::Document *doc)
{
  for (int i = 0;i < m_viewSpaceContainerList.count();i++)
  {
    m_viewSpaceContainerList[i]->closeViews(doc);
  }
  tabChanged(m_currentContainer);
}

void KateViewManager::slotDocumentNew ()
{
  m_currentContainer->createView ();
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

void KateViewManager::removeViewSpace (KateViewSpace *viewspace)
{
  m_currentContainer->removeViewSpace(viewspace);
}

void KateViewManager::slotCloseCurrentViewSpace()
{
  m_currentContainer->slotCloseCurrentViewSpace();
}

void KateViewManager::slotSplitViewSpaceVert()
{
  m_currentContainer->slotSplitViewSpaceVert();
}

void KateViewManager::slotSplitViewSpaceHoriz()
{
  m_currentContainer->slotSplitViewSpaceHoriz();
}

/**
 * session config functions
 */

void KateViewManager::saveViewConfiguration(KConfigGroup& config)
{
  config.writeEntry("ViewSpaceContainers", m_viewSpaceContainerList.count());
  config.writeEntry("Active ViewSpaceContainer", m_mainWindow->tabWidget()->currentIndex());
  for (int i = 0; i < m_viewSpaceContainerList.count(); i++)
  {
    KConfigGroup cg = config;
    cg.changeGroup( cg.group() + QString(":ViewSpaceContainer-%1:").arg(i) );
    m_viewSpaceContainerList[i]->saveViewConfiguration(cg);
  }
}

void KateViewManager::restoreViewConfiguration (const KConfigGroup &config)
{
  uint tabCount = config.readEntry("ViewSpaceContainers", 0);
  int activeOne = config.readEntry("Active ViewSpaceContainer", 0);
  if (tabCount == 0) return;
  KConfigGroup cg = config;
  cg.changeGroup( cg.group() + QString(":ViewSpaceContainer-0:") );
  m_viewSpaceContainerList[0]->restoreViewConfiguration(cg);
  for (uint i = 1;i < tabCount;i++)
  {
    slotNewTab();
    cg.changeGroup( cg.group() + QString(":ViewSpaceContainer-%1:").arg(i) );
    m_viewSpaceContainerList[i]->restoreViewConfiguration(cg);
  }

  if (activeOne != m_mainWindow->tabWidget()->currentIndex())
    m_mainWindow->tabWidget()->setCurrentIndex (activeOne);

  updateViewSpaceActions();
}

KateMainWindow *KateViewManager::mainWindow()
{
  return m_mainWindow;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
