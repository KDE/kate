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

//BEGIN KateViewSpace
KateViewSpace::KateViewSpace( KateViewManager *viewManager,
                              QWidget* parent, const char* name )
    : QWidget(parent)
    , m_viewManager(viewManager)
{
  setObjectName(QString::fromLatin1(name));
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  //BEGIN tab bar
  QHBoxLayout * hLayout = new QHBoxLayout();

  // add tab bar
  m_tabBar = new KateTabBar(this);
  connect(m_tabBar, &KateTabBar::currentChanged, this, &KateViewSpace::changeView);
  hLayout->addWidget(m_tabBar);

  // add vertical split view space
  QToolButton * split = new QToolButton(this);
  split->setAutoRaise(true);
  split->setDefaultAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_split_vert")));
  split->setWhatsThis(i18n("Split this view horizontally into two views."));
  split->installEventFilter(this); // on click, active this view space
  hLayout->addWidget(split);

  // add horizontally split view space
  split = new QToolButton(this);
  split->setAutoRaise(true);
  split->setDefaultAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_split_horiz")));
  split->setWhatsThis(i18n("Split this view vertically into two views."));
  split->installEventFilter(this); // on click, active this view space
  hLayout->addWidget(split);

  // add quick open
  QToolButton * quickOpen = new QToolButton(this);
  quickOpen->setAutoRaise(true);
  quickOpen->setDefaultAction(m_viewManager->mainWindow()->actionCollection()->action(QStringLiteral("view_quick_open")));
  quickOpen->installEventFilter(this); // on click, active this view space
  hLayout->addWidget(quickOpen);

  layout->addLayout(hLayout);
  //END tab bar

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

bool KateViewSpace::eventFilter(QObject *obj, QEvent *event)
{
  if (! isActiveSpace() && qobject_cast<QToolButton*>(obj) && event->type() == QEvent::MouseButtonPress) {
    m_viewManager->setActiveSpace(this);
    m_viewManager->activateView(currentView()->document());
  }
  return false;
}

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

  // just register document, it is shown below through showView() then
  if ( ! m_docToTabId.contains(doc)) {
    registerDocument(doc);
    Q_ASSERT(m_docToTabId.contains(doc));
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
  Q_ASSERT(m_docToTabId.contains(v->document()));
  documentDestroyed(v->document());

  // remove from view space
  bool active = ( v == currentView() );

  mViewList.removeAt ( mViewList.indexOf ( v ) );
  stack->removeWidget (v);

  if ( ! active )
    return;

  // the last recently used viewspace is always at the end of the list
  if (!mViewList.isEmpty()) {
    showView(mViewList.last());
  }
  // if we still have tabs, use these
  else if (! m_docToTabId.isEmpty()) {
    QList<KTextEditor::Document*> keys = m_docToTabId.keys();
    m_viewManager->createView(keys.first());
  }
}

KTextEditor::View * KateViewSpace::viewForDocument(KTextEditor::Document * doc) const
{
  QList<KTextEditor::View*>::const_iterator it = mViewList.constEnd();
  while (it != mViewList.constBegin()) {
    --it;
    if ((*it)->document() == doc) {
      return *it;
    }
  }
  return 0;
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
      Q_ASSERT(m_docToTabId.contains(document));
//       m_tabBar->raiseTab(m_docToTabId[document]);
      m_tabBar->setCurrentTab(m_docToTabId[document]);

      return true;
    }
  }
  return false;
}

void KateViewSpace::changeView(int buttonId)
{
  KTextEditor::Document * doc = m_docToTabId.key(buttonId);
  Q_ASSERT(doc);

  // make sure we open the view in this view space
  if (! isActiveSpace()) {
    m_viewManager->setActiveSpace(this);
  }

  // tell the view manager to show the view
  m_viewManager->activateView(doc);
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

void KateViewSpace::registerDocument(KTextEditor::Document *doc)
{
  Q_ASSERT( ! m_docToTabId.contains(doc));
  // add to tab bar
  const int index = m_tabBar->addTab(doc->url().toString(), doc->documentName());
  m_docToTabId[doc] = index;

  connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document*)),
          this, SLOT(updateDocumentName(KTextEditor::Document*)));
  connect(doc, SIGNAL(destroyed(QObject*)), this, SLOT(documentDestroyed(QObject*)));
}

void KateViewSpace::documentDestroyed(QObject * doc)
{
  Q_ASSERT(m_docToTabId.contains(static_cast<KTextEditor::Document*>(doc)));
  const int index = m_docToTabId[static_cast<KTextEditor::Document*>(doc)];
  m_tabBar->removeTab(index);
  m_docToTabId.remove(static_cast<KTextEditor::Document*>(doc));
  disconnect(doc, 0, this, 0);
}

void KateViewSpace::updateDocumentName(KTextEditor::Document* doc)
{
  const int buttonId = m_docToTabId[doc];
  Q_ASSERT(buttonId >= 0);
  m_tabBar->setTabText(buttonId, doc->documentName());
  m_tabBar->setTabURL(buttonId, doc->url().toDisplayString());
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
