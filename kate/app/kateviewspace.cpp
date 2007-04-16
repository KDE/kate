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
#include "kateviewspace.moc"

#include "katemainwindow.h"
#include "kateviewspacecontainer.h"
#include "katedocmanager.h"
#include "kateapp.h"
#include "katesession.h"

#include <KTextEditor/SessionConfigInterface>

#include <KIconLoader>
#include <KLocale>
#include <KSqueezedTextLabel>
#include <KConfig>
#include <kdebug.h>
#include <KStringHandler>
#include <KXMLGUIFactory>
#include <KVBox>

#include <QStackedWidget>
#include <QPainter>
#include <QCursor>
#include <QMenu>
#include <QTimer>
#include <QPaintEvent>
#include <QEvent>
#include <QMouseEvent>

//BEGIN KateViewSpace
KateViewSpace::KateViewSpace( KateViewSpaceContainer *viewManager,
                              QWidget* parent, const char* name )
    : KVBox(parent),
    m_viewManager( viewManager )
{
  setObjectName(name);

  stack = new QStackedWidget( this );
  stack->setFocus();
  stack->setSizePolicy (QSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  mStatusBar = new KateVSStatusBar(this);
  mIsActiveSpace = false;

  setMinimumWidth (mStatusBar->minimumWidth());
  m_group.clear();

  // connect signal to hide/show statusbar
  connect (m_viewManager->mainWindow(), SIGNAL(statusBarToggled ()), this, SLOT(statusBarToggled ()));

  // init the statusbar...
  statusBarToggled ();
}

KateViewSpace::~KateViewSpace()
{}

void KateViewSpace::statusBarToggled ()
{
  // show or hide the bar?
  if (m_viewManager->mainWindow()->showStatusBar())
    mStatusBar->show ();
  else
    mStatusBar->hide ();
}

void KateViewSpace::addView(KTextEditor::View* v, bool show)
{
  // restore the config of this view if possible
  if ( !m_group.isEmpty() )
  {
    QString fn = v->document()->url().prettyUrl();
    if ( ! fn.isEmpty() )
    {
      QString vgroup = QString("%1 %2").arg(m_group).arg(fn);

      KateSession::Ptr as = KateSessionManager::self()->activeSession ();
      if ( as->configRead() && as->configRead()->hasGroup( vgroup ) )
      {
        KConfigGroup cg( as->configRead(), vgroup );

        if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(v))
          iface->readSessionConfig ( cg );
      }
    }
  }

  stack->addWidget(v);
  if (show)
  {
    mViewList.append(v);
    showView( v );
  }
  else
  {
    KTextEditor::View* c = (KTextEditor::View*)stack->currentWidget();
    mViewList.prepend( v );
    showView( c );
  }

  // signals for the statusbar
  connect(v, SIGNAL(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor&)), mStatusBar, SLOT(cursorPositionChanged(KTextEditor::View *)));
  connect(v, SIGNAL(viewModeChanged(KTextEditor::View *)), mStatusBar, SLOT(viewModeChanged(KTextEditor::View *)));
  connect(v, SIGNAL(selectionChanged (KTextEditor::View *)), mStatusBar, SLOT(selectionChanged (KTextEditor::View *)));
  connect(v, SIGNAL(informationMessage (KTextEditor::View *, const QString &)), mStatusBar, SLOT(informationMessage (KTextEditor::View *, const QString &)));
  connect(v->document(), SIGNAL(modifiedChanged(KTextEditor::Document *)), mStatusBar, SLOT(modifiedChanged()));
  connect(v->document(), SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)), mStatusBar, SLOT(modifiedChanged()) );
  connect(v->document(), SIGNAL(documentNameChanged(KTextEditor::Document *)), mStatusBar, SLOT(documentNameChanged()));
}

void KateViewSpace::removeView(KTextEditor::View* v)
{
  bool active = ( v == currentView() );

  mViewList.removeAt ( mViewList.indexOf ( v ) );
  stack->removeWidget (v);

  if ( ! active )
    return;

  // the last recently used viewspace is always at the end of the list
  if (!mViewList.isEmpty())
    showView(mViewList.last());
}

