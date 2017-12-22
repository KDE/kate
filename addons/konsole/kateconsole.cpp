/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Anders Lund <anders@alweb.dk>
   Copyright (C) 2017 Ederag <edera@gmx.fr>

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

#include <klocalizedstring.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/message.h>

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
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileInfo>

#include <KPluginLoader>
#include <KPluginFactory>
#include <kaboutdata.h>
#include <kpluginfactory.h>
#include <KAuthorized>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KXMLGUIFactory>

K_PLUGIN_FACTORY_WITH_JSON (KateKonsolePluginFactory, "katekonsoleplugin.json", registerPlugin<KateKonsolePlugin>();)

KateKonsolePlugin::KateKonsolePlugin( QObject* parent, const QList<QVariant>& ):
    KTextEditor::Plugin ( parent )
{
  m_previousEditorEnv=qgetenv("EDITOR");
  if (!KAuthorized::authorize(QStringLiteral("shell_access")))
  {
    KMessageBox::sorry(nullptr, i18n ("You do not have enough karma to access a shell or terminal emulation"));
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
    return nullptr;
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
  QWidget *toolview = mainWindow->createToolView (plugin, QStringLiteral("kate_private_plugin_katekonsoleplugin"), KTextEditor::MainWindow::Bottom, QIcon::fromTheme(QStringLiteral("utilities-terminal")), i18n("Terminal"));
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
    , m_part (nullptr)
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
  connect(a, &QAction::triggered, this, &KateConsole::slotPipeToConsole);

  a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_sync"));
  a->setText(i18nc("@action", "S&ynchronize Terminal with Current Document"));
  connect(a, &QAction::triggered, this, &KateConsole::slotManualSync);

  a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_run"));
  a->setText(i18nc("@action", "Run Current Document"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotRun()));

  a = actionCollection()->addAction(QStringLiteral("katekonsole_tools_toggle_focus"));
  a->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
  a->setText(i18nc("@action", "&Focus Terminal"));
  connect(a, &QAction::triggered, this, &KateConsole::slotToggleFocus);

  m_mw->guiFactory()->addClient (this);

  readConfig();
}

KateConsole::~KateConsole ()
{
  m_mw->guiFactory()->removeClient (this);
  if (m_part)
    disconnect(m_part, &KParts::ReadOnlyPart::destroyed, this, &KateConsole::slotDestroyed);
}

void KateConsole::loadConsoleIfNeeded()
{
  if (m_part) return;

  if (!window() || !parentWidget()) return;
  if (!window() || !isVisibleTo(window())) return;


  /**
   * get konsole part factory
   */
  KPluginFactory *factory = KPluginLoader(QStringLiteral("konsolepart")).factory();
  if (!factory)
    return;

  m_part = factory->create<KParts::ReadOnlyPart>(this, this);

  if (!m_part) return;

  layout()->addWidget(m_part->widget());

  // start the terminal
  qobject_cast<TerminalInterface*>(m_part)->showShellInDir( QString() );

//   KGlobal::locale()->insertCatalog("konsole"); // FIXME KF5: insert catalog

  setFocusProxy(m_part->widget());
  m_part->widget()->show();

  connect(m_part, &KParts::ReadOnlyPart::destroyed, this, &KateConsole::slotDestroyed);
  connect ( m_part, SIGNAL(overrideShortcut(QKeyEvent*,bool&)),
                    this, SLOT(overrideShortcut(QKeyEvent*,bool&)));
  slotSync();
}

void KateConsole::slotDestroyed ()
{
  m_part = nullptr;
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
  QString command = QStringLiteral(" cd ") + KShell::quoteArg(m_currentPath) + QLatin1Char('\n');

  // special handling for some interpreters
  TerminalInterface *t = qobject_cast<TerminalInterface *>(m_part);
  if (t) {
    // ghci doesn't allow \space dir names, does allow spaces in dir names
    // irb can take spaces or \space but doesn't allow " 'path' "
    if (t->foregroundProcessName() == QStringLiteral("irb") ) {
      command = QStringLiteral("Dir.chdir(\"") + path + QStringLiteral("\") \n") ;
    } else if(t->foregroundProcessName() == QStringLiteral("ghc")) {
      command = QStringLiteral(":cd ") + path + QStringLiteral("\n");
    }
  }
  sendInput(command);
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

void KateConsole::slotRun()
{
  if ( m_mw->activeView() ) {
    KTextEditor::Document *document = m_mw->activeView()->document();
    QUrl u = document->url();
    if ( ! u.isLocalFile() ) {
      QPointer<KTextEditor::Message> message =
          new KTextEditor::Message(
            i18n("Not a local file: '%1'", u.path()),
            KTextEditor::Message::Error
          );
      // auto hide is enabled and set to a sane default value of several seconds.
      message->setAutoHide(2000);
      message->setAutoHideMode( KTextEditor::Message::Immediate );
      document->postMessage( message );
      return;
    }
    // ensure that file is saved
    if ( document->isModified() ) {
      document->save();
    }

    // The string that should be output to terminal, upon acceptance
    QString output_str;
    // prefix first
    output_str += KConfigGroup( KSharedConfig::openConfig(),
                                "Konsole"
                              ).readEntry("RunPrefix", "");
    // then filename
    if ( KConfigGroup(KSharedConfig::openConfig(),
                      "Konsole"
                     ).readEntry("RemoveExtension", true) ) {
      // append filename without extension (i.e. keep only the basename)
      output_str +=  QFileInfo( u.path() ).baseName() + QLatin1Char('\n');
    } else {
      // append filename to the terminal
      output_str += QFileInfo( u.path() ).fileName() + QLatin1Char('\n');
    }

    if ( KMessageBox::Continue !=
         KMessageBox::warningContinueCancel(
             m_mw->window(),
             i18n("Do you really want to Run the document ?\n"
                  "This will execute the following command,\n"
                  "with your user rights, in the terminal:\n"
                  "'%1'", output_str),
             i18n("Run in Terminal?"),
             KGuiItem( i18n("Run") ),
             KStandardGuiItem::cancel(),
             QStringLiteral("Konsole: Run in Terminal Warning") )
        ) {
      return;
    }
    // echo to terminal
    sendInput( output_str );
  }
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

  QVBoxLayout *vboxRun = new QVBoxLayout;
  QGroupBox *groupRun = new QGroupBox( i18n("Run in terminal"), this );
  // Remove extension
  cbRemoveExtension = new QCheckBox( i18n("&Remove extension"), this );
  vboxRun->addWidget( cbRemoveExtension );
  // Prefix
  QFrame *framePrefix = new QFrame( this );
  QHBoxLayout *hboxPrefix = new QHBoxLayout( framePrefix );
  QLabel *label = new QLabel( i18n("Prefix:"), framePrefix );
  hboxPrefix->addWidget( label );
  lePrefix = new QLineEdit( framePrefix );
  hboxPrefix->addWidget( lePrefix );
  vboxRun->addWidget( framePrefix );
  // show warning next time
  QFrame *frameWarn = new QFrame( this );
  QHBoxLayout *hboxWarn = new QHBoxLayout( frameWarn );
  QPushButton *buttonWarn = new QPushButton( i18n("&Show warning next time"), frameWarn);
  buttonWarn->setWhatsThis (
    i18n ( "The next time '%1' is executed, "
           "make sure a warning window will pop up, "
           "displaying the command to be sent to terminal, "
           "for review.",
           i18n ("Run in terminal")
         )
  );
  connect( buttonWarn, &QPushButton::pressed,
           this, &KateKonsoleConfigPage::slotEnableRunWarning );
  hboxWarn->addWidget( buttonWarn );
  vboxRun->addWidget( frameWarn );
  groupRun->setLayout( vboxRun );
  lo->addWidget( groupRun );

  cbSetEditor = new QCheckBox( i18n("Set &EDITOR environment variable to 'kate -b'"), this );
  lo->addWidget( cbSetEditor );
  QLabel *tmp = new QLabel(this);
  tmp->setText(i18n("Important: The document has to be closed to make the console application continue"));
  lo->addWidget(tmp);
  reset();
  lo->addStretch();
  connect(cbAutoSyncronize, &QCheckBox::stateChanged, this, &KateKonsoleConfigPage::changed);
  connect( cbRemoveExtension, SIGNAL(stateChanged(int)), SIGNAL(changed()) );
  connect( lePrefix, &QLineEdit::textChanged,
           this, &KateKonsoleConfigPage::changed );
  connect(cbSetEditor, &QCheckBox::stateChanged, this, &KateKonsoleConfigPage::changed);
}

void KateKonsoleConfigPage::slotEnableRunWarning ()
{
  KMessageBox::enableMessage(QStringLiteral("Konsole: Run in Terminal Warning"));
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
  config.writeEntry("RemoveExtension", cbRemoveExtension->isChecked());
  config.writeEntry("RunPrefix", lePrefix->text());
  config.writeEntry("SetEditor", cbSetEditor->isChecked());
  config.sync();
  mPlugin->readConfig();
}

void KateKonsoleConfigPage::reset()
{
  KConfigGroup config(KSharedConfig::openConfig(), "Konsole");
  cbAutoSyncronize->setChecked(config.readEntry("AutoSyncronize", false));
  cbRemoveExtension->setChecked(config.readEntry("RemoveExtension", false));
  lePrefix->setText(config.readEntry("RunPrefix", ""));
  cbSetEditor->setChecked(config.readEntry("SetEditor", false));
}

#include "kateconsole.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;

