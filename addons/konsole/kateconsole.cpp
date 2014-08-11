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

#include <kiconloader.h>
#include <klocalizedstring.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kde_terminal_interface.h>
#include <kshell.h>
#include <kparts/part.h>
#include <QAction>
#include <kactioncollection.h>

#include <kmessagebox.h>

#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QShowEvent>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QFileInfo>

#include <kpluginloader.h>
#include <kservice.h>
#include <kaboutdata.h>
#include <kpluginfactory.h>
#include <kauthorized.h>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON (KateKonsolePluginFactory, "katekonsoleplugin.json", registerPlugin<KateKonsolePlugin>();)

KateKonsolePlugin::KateKonsolePlugin( QObject* parent, const QList<QVariant>& ):
    KTextEditor::Plugin ( parent )
{
  m_previousEditorEnv=qgetenv("EDITOR");
  if (!KAuthorized::authorizeKAction(QStringLiteral("shell_access")))
  {
    KMessageBox::sorry(0, i18n ("You do not have enough karma to access a shell or terminal emulation"));
  }
}

KateKonsolePlugin::~KateKonsolePlugin()
{
  qputenv("EDITOR", m_previousEditorEnv.data());
}

QObject *KateKonsolePlugin::createView (KTextEditor::MainWindow *mainWindow)
{
  KateKonsolePluginView *view = new KateKonsolePluginView (this, mainWindow);
  return view;
}

KTextEditor::ConfigPage *KateKonsolePlugin::configPage (int number, QWidget *parent)
{
  if (number != 0)
    return 0;
  return new KateKonsoleConfigPage(parent, this);
}

void KateKonsolePlugin::readConfig()
{
  foreach ( KateKonsolePluginView *view, mViews )
    view->readConfig();
}

KateKonsolePluginView::KateKonsolePluginView (KateKonsolePlugin* plugin, KTextEditor::MainWindow *mainWindow)
    : QObject(mainWindow),m_plugin(plugin)
{
  // init console
  QWidget *toolview = mainWindow->createToolView (plugin, QStringLiteral("kate_private_plugin_katekonsoleplugin"), KTextEditor::MainWindow::Bottom, SmallIcon(QStringLiteral("utilities-terminal")), i18n("Terminal"));
  m_console = new KateConsole(m_plugin, mainWindow, toolview);
  
  // register this view
  m_plugin->mViews.append ( this );
}

KateKonsolePluginView::~KateKonsolePluginView ()
{
  // unregister this view
  m_plugin->mViews.removeAll (this);
  
  // cleanup, kill toolview + console
  QWidget *toolview = m_console->parentWidget();
  delete m_console;
  delete toolview;
}

void KateKonsolePluginView::readConfig()
{
  m_console->readConfig();
}

KateConsole::KateConsole (KateKonsolePlugin* plugin, KTextEditor::MainWindow *mw, QWidget *parent)
    : QWidget (parent)
    , m_part (0)
    , m_mw (mw)
    , m_toolView (parent)
    , m_plugin(plugin)
{
  KXMLGUIClient::setComponentName (QStringLiteral("katekonsole"), i18n ("Kate Terminal"));
  setXMLFile( QStringLiteral("ui.rc") );

  // make sure we have a vertical layout
  new QVBoxLayout(this);
  layout()->setContentsMargins(0, 0, 0, 0);

  QAction* a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_pipe_to_terminal"));
  a->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
  a->setText(i18nc("@action", "&Pipe to Terminal"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotPipeToConsole()));

  a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_sync"));
  a->setText(i18nc("@action", "S&ynchronize Terminal with Current Document"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotManualSync()));

  a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_toggle_focus"));
  a->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
  a->setText(i18nc("@action", "&Focus Terminal"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotToggleFocus()));

  m_mw->guiFactory()->addClient (this);

  readConfig();
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

  KPluginFactory* factory = 0;
  KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("konsolepart"));
  if (service) {
      factory = KPluginLoader(service->library()).factory();
  }

  if (!factory) return;

  m_part = static_cast<KParts::ReadOnlyPart *>(factory->create<QObject>(this, this));

  if (!m_part) return;

  layout()->addWidget(m_part->widget());

  // start the terminal
  qobject_cast<TerminalInterface*>(m_part)->showShellInDir( QString() );

//   KGlobal::locale()->insertCatalog("konsole"); // FIXME KF5: insert catalog

  setFocusProxy(m_part->widget());
  m_part->widget()->show();

  connect ( m_part, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );
  connect ( m_part, SIGNAL(overrideShortcut(QKeyEvent*,bool&)),
                    this, SLOT(overrideShortcut(QKeyEvent*,bool&)));
  slotSync();
}

void KateConsole::slotDestroyed ()
{
  m_part = 0;
  m_currentPath.clear ();

  // hide the dockwidget
  if (parentWidget()) {
    m_mw->hideToolView (m_toolView);
  }
}

void KateConsole::overrideShortcut (QKeyEvent *, bool &override)
{
  /**
   * let konsole handle all shortcuts
   */
  override = true;
}

