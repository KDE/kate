 /***************************************************************************
                          plugin_kateinsertcommand.cpp  -  description
                             -------------------
    begin                : THU Apr 19 2001
    copyright            : (C) 2001 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//BEGIN includes
#include "plugin_kateinsertcommand.h"
#include "plugin_kateinsertcommand.moc"

#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qdir.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <q3whatsthis.h>
#include <qwidget.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kaction.h>
#include <kanimwidget.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <k3process.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>
#include <kauthorized.h>
//END includes

K_EXPORT_COMPONENT_FACTORY( kateinsertcommandplugin, KGenericFactory<PluginKateInsertCommand>( "kateinsertcommand" ) )

//BEGIN obligatory stuff
class PluginView : public KXMLGUIClient
{
  friend class PluginKateInsertCommand;

  public:
    Kate::MainWindow *win;
};
//END

//BEGIN PluginKateInsertCommand
PluginKateInsertCommand::PluginKateInsertCommand( QObject* parent, const char* name, const QStringList& )
    : Kate::Plugin ( (Kate::Application *)parent, name ),
      kv              ( 0 ),
      sh              ( 0 )
{
  config = new KConfig("kateinsertcommandpluginrc");
  cmdhist = config->readEntry("Command History",QStringList());
  wdlg = 0;
  workingdir = QDir::currentPath();
}

PluginKateInsertCommand::~PluginKateInsertCommand()
{
  // write config file
  config->writeEntry("Command History", cmdhist);
  config->writeEntry("Dialog Settings", dialogSettings);
  config->sync();
  delete config;
  delete sh;
}

void PluginKateInsertCommand::addView(Kate::MainWindow *win)
{
    // TODO: doesn't this have to be deleted?
    PluginView *view = new PluginView ();

    (void) new KAction ( i18n("Insert Command..."), "", 0, this,
                      SLOT( slotInsertCommand() ), view->actionCollection(),
                      "edit_insert_command" );

    view->setComponentData (KComponentData("kate"));
    view->setXMLFile("plugins/kateinsertcommand/ui.rc");
    win->guiFactory()->addClient (view);
    view->win = win;

   m_views.append (view);
}

void PluginKateInsertCommand::removeView(Kate::MainWindow *win)
{
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginView *view = m_views.at(z);
      m_views.remove (view);
      win->guiFactory()->removeClient (view);
      delete view;
    }
}

void PluginKateInsertCommand::slotInsertCommand()
{
  if (!KAuthorized::authorizeKAction("shell_access")) {
      KMessageBox::sorry(0,i18n("You are not allowed to execute arbitrary external applications. If you want to be able to do this, contact your system administrator."),i18n("Access Restrictions"));
      return;
  }
  if ( sh && sh->isRunning() ) {
    KMessageBox::sorry (0, i18n("A process is currently being executed."),
                        i18n("Error"));
    return;
  }

  if (!application()->activeMainWindow() || !application()->activeMainWindow()->activeView())
    return;

  kv = application()->activeMainWindow()->activeView();

  QString dir = workingdir;
  QString docdir;
  KUrl docurl = kv->document()->url();
  if (docurl.isLocalFile())
    docdir = docurl.directory();
  QString lwd( config->readPathEntry("Last WD") );
  switch ( (int)config->readNumEntry("Start In", 0) )
  {
    case 1:
      if ( ! docdir.isEmpty() ) dir = docdir;
      break;
    case 2:
      if ( ! lwd.isEmpty() ) dir = lwd;
      break;
    default:
      break;
  }
  dialogSettings = config->readNumEntry("Dialog Settings", 0);
  CmdPrompt *d = new CmdPrompt((QWidget*)kv, 0, cmdhist, dir,
                               docdir, dialogSettings);
  if ( d->exec() && ! d->command().isEmpty() ) {
    if ( ! sh ) {
    sh = new K3ShellProcess;

    connect ( sh, SIGNAL(receivedStdout(K3Process*, char*, int)),
              this, SLOT(slotReceivedStdout(K3Process*, char*, int)) );
    connect ( sh, SIGNAL(receivedStderr(K3Process*, char*, int)),
              this, SLOT(slotReceivedStderr(K3Process*, char*, int)) );
    connect ( sh, SIGNAL(processExited(K3Process*)),
              this, SLOT(slotProcessExited(K3Process*)) ) ;
    }

    sh->clearArguments();

    bInsStdErr = d->insertStdErr();

    if ( d->printCmd() ) {
      if ( ! d->wd().isEmpty() )
        kv->insertText( d->wd() + ": ");
      kv->insertText( d->command()+":\n" );
    }
    if ( ! d->wd().isEmpty() ) {
      *sh << "cd" << d->wd() << "&&";
      config->writePathEntry("Last WD", d->wd());
    }
    *sh << QFile::encodeName(d->command());
    sh->start( K3Process::NotifyOnExit, K3Process::All );

    // add command to history
    if ( cmdhist.contains( d->command() ) ) {
      cmdhist.remove( d->command() );
    }
    cmdhist.prepend( d->command() );
    int cmdhistlen = config->readNumEntry("Command History Length", 20);
    while ( (int)cmdhist.count() > cmdhistlen )
         cmdhist.remove( cmdhist.last() );
    // save dialog state
    dialogSettings = 0;
    if ( d->insertStdErr() )
      dialogSettings += 1;
    if ( d->printCmd() )
      dialogSettings += 2;

    cmd = d->command();
    delete d;
    // If process is still running, display a dialog to cancel...
    slotShowWaitDlg();

    config->writeEntry("Dialog Settings", dialogSettings);
    config->sync();
  }
}

void PluginKateInsertCommand::slotAbort()
{
  if ( sh->isRunning() )
    if (! sh->kill() )
      KMessageBox::sorry(0, i18n("Could not kill command."), i18n("Kill Failed"));
}

void PluginKateInsertCommand::slotShowWaitDlg()
{
    if ( sh->isRunning() ) {
      wdlg = new WaitDlg( (QWidget*)kv, i18n(
        "Executing command:\n%1\n\nPress 'Cancel' to abort.", cmd)  );
      connect(wdlg, SIGNAL(cancelClicked()), this, SLOT(slotAbort()) );
    }
    if ( sh->isRunning() )    // we may have finished while creating the dialog.
      wdlg->show();
    else if (wdlg) { // process may have exited before the WaitDlg constructor returned.
      delete wdlg;
      wdlg = 0;
    }
}

void PluginKateInsertCommand::slotReceivedStdout( K3Process* /*p*/, char* text,
                                                  int len )
{
  QString t = QString::fromLocal8Bit ( text );
  t.truncate(len);
  kv->insertText( t );
}

