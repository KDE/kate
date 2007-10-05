/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Anders Lund <anders@alweb.dk>

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

#include "kateconsole.h"
#include "kateconsole.moc"

#include <kicon.h>
#include <kiconloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kde_terminal_interface.h>
#include <kshell.h>
#include <kparts/part.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <kurl.h>
#include <klibloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
//Added by qt3to4:
#include <QShowEvent>

#include <QApplication>

#include <kgenericfactory.h>
#include <kpluginfactory.h>
#include <kauthorized.h>

K_EXPORT_COMPONENT_FACTORY( katekonsoleplugin, KGenericFactory<KateKonsolePlugin>( "katekonsoleplugin" ) )

KateKonsolePlugin::KateKonsolePlugin( QObject* parent, const QStringList& ):
    Kate::Plugin ( (Kate::Application*)parent )
{
  if (!KAuthorized::authorizeKAction("shell_access"))
  {
    KMessageBox::sorry(0, i18n ("You do not have enough karma to access a shell or terminal emulation"));
  }
}

Kate::PluginView *KateKonsolePlugin::createView (Kate::MainWindow *mainWindow)
{
  return new KateKonsolePluginView (mainWindow);
}

KateKonsolePluginView::KateKonsolePluginView (Kate::MainWindow *mainWindow)
    : Kate::PluginView (mainWindow)
{
  // init console
  QWidget *toolview = mainWindow->createToolView ("kate_private_plugin_katekonsoleplugin", Kate::MainWindow::Bottom, SmallIcon("konsole"), i18n("Terminal"));
  m_console = new KateConsole(mainWindow, toolview);
}

KateKonsolePluginView::~KateKonsolePluginView ()
{
  // cleanup, kill toolview + console
  QWidget *toolview = m_console->parentWidget();
  delete m_console;
  delete toolview;
}

KateConsole::KateConsole (Kate::MainWindow *mw, QWidget *parent)
    : KVBox (parent), KXMLGUIClient()
    , m_part (0)
    , m_mw (mw)
    , m_toolView (parent)
{
  QAction* a = actionCollection()->addAction("katekonsole_tools_pipe_to_terminal");
  a->setIcon(KIcon("pipe"));
  a->setText(i18n("&Pipe to Console"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotPipeToConsole()));

  a = actionCollection()->addAction("katekonsole_tools_sync");
  a->setText(i18n("S&yncronize Console with Current Document"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotSync()));

  a = actionCollection()->addAction("katekonsole_tools_toggle_focus");
  a->setIcon(KIcon("terminal"));
  a->setText(i18n("&Focus Console"));
  a->setShortcut( Qt::Key_F8 );
  connect(a, SIGNAL(triggered()), this, SLOT(slotToggleFocus()));

  setComponentData (KComponentData("kate"));
  setXMLFile("plugins/katekonsole/ui.rc");
  m_mw->guiFactory()->addClient (this);
}

KateConsole::~KateConsole ()
{
  m_mw->guiFactory()->removeClient (this);
  if (m_part)
    disconnect ( m_part, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );
}

void KateConsole::loadConsoleIfNeeded()
{
  if (m_part) return;

  if (!window() || !parentWidget()) return;
  if (!window() || !isVisibleTo(window())) return;

  KPluginFactory *factory = KPluginLoader("libkonsolepart").factory();

  if (!factory) return;

  m_part = static_cast<KParts::ReadOnlyPart *>(factory->create<QObject>(this, this));

  if (!m_part) return;

  // start the terminal
  qobject_cast<TerminalInterface*>(m_part)->showShellInDir( QString() );

  KGlobal::locale()->insertCatalog("konsole");

  setFocusProxy(m_part->widget());
  m_part->widget()->show();

  connect ( m_part, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );

  slotSync();
}

void KateConsole::slotDestroyed ()
{
  m_part = 0;

  // hide the dockwidget
  if (parentWidget())
  {
    m_mw->hideToolView (m_toolView);
    m_mw->centralWidget()->setFocus ();
  }
}

void KateConsole::showEvent(QShowEvent *)
{
  if (m_part) return;

  loadConsoleIfNeeded();
}

void KateConsole::cd (const KUrl &url)
{
  sendInput("cd " + KShell::quoteArg(url.path()) + '\n');
}

void KateConsole::sendInput( const QString& text )
{
  loadConsoleIfNeeded();

  if (!m_part) return;

  TerminalInterface *t = qobject_cast<TerminalInterface *>(m_part);

  if (!t) return;

  t->sendInput (text);
}

void KateConsole::slotPipeToConsole ()
{
  if (KMessageBox::warningContinueCancel
      (m_mw->window()
       , i18n ("Do you really want to pipe the text to the console? This will execute any contained commands with your user rights.")
       , i18n ("Pipe to Console?")
       , KGuiItem(i18n("Pipe to Console")), KStandardGuiItem::cancel(), "Pipe To Console Warning") != KMessageBox::Continue)
    return;

  KTextEditor::View *v = m_mw->activeView();

  if (!v)
    return;

  if (v->selection())
    sendInput (v->selectionText());
  else
    sendInput (v->document()->text());
}

void KateConsole::slotSync() {
  if (m_mw->activeView()
    && m_mw->activeView()->document()->url().isValid()
    && m_mw->activeView()->document()->url().isLocalFile() )
      cd(KUrl( m_mw->activeView()->document()->url().directory() ));
}

void KateConsole::slotToggleFocus()
{
  QAction *action = actionCollection()->action("katekonsole_tools_toggle_focus");
  if ( ! m_part ) {
    m_mw->showToolView( parentWidget() );
    action->setText( i18n("Defocus Console") );
    return; // this shows and focuses the konsole
  }

  if ( ! m_part ) return;

  if (m_part->widget()->hasFocus()) {
    if (m_mw->activeView())
      m_mw->activeView()->setFocus();
      action->setText( i18n("Focus Console") );
  } else {
    // show the view if it is hidden
    if (parentWidget()->isHidden())
      m_mw->showToolView( parentWidget() );
    else // should focus the widget too!
      m_part->widget()->setFocus( Qt::OtherFocusReason );
    action->setText( i18n("Defocus Console") );
  }
}
// kate: space-indent on; indent-width 2; replace-tabs on;