bool KateViewSpace::showView(KTextEditor::Document *document)
{
  QList<KTextEditor::View*>::const_iterator it = mViewList.constEnd();
  while( it != mViewList.constBegin() )
  {
    --it;
    if ((*it)->document() == document)
    {
      KTextEditor::View* kv = *it;

      // move viewspace to end of list
      mViewList.removeAt( mViewList.indexOf(kv) );
      mViewList.append( kv );
      stack->setCurrentWidget( kv );
      kv->show();

      mStatusBar->updateStatus ();

      return true;
    }
  }
  return false;
}


KTextEditor::View* KateViewSpace::currentView()
{
  // stack->currentWidget() returns NULL, if stack.count() == 0,
  // i.e. if mViewList.isEmpty()
  return (KTextEditor::View*)stack->currentWidget();
}

bool KateViewSpace::isActiveSpace()
{
  return mIsActiveSpace;
}

void KateViewSpace::setActive( bool active, bool )
{
  mIsActiveSpace = active;

  // change the statusbar palette and make sure it gets updated
  QPalette pal( palette() );
  if ( ! active )
  {
    pal.setColor( QPalette::Window, pal.color(QPalette::Window).dark() );
    pal.setColor( QPalette::Light, pal.color(QPalette::Light).light() );
  }

  mStatusBar->setPalette( pal );
  mStatusBar->update();
}

bool KateViewSpace::event( QEvent *e )
{
  if ( e->type() == QEvent::PaletteChange )
  {
    setActive( mIsActiveSpace );
    return true;
  }
  return KVBox::event( e );
}

void KateViewSpace::saveConfig ( KConfigBase* config, int myIndex , const QString& viewConfGrp)
{
//   kDebug()<<"KateViewSpace::saveConfig("<<myIndex<<", "<<viewConfGrp<<") - currentView: "<<currentView()<<")"<<endl;
  QString groupname = QString(viewConfGrp + "-ViewSpace %1").arg( myIndex );

  KConfigGroup group (config, groupname);
  group.writeEntry ("Count", mViewList.count());

  if (currentView())
    group.writeEntry( "Active View", currentView()->document()->url().prettyUrl() );

  // Save file list, including cursor position in this instance.
  int idx = 0;
  for (QList<KTextEditor::View*>::iterator it = mViewList.begin();
       it != mViewList.end(); ++it)
  {
    if ( !(*it)->document()->url().isEmpty() )
    {
      group.writeEntry( QString("View %1").arg( idx ), (*it)->document()->url().prettyUrl() );

      // view config, group: "ViewSpace <n> url"
      QString vgroup = QString("%1 %2").arg(groupname).arg((*it)->document()->url().prettyUrl());
      KConfigGroup viewGroup( config, vgroup );

      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(*it))
        iface->writeSessionConfig( viewGroup );
    }

    ++idx;
  }
}

void KateViewSpace::restoreConfig ( KateViewSpaceContainer *viewMan, KConfigBase* config, const QString &groupname )
{
  KConfigGroup group (config, groupname);
  QString fn = group.readEntry( "Active View" );

  if ( !fn.isEmpty() )
  {
    KTextEditor::Document *doc = KateDocManager::self()->findDocument (KUrl(fn));

    if (doc)
    {
      // view config, group: "ViewSpace <n> url"
      QString vgroup = QString("%1 %2").arg(groupname).arg(fn);
      KConfigGroup configGroup( config, vgroup );

      viewMan->createView (doc);

      KTextEditor::View *v = viewMan->activeView ();

      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(v))
        iface->readSessionConfig( configGroup );
    }
  }

  if (mViewList.isEmpty())
    viewMan->createView (KateDocManager::self()->document(0));

  m_group = groupname; // used for restroing view configs later
}
//END KateViewSpace