void PluginKateInsertCommand::slotReceivedStderr( K3Process* p, char* text,
                                                  int len )
{
  if ( bInsStdErr )
    slotReceivedStdout( p, text, len );
}

void PluginKateInsertCommand::slotProcessExited( K3Process* p )
{
  if (wdlg) {
    wdlg->hide();
    delete wdlg;
    wdlg = 0;
  }
  if ( ! p->normalExit() )
    KMessageBox::sorry(0, i18n("Command exited with status %1", 
                               p->exitStatus()), i18n("Error"));
  kv->setFocus();
}
//END PluginKateInsertCommand

//BEGIN PluginConfigPage
Kate::PluginConfigPage* PluginKateInsertCommand::configPage (uint,
                                  QWidget *w, const char */*name*/)
{
  InsertCommandConfigPage* p = new InsertCommandConfigPage(this, w);
  initConfigPage( p );
  connect( p, SIGNAL(configPageApplyRequest(InsertCommandConfigPage*)),
           this, SLOT(applyConfig(InsertCommandConfigPage*)) );
  return (Kate::PluginConfigPage*)p;
}

void PluginKateInsertCommand::initConfigPage( InsertCommandConfigPage *p )
{
  p->sb_cmdhistlen->setValue( config->readNumEntry("Command History Length", 20) );
  p->rg_startin->setButton( config->readNumEntry("Start In", 0) );
}

void PluginKateInsertCommand::applyConfig( InsertCommandConfigPage *p )
{
  config->writeEntry( "Command History Length", p->sb_cmdhistlen->value() );
  // truncate the cmd hist if nessecary?
  config->writeEntry( "Start In", p->rg_startin->id(p->rg_startin->selected()) );
  config->sync();
}
//END PluginConfigPage