void KateConsole::showEvent(QShowEvent *)
{
  if (m_part) return;

  loadConsoleIfNeeded();
}

void KateConsole::cd (const QString & path)
{
  if (m_currentPath == path)
    return;

  if (!m_part)
    return;

  m_currentPath = path;
  sendInput(QStringLiteral("cd ") + KShell::quoteArg(m_currentPath) + QLatin1Char('\n'));
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
       , i18n ("Pipe to Terminal?")
       , KGuiItem(i18n("Pipe to Terminal")), KStandardGuiItem::cancel(), QStringLiteral("Pipe To Terminal Warning")) != KMessageBox::Continue)
    return;

  KTextEditor::View *v = m_mw->activeView();

  if (!v)
    return;

  if (v->selection())
    sendInput (v->selectionText());
  else
    sendInput (v->document()->text());
}

void KateConsole::slotSync(KTextEditor::View *)
{
  if (m_mw->activeView()) {
    QUrl u = m_mw->activeView()->document()->url();
    if (u.isValid() && u.isLocalFile()) {
      QFileInfo fi(u.toLocalFile());
      cd(fi.absolutePath());
    } else if (!u.isEmpty()) {
      sendInput( QStringLiteral("### ") + i18n("Sorry, cannot cd into '%1'", u.toLocalFile() ) + QLatin1Char('\n') );
    }
  }
}

void KateConsole::slotManualSync()
{
  m_currentPath.clear ();
  slotSync();
  if ( ! m_part || ! m_part->widget()->isVisible() )
    m_mw->showToolView( parentWidget() );
}
void KateConsole::slotToggleFocus()
{
  QAction *action = actionCollection()->action(QStringLiteral("katekonsole_tools_toggle_focus"));
  if ( ! m_part ) {
    m_mw->showToolView( parentWidget() );
    action->setText( i18n("Defocus Terminal") );
    return; // this shows and focuses the konsole
  }

  if ( ! m_part ) return;

  if (m_part->widget()->hasFocus()) {
    if (m_mw->activeView())
      m_mw->activeView()->setFocus();
      action->setText( i18n("Focus Terminal") );
  } else {
    // show the view if it is hidden
    if (parentWidget()->isHidden())
      m_mw->showToolView( parentWidget() );
    else // should focus the widget too!
      m_part->widget()->setFocus( Qt::OtherFocusReason );
    action->setText( i18n("Defocus Terminal") );
  }
}

void KateConsole::readConfig()
{
  disconnect(m_mw, &KTextEditor::MainWindow::viewChanged, this, &KateConsole::slotSync);
  if ( KConfigGroup(KSharedConfig::openConfig(), "Konsole").readEntry("AutoSyncronize", false) ) {
    connect(m_mw, &KTextEditor::MainWindow::viewChanged, this, &KateConsole::slotSync);
  }

  if ( KConfigGroup(KSharedConfig::openConfig(), "Konsole").readEntry("SetEditor", false) )
    qputenv( "EDITOR", "kate -b");
  else
    qputenv( "EDITOR", m_plugin->previousEditorEnv().data());
}

KateKonsoleConfigPage::KateKonsoleConfigPage( QWidget* parent, KateKonsolePlugin *plugin )
  : KTextEditor::ConfigPage( parent )
  , mPlugin( plugin )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  lo->setSpacing(QApplication::style()->pixelMetric(QStyle::PM_DefaultLayoutSpacing));

  cbAutoSyncronize = new QCheckBox( i18n("&Automatically synchronize the terminal with the current document when possible"), this );
  lo->addWidget( cbAutoSyncronize );
  cbSetEditor = new QCheckBox( i18n("Set &EDITOR environment variable to 'kate -b'"), this );
  lo->addWidget( cbSetEditor );
  QLabel *tmp = new QLabel(this);
  tmp->setText(i18n("Important: The document has to be closed to make the console application continue"));
  lo->addWidget(tmp);
  reset();
  lo->addStretch();
  connect( cbAutoSyncronize, SIGNAL(stateChanged(int)), SIGNAL(changed()) );
  connect( cbSetEditor, SIGNAL(stateChanged(int)), SIGNAL(changed()) );
}

QString KateKonsoleConfigPage::name() const
{
  return i18n("Terminal");
}

QString KateKonsoleConfigPage::fullName() const
{
  return i18n("Terminal Settings");
}

QIcon KateKonsoleConfigPage::icon() const
{
  return QIcon::fromTheme(QStringLiteral("utilities-terminal"));
}

void KateKonsoleConfigPage::apply()
{
  KConfigGroup config(KSharedConfig::openConfig(), "Konsole");
  config.writeEntry("AutoSyncronize", cbAutoSyncronize->isChecked());
  config.writeEntry("SetEditor", cbSetEditor->isChecked());
  config.sync();
  mPlugin->readConfig();
}

void KateKonsoleConfigPage::reset()
{
  KConfigGroup config(KSharedConfig::openConfig(), "Konsole");
  cbAutoSyncronize->setChecked(config.readEntry("AutoSyncronize", false));
  cbSetEditor->setChecked(config.readEntry("SetEditor", false));
}

#include "kateconsole.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;