//BEGIN KateVSStatusBar
KateVSStatusBar::KateVSStatusBar ( KateViewSpace *parent)
    : KStatusBar( parent),
    m_viewSpace( parent )
{
  m_lineColLabel = new QLabel( this );
  addWidget( m_lineColLabel, 0 );
  m_lineColLabel->setAlignment( Qt::AlignCenter );
  m_lineColLabel->installEventFilter( this );

  m_modifiedLabel = new QLabel( QString("   "), this );
  addWidget( m_modifiedLabel, 0 );
  m_modifiedLabel->setAlignment( Qt::AlignCenter );
  m_modifiedLabel->installEventFilter( this );

  m_insertModeLabel = new QLabel( i18n(" INS "), this );
  addWidget( m_insertModeLabel, 0 );
  m_insertModeLabel->setAlignment( Qt::AlignCenter );
  m_insertModeLabel->installEventFilter( this );

  m_selectModeLabel = new QLabel( i18n(" NORM "), this );
  addWidget( m_selectModeLabel, 0 );
  m_selectModeLabel->setAlignment( Qt::AlignCenter );
  m_selectModeLabel->installEventFilter( this );

  m_fileNameLabel = new KSqueezedTextLabel( this );
  addPermanentWidget( m_fileNameLabel, 1 );
  m_fileNameLabel->setMinimumSize( 0, 0 );
  m_fileNameLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
  m_fileNameLabel->setAlignment( /*Qt::AlignRight*/Qt::AlignLeft );
  m_fileNameLabel->installEventFilter( this );

  installEventFilter( this );
  m_modPm = SmallIcon("modified");
  m_modDiscPm = SmallIcon("modonhd");
  m_modmodPm = SmallIcon("modmod");
  m_noPm = SmallIcon("null");
}

KateVSStatusBar::~KateVSStatusBar ()
{}

void KateVSStatusBar::showMenu()
{
  KXmlGuiWindow* mainWindow = static_cast<KXmlGuiWindow*>( window() );
  QMenu* menu = static_cast<QMenu*>( mainWindow->factory()->container("viewspace_popup", mainWindow ) );

  if (menu)
    menu->exec(QCursor::pos());
}

bool KateVSStatusBar::eventFilter(QObject*, QEvent *e)
{
  if (e->type() == QEvent::MouseButtonPress)
  {
    if ( m_viewSpace->currentView() )
      m_viewSpace->currentView()->setFocus();

    if ( ((QMouseEvent*)e)->button() == Qt::RightButton)
      showMenu();

    return true;
  }

  return false;
}

void KateVSStatusBar::updateStatus ()
{
  if (!m_viewSpace->currentView())
    return;

  KTextEditor::View* view = m_viewSpace->currentView();
  viewModeChanged (view);
  cursorPositionChanged (view);
  selectionChanged (view);
  modifiedChanged ();
  documentNameChanged ();
}

void KateVSStatusBar::viewModeChanged ( KTextEditor::View *view )
{
  if (view != m_viewSpace->currentView())
    return;

  m_insertModeLabel->setText( view->viewMode() );
}

void KateVSStatusBar::cursorPositionChanged ( KTextEditor::View *view )
{
  if (view != m_viewSpace->currentView())
    return;

  KTextEditor::Cursor position (view->cursorPositionVirtual());

  m_lineColLabel->setText(
    i18n(" Line: %1 Col: %2 ", KGlobal::locale()->formatNumber(position.line() + 1, 0),
         KGlobal::locale()->formatNumber(position.column() + 1, 0)) );
}

void KateVSStatusBar::selectionChanged (KTextEditor::View *view)
{
  if (view != m_viewSpace->currentView())
    return;

  m_selectModeLabel->setText( view->blockSelection() ? i18n(" BLK ") : i18n(" NORM ") );
}

void KateVSStatusBar::informationMessage (KTextEditor::View *view, const QString &message)
{
  if (view != m_viewSpace->currentView())
    return;

  m_fileNameLabel->setText( message );

  // timer to reset this after 4 seconds
  QTimer::singleShot(4000, this, SLOT(documentNameChanged()));
}

void KateVSStatusBar::modifiedChanged()
{
  KTextEditor::View *v = m_viewSpace->currentView();

  if ( v )
  {
    bool mod = v->document()->isModified();

    const KateDocumentInfo *info
    = KateDocManager::self()->documentInfo ( v->document() );

    bool modOnHD = info && info->modifiedOnDisc;

    m_modifiedLabel->setPixmap(
      mod ?
      info && modOnHD ?
      m_modmodPm :
  m_modPm :
      info && modOnHD ?
      m_modDiscPm :
      m_noPm
    );
  }
}

void KateVSStatusBar::documentNameChanged ()
{
  KTextEditor::View *v = m_viewSpace->currentView();

  if ( v )
    m_fileNameLabel->setText( KStringHandler::lsqueeze(v->document()->documentName (), 64) );
}

//END KateVSStatusBar

// kate: space-indent on; indent-width 2; replace-tabs on;