//BEGIN CmdPrompt
// This is a simple dialog to retrieve a command and decide if
// stdErr should be included in the text inserted.
CmdPrompt::CmdPrompt(QWidget* parent,
                     const char* name,
                     const QStringList& cmdhist,
                     const QString& dir,
                     const QString& /*docdir*/,
                     int settings)
  : KDialogBase (parent, name, true, i18n("Insert Command"), Ok|Cancel, Ok, true)
{
   QWidget *page = new QWidget( this );
   setMainWidget(page);

   QVBoxLayout *lo = new QVBoxLayout( page, 0, spacingHint() );

   QLabel *l = new QLabel( i18n("Enter &command:"), page );
   lo->addWidget( l );
   cmb_cmd = new KHistoryCombo(true, page);
   cmb_cmd->setHistoryItems(cmdhist, true);
   cmb_cmd->setCurrentItem(0);
   cmb_cmd->lineEdit()->setSelection(0, cmb_cmd->currentText().length());
   l->setBuddy(cmb_cmd);
   cmb_cmd->setFocus();
   lo->addWidget(cmb_cmd);
   connect( cmb_cmd->lineEdit(),SIGNAL(textChanged ( const QString & )),
            this, SLOT( slotTextChanged(const QString &)));

   QLabel *lwd = new QLabel( i18n("Choose &working folder:"), page );
   lo->addWidget( lwd );
   wdreq = new KUrlRequester( page );
   if ( ! dir.isEmpty() )
     wdreq->setURL( dir );
   wdreq->setMode( static_cast<KFile::Mode>(KFile::Directory|KFile::LocalOnly|KFile::ExistingOnly) );
   lwd->setBuddy( wdreq );
   lo->addWidget( wdreq );

   //kDebug()<<"settings: "<<settings<<endl;
   cb_insStdErr = new QCheckBox( i18n("Insert Std&Err messages"), page );
   cb_insStdErr->setChecked(settings & 1);
   lo->addWidget( cb_insStdErr );
   cb_printCmd = new QCheckBox( i18n("&Print command name"), page );
   cb_printCmd->setChecked(settings & 2);
   lo->addWidget( cb_printCmd );

   Q3WhatsThis::add( cmb_cmd, i18n(
     "Enter the shell command, the output of which you want inserted into your "
     "document. Feel free to use a pipe or two if you wish.") );
   Q3WhatsThis::add( wdreq, i18n(
     "Sets the working folder of the command. The command executed is 'cd <dir> "
     "&& <command>'") );
   Q3WhatsThis::add( cb_insStdErr, i18n(
     "Check this if you want the error output from <command> inserted as well.\n"
     "Some commands, such as locate, print everything to STDERR") );
   Q3WhatsThis::add( cb_printCmd, i18n(
     "If you check this, the command string will be printed followed by a "
     "newline before the output.") );
   slotTextChanged(cmb_cmd->lineEdit()->text());
}

CmdPrompt::~CmdPrompt() {}

void CmdPrompt::slotTextChanged(const QString &text)
{
    enableButtonOk( !text.isEmpty());
}
//END CmdPrompt

//BEGIN WaitDlg implementation
// This is a dialog that is displayed while a command is running,
// with a cancel button to allow the user to kill the command
WaitDlg::WaitDlg(QWidget* parent, const QString& text, const QString& title)
  : KDialogBase( parent, "wait dialog", true, title, Cancel, Cancel, true )
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QHBoxLayout *lo = new QHBoxLayout( page, 0, spacingHint() );

  KAnimWidget *aw = new KAnimWidget( QString::fromLatin1("kde"), 48, page );
  lo->addWidget(aw);
  QLabel *l = new QLabel( text, page );
  lo->addWidget( l );

  aw->start();
}
WaitDlg::~WaitDlg()
{
}
//END WaitDlg

//BEGIN InsertCommandConfigPage
// This is the config page for this plugin.
InsertCommandConfigPage::InsertCommandConfigPage(QObject* /*parent*/,
                                                 QWidget *parentWidget)
  : Kate::PluginConfigPage( parentWidget )
{
  QVBoxLayout* lo = new QVBoxLayout( this );
  lo->setSpacing(KDialogBase::spacingHint());

  // command history length
  Q3HBox *hb1 = new Q3HBox( this );
  hb1->setSpacing(KDialogBase::spacingHint());
  (void) new QLabel( i18n("Remember"), hb1 );
  sb_cmdhistlen = new QSpinBox( hb1 );
  QLabel *l1 =  new QLabel( sb_cmdhistlen, i18n("Co&mmands"), hb1);
  hb1->setStretchFactor(l1, 1);
  lo->addWidget( hb1 );

  // dir history length

  // initial dir choice
  rg_startin = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Start In"), this );
  rg_startin->setRadioButtonExclusive( true );
  (void) new QRadioButton( i18n("Application &working folder"), rg_startin);
  (void) new QRadioButton( i18n("&Document folder"), rg_startin);
  (void) new QRadioButton( i18n("&Latest used working folder"), rg_startin);
  lo->addWidget( rg_startin );
  // other?

  lo->addStretch(1);  // look nice

  // Be helpfull!
  Q3WhatsThis::add( sb_cmdhistlen, i18n(
    "Sets the number of commands to remember. The command history is saved "
    "over sessions.") );
  Q3WhatsThis::add( rg_startin, i18n(
    "<qt><p>Decides what is suggested as <em>working folder</em> for the "
    "command.</p><p><strong>Application Working Folder (default):</strong> "
    "The folder from which you launched the application hosting the plugin, "
    "usually your home folder.</p><p><strong>Document Folder:</strong> The "
    "folder of the document. Used only for local documents.</p><p><strong>"
    "Latest Working Folder:</strong> The folder used last time you used this "
    "plugin.</p></qt>") );
}

void InsertCommandConfigPage::apply()
{
  emit configPageApplyRequest( this );
}
//END InsertCommandConfigPage
// kate: space-indent on; indent-width 2; replace-tabs on;
