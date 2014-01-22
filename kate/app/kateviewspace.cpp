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

#include <KSqueezedTextLabel>
#include <KStringHandler>
#include <KLocalizedString>
#include <kiconutils.h>
#include <KConfigGroup>
#include <KXMLGUIFactory>

#include <QTimer>
#include <QCursor>
#include <QMouseEvent>
#include <QMenu>
#include <QSizeGrip>
#include <QStackedWidget>

//BEGIN KateViewSpace
KateViewSpace::KateViewSpace( KateViewManager *viewManager,
                              QWidget* parent, const char* name )
    : QFrame(parent),
    m_viewManager( viewManager )
{
  setObjectName(QString::fromLatin1(name));
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  m_tabBar = new KateTabBar(this);
  layout->addWidget(m_tabBar);
  connect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);

  stack = new QStackedWidget( this );
  stack->setFocus();
  stack->setSizePolicy (QSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  layout->addWidget(stack);

  mIsActiveSpace = false;

  m_group.clear();

  // connect signal to hide/show statusbar
  connect (m_viewManager->mainWindow(), SIGNAL(statusBarToggled()), this, SLOT(statusBarToggled()));

  // init the statusbar...
  statusBarToggled ();
}

KateViewSpace::~KateViewSpace()
{}

void KateViewSpace::statusBarToggled ()
{
  Q_FOREACH(auto view, mViewList) {
    view->setStatusBarEnabled(m_viewManager->mainWindow()->showStatusBar());
  }
}

KTextEditor::View *KateViewSpace::createView (KTextEditor::Document *doc)
{
  /**
   * Create a fresh view
   */
  KTextEditor::View *v = doc->createView (stack, m_viewManager->mainWindow()->wrapper());

  // set status bar to right state
  v->setStatusBarEnabled(m_viewManager->mainWindow()->showStatusBar());

  // restore the config of this view if possible
  if ( !m_group.isEmpty() )
  {
    QString fn = v->document()->url().toString();
    if ( ! fn.isEmpty() )
    {
      QString vgroup = QString::fromLatin1("%1 %2").arg(m_group).arg(fn);

      KateSession::Ptr as = KateSessionManager::self()->activeSession ();
      if ( as->config() && as->config()->hasGroup( vgroup ) )
      {
        KConfigGroup cg( as->config(), vgroup );

        if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(v))
          iface->readSessionConfig ( cg );
      }
    }
  }

  // check whether the document is in lazy list
  if (m_docToTabId.contains(doc)) {
    // reuse tab id by moving it from doc mapper to view mapper
    m_viewToTabId[v] = m_docToTabId[doc];
    // no need to track it here anymore
    m_docToTabId.remove(doc);
    disconnect(doc, SIGNAL(destroyed(QObject*)), this, SLOT(documentDestroyed(QObject*)));
  } else {
    // create new tab bar button
    const int index = m_tabBar->addTab(doc->url().toString(), doc->documentName());
    Q_ASSERT(index >= 0);
    m_viewToTabId[v] = index;
  }

  // insert View into stack
  stack->addWidget(v);
  mViewList.append(v);
  showView( v );

  return v;
}

void KateViewSpace::removeView(KTextEditor::View* v)
{
  // remove from tab bar
  Q_ASSERT(m_viewToTabId.contains(v));
  m_tabBar->removeTab(m_viewToTabId[v]);
  m_viewToTabId.remove(v);

  // remove from view space
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

      // move view to end of list
      mViewList.removeAt( mViewList.indexOf(kv) );
      mViewList.append( kv );
      stack->setCurrentWidget( kv );
      kv->show();

      // raise tab in tab bar
      Q_ASSERT(m_viewToTabId.contains(kv));
//       m_tabBar->raiseTab(m_viewToTabId[kv]);
      m_tabBar->setCurrentTab(m_viewToTabId[kv]);

      return true;
    }
  }
  return false;
}

void KateViewSpace::changeView(int buttonId)
{
  // lazy button?
  KTextEditor::Document * doc = m_docToTabId.key(buttonId);
  if (doc) {
    // make sure this view space is active, so that the view is created in this view
    m_viewManager->setActiveSpace(this);
    m_viewManager->createView(doc);
  } else {
    KTextEditor::View * view = m_viewToTabId.key(buttonId);
    Q_ASSERT(view);

    if (view != currentView()) {
      showView(view);
    }
  }
  Q_ASSERT(! m_docToTabId.contains(doc));
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

  // change the statusbar palette according to the activation state
  // FIXME KF5 mStatusBar->setEnabled(active);
}

void KateViewSpace::registerDocumentWhileActive(KTextEditor::Document *doc)
{
  Q_ASSERT( ! m_docToTabId.contains(doc));
  // add lazy to tab bar
  const int index = m_tabBar->addTab(doc->url().toString(), doc->documentName());
  m_docToTabId[doc] = index;

  connect(doc, SIGNAL(destroyed(QObject*)), this, SLOT(documentDestroyed(QObject*)));
}

void KateViewSpace::documentDestroyed(QObject * doc)
{
  Q_ASSERT(m_docToTabId.contains(static_cast<KTextEditor::Document*>(doc)));
  const int index = m_docToTabId[static_cast<KTextEditor::Document*>(doc)];
  m_tabBar->removeTab(index);
  m_docToTabId.remove(static_cast<KTextEditor::Document*>(doc));
}

void KateViewSpace::saveConfig ( KConfigBase* config, int myIndex , const QString& viewConfGrp)
{
//   qCDebug(LOG_KATE)<<"KateViewSpace::saveConfig("<<myIndex<<", "<<viewConfGrp<<") - currentView: "<<currentView()<<")";
  QString groupname = QString(viewConfGrp + QStringLiteral("-ViewSpace %1")).arg( myIndex );

  KConfigGroup group (config, groupname);
  group.writeEntry ("Count", mViewList.count());

  if (currentView())
    group.writeEntry( "Active View", currentView()->document()->url().toString() );

  // Save file list, including cursor position in this instance.
  int idx = 0;
  for (QList<KTextEditor::View*>::iterator it = mViewList.begin();
       it != mViewList.end(); ++it)
  {
    if ( !(*it)->document()->url().isEmpty() )
    {
      group.writeEntry( QString::fromLatin1("View %1").arg( idx ), (*it)->document()->url().toString() );

      // view config, group: "ViewSpace <n> url"
      QString vgroup = QString::fromLatin1("%1 %2").arg(groupname).arg((*it)->document()->url().toString());
      KConfigGroup viewGroup( config, vgroup );

      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(*it))
        iface->writeSessionConfig( viewGroup );
    }

    ++idx;
  }
}

void KateViewSpace::restoreConfig ( KateViewManager *viewMan, const KConfigBase* config, const QString &groupname )
{
  KConfigGroup group (config, groupname);
  QString fn = group.readEntry( "Active View" );

  if ( !fn.isEmpty() )
  {
    KTextEditor::Document *doc = KateDocManager::self()->findDocument (QUrl(fn));

    if (doc)
    {
      // view config, group: "ViewSpace <n> url"
      QString vgroup = QString::fromLatin1("%1 %2").arg(groupname).arg(fn);
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
