/*
   This file is part of the Kate text editor of the KDE project.

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

   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/
// TODO
// Icons
// Direct shortcut setting
//BEGIN Includes
#include "kateexternaltools.h"
#include "kateexternaltools.moc"

#include <KActionCollection>
#include <KListWidget>
#include <KLocale>
#include <KIconLoader>
#include <KMessageBox>
#include <KMimeTypeChooser>
#include <KXmlGuiWindow>
#include <KConfig>
#include <KConfigGroup>
#include <KRun>
#include <KIconDialog>
#include <KMenu>
#include <KSharedConfig>
#include <kdebug.h>

#include <QBitmap>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExp>
#include <QToolButton>
#include <QGridLayout>

#include <qtextedit.h>

#include <unistd.h>
//END Includes

KateExternalToolsCommand *KateExternalToolsCommand::s_self = 0;

//BEGIN KateExternalTool
KateExternalTool::KateExternalTool( const QString &name,
                                    const QString &command,
                                    const QString &icon,
                                    const QString &tryexec,
                                    const QStringList &mimetypes,
                                    const QString &acname,
                                    const QString &cmdname,
                                    int save )
    : name ( name ),
    command ( command ),
    icon ( icon ),
    tryexec ( tryexec ),
    mimetypes ( mimetypes ),
    acname ( acname ),
    cmdname ( cmdname ),
    save ( save )
{
  //if ( ! tryexec.isEmpty() )
  hasexec = checkExec();
}

bool KateExternalTool::checkExec()
{
  // if tryexec is empty, it is the first word of command
  if ( tryexec.isEmpty() )
    tryexec = command.section( " ", 0, 0, QString::SectionSkipEmpty );

  // NOTE this code is modified taken from kdesktopfile.cpp, from KDesktopFile::tryExec()
  if (!tryexec.isEmpty())
  {
    if (tryexec[0] == '/')
    {
      if (::access(QFile::encodeName(tryexec), R_OK | X_OK))
        return false;

      m_exec = tryexec;
    }
    else
    {
      // !!! Sergey A. Sukiyazov <corwin@micom.don.ru> !!!
      // Environment PATH may contain filenames in 8bit locale cpecified
      // encoding (Like a filenames).
      QString path = QFile::decodeName(::getenv("PATH"));
      QStringList dirs = path.split(':', QString::SkipEmptyParts);
      QStringList::Iterator it(dirs.begin());
      bool match = false;
      for (; it != dirs.end(); ++it)
      {
        QString fName = *it + '/' + tryexec;
        if (::access(QFile::encodeName(fName), R_OK | X_OK) == 0)
        {
          match = true;
          m_exec = fName;
          break;
        }
      }
      // didn't match at all
      if (!match)
        return false;
    }
    return true;
  }
  return false;
}

bool KateExternalTool::valid( const QString &mt ) const
{
  return mimetypes.isEmpty() || mimetypes.contains( mt );
}
//END KateExternalTool

//BEGIN KateExternalToolsCommand
KateExternalToolsCommand::KateExternalToolsCommand() : KTextEditor::Command()
{
  m_inited = false;
  reload();
}

const QStringList &KateExternalToolsCommand::cmds ()
{
  return m_list;
}

KateExternalToolsCommand *KateExternalToolsCommand::self ()
{
  if (s_self) return s_self;
  s_self = new KateExternalToolsCommand;
  return s_self;
}

void KateExternalToolsCommand::reload ()
{
  m_list.clear();
  m_map.clear();
  m_name.clear();

  KConfig _config( "externaltools", KConfig::NoGlobals, "appdata" );
  KConfigGroup config(&_config, "Global");
  QStringList tools = config.readEntry("tools", QStringList());


  for( QStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
      continue;


    config= KConfigGroup(&_config,  *it );

    KateExternalTool t = KateExternalTool(
                           config.readEntry( "name", "" ),
                           config.readEntry( "command", ""),
                           config.readEntry( "icon", ""),
                           config.readEntry( "executable", ""),
                           config.readEntry( "mimetypes", QStringList() ),
                           config.readEntry( "acname", "" ),
                           config.readEntry( "cmdname", "" ) );
    // FIXME test for a command name first!
    if ( t.hasexec && (!t.cmdname.isEmpty()))
    {
      m_list.append("exttool-" + t.cmdname);
      m_map.insert("exttool-" + t.cmdname, t.acname);
      m_name.insert("exttool-" + t.cmdname, t.name);
    }
  }
  if (m_inited)
  {
    KTextEditor::CommandInterface* cmdIface =
      qobject_cast<KTextEditor::CommandInterface*>( Kate::application()->editor() );
    if( cmdIface )
    {
      // reregister commands, in case of something has changed
      cmdIface->unregisterCommand( this );
      cmdIface->registerCommand( this );
    }
  }
  else m_inited = true;
}

bool KateExternalToolsCommand::exec (KTextEditor::View *view, const QString &cmd, QString &)
{
  QWidget *wv = dynamic_cast<QWidget*>(view);
  if (!wv)
  {
//   kDebug(13001)<<"KateExternalToolsCommand::exec: Could not get view widget";
    return false;
  }
  KXmlGuiWindow *dmw = dynamic_cast<KXmlGuiWindow*>(wv->window());
  if (!dmw)
  {
//   kDebug(13001)<<"KateExternalToolsCommand::exec: Could not get main window";
    return false;
  }
//  kDebug(13001)<<"cmd="<<cmd.trimmed();
  QString actionName = m_map[cmd.trimmed()];
  if (actionName.isEmpty()) return false;
//  kDebug(13001)<<"actionName is not empty:"<<actionName;
  KateExternalToolsMenuAction *a =
    dynamic_cast<KateExternalToolsMenuAction*>(dmw->action("tools_external"));
  if (!a) return false;
//  kDebug(13001)<<"trying to find action";
  QAction *a1 = a->actionCollection()->action(actionName.toUtf8().constData ());
  if (!a1) return false;
//  kDebug(13001)<<"activating action";
  a1->trigger();
  return true;
}

bool KateExternalToolsCommand::help (KTextEditor::View *, const QString &, QString &)
{
  return false;
}
//END KateExternalToolsCommand

//BEGIN KateExternalToolAction
KateExternalToolAction::KateExternalToolAction( QObject *parent, KateExternalTool *t)
    : KAction( KIcon(t->icon), t->name, parent ),
    tool ( t )
{
  //setText( t->name );
  //if ( ! t->icon.isEmpty() )
  //  setIcon( KIcon( t->icon ) );

  connect( this, SIGNAL(triggered(bool)), SLOT(slotRun()) );
}

bool KateExternalToolAction::expandMacro( const QString &str, QStringList &ret )
{
#if 0
  KateMainWindow *mw = (KateMainWindow*)parent()->parent();

  KTextEditor::View *view = mw->viewManager()->activeView();
  if ( ! view ) return false;

  if ( str == "URL" )
    ret += mw->activeDocumentUrl().url();
  else if ( str == "directory" ) // directory of current doc
    ret += mw->activeDocumentUrl().directory();
  else if ( str == "filename" )
    ret += mw->activeDocumentUrl().fileName();
  else if ( str == "line" ) // cursor line of current doc
    ret += QString::number( view->cursorPosition().line() );
  else if ( str == "col" ) // cursor col of current doc
    ret += QString::number( view->cursorPosition().column() );
  else if ( str == "selection" ) // selection of current doc if any
    ret += view->selectionText();
  else if ( str == "text" ) // text of current doc
    ret += view->document()->text();
  else if ( str == "URLs" )
  {
    foreach( KTextEditor::Document *doc, KateDocManager::self()->documentList())
    if ( ! doc->url().isEmpty() )
      ret += doc->url().url();
  }
  else
    return false;

#endif
  return true;
}

KateExternalToolAction::~KateExternalToolAction()
{
  delete(tool);
}

void KateExternalToolAction::slotRun()
{
#if 0
  // expand the macros in command if any,
  // and construct a command with an absolute path
  QString cmd = tool->command;

  if ( ! expandMacrosShellQuote( cmd ) )
  {
    KMessageBox::sorry( (KateMainWindow*)parent()->parent(),
                        i18n("Failed to expand the command '%1'.", cmd ),
                        i18n( "Kate External Tools") );
    return;
  }
  kDebug(13001) << "externaltools: Running command: " << cmd;

  // save documents if requested
  KateMainWindow *mw = (KateMainWindow*)parent()->parent();
  if ( tool->save == 1 )
    mw->viewManager()->activeView()->document()->save();
  else if ( tool->save == 2 )
    mw->actionCollection()->action("file_save_all")->trigger();

  KRun::runCommand( cmd, tool->tryexec, tool->icon );
#endif
}
//END KateExternalToolAction

//BEGIN KateExternalToolsMenuAction
KateExternalToolsMenuAction::KateExternalToolsMenuAction( const QString &text,
    QObject *parent,
    Kate::MainWindow *mw )
    : KActionMenu( text, parent ),
    mainwindow( mw )
{

  m_actionCollection = new KActionCollection( (QWidget*) mainwindow );

  // connect to view changed...
  connect(mw, SIGNAL(viewChanged()), this, SLOT(slotDocumentChanged()));

  reload();
}

void KateExternalToolsMenuAction::reload()
{
  m_actionCollection->clear ();
  menu()->clear();

  // load all the tools, and create a action for each of them
  KSharedConfig::Ptr pConfig = KSharedConfig::openConfig( "externaltools", KConfig::NoGlobals, "appdata" );
  KConfigGroup config(pConfig, "Global" );
  QStringList tools = config.readEntry( "tools", QStringList() );

  // if there are tools that are present but not explicitly removed,
  // add them to the end of the list
  pConfig->setReadDefaults( true );
  QStringList dtools = config.readEntry( "tools", QStringList() );
  int gver = config.readEntry( "version", 1 );
  pConfig->setReadDefaults( false );

  int ver = config.readEntry( "version", 0 );
  if ( ver <= gver )
  {
    QStringList removed = config.readEntry( "removed", QStringList() );
    bool sepadded = false;
    for (QStringList::iterator itg = dtools.begin(); itg != dtools.end(); ++itg )
    {
      if ( ! tools.contains( *itg ) &&
           ! removed.contains( *itg ) )
      {
        if ( ! sepadded )
        {
          tools << "---";
          sepadded = true;
        }
        tools << *itg;
      }
    }

    config.writeEntry( "tools", tools );
    config.sync();
    config.writeEntry( "version", gver );
  }

  for( QStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
    {
      menu()->addSeparator();
      // a separator
      continue;
    }

    config=KConfigGroup(pConfig, *it );

    KateExternalTool *t = new KateExternalTool(
                            config.readEntry( "name", "" ),
                            config.readEntry( "command", ""),
                            config.readEntry( "icon", ""),
                            config.readEntry( "executable", ""),
                            config.readEntry( "mimetypes", QStringList() ),
                            config.readEntry( "acname", "" ),
                            config.readEntry( "cmdname", "" ),
                            config.readEntry( "save", 0 ) );

    if ( t->hasexec )
    {
      KAction *a = new KateExternalToolAction( this, t );
      m_actionCollection->addAction( t->acname.toAscii(), a );
      addAction( a );
    }
    else
      delete t;
  }

  config=KConfigGroup(pConfig, "Shortcuts");
  m_actionCollection->readSettings( &config );
  slotDocumentChanged();
}

void KateExternalToolsMenuAction::slotDocumentChanged()
{
  // try to enable/disable to match current mime type
  KTextEditor::View *v = mainwindow->activeView();

  // no active view, oh oh
  if (!v)
    return;

  KTextEditor::Document *de = v->document();
  if ( de )
  {
    QString mt = de->mimeType();
    QStringList l;
    bool b;

    foreach (QAction* kaction, m_actionCollection->actions())
    {
      KateExternalToolAction *action = dynamic_cast<KateExternalToolAction*>(kaction);
      if ( action )
      {
        l = action->tool->mimetypes;
        b = ( ! l.count() || l.contains( mt ) );
        action->setEnabled( b );
      }
    }
  }
}
//END KateExternalToolsMenuAction

//BEGIN ToolItem
/**
 * This is a QListBoxItem, that has a KateExternalTool. The text is the Name
 * of the tool.
 */
class ToolItem : public QListWidgetItem
{
  public:
    ToolItem( QListWidget *lb, const QPixmap &icon, KateExternalTool *tool )
        : QListWidgetItem( icon , tool->name , lb ),
        tool ( tool )
    {
    }

    ~ToolItem()
    {}

    KateExternalTool *tool;
};
//END ToolItem

//BEGIN KateExternalToolServiceEditor
KateExternalToolServiceEditor::KateExternalToolServiceEditor( KateExternalTool *tool,
    QWidget *parent, const char *name )
    : KDialog( parent ),
    tool( tool )
{
  setCaption( i18n("Edit External Tool") );
  setButtons( Ok | Cancel );
  setObjectName( name );
  setModal( true );

  // create a entry for each property
  // fill in the values from the service if available
  QWidget *w = new QWidget( this );
  setMainWidget( w );
  QGridLayout *lo = new QGridLayout( w );
  lo->setSpacing( KDialog::spacingHint() );

  QLabel *l;

  leName = new QLineEdit( w );
  lo->addWidget( leName, 1, 2 );
  l = new QLabel( w );
  l->setBuddy( leName );
  l->setText( i18n("&Label:") );
  l->setAlignment( l->alignment() | Qt::AlignRight );
  lo->addWidget( l, 1, 1 );
  if ( tool ) leName->setText( tool->name );
  leName->setWhatsThis(i18n(
                         "The name will be displayed in the 'Tools->External' menu") );

  btnIcon = new KIconButton( w );
  btnIcon->setIconSize( KIconLoader::SizeSmall );
  lo->addWidget( btnIcon, 1, 3 );
  if ( tool && !tool->icon.isEmpty() )
    btnIcon->setIcon( tool->icon );

  teCommand = new QTextEdit( w );
  lo->addWidget( teCommand, 2, 2, 1, 2 );
  l = new QLabel( w );
  l->setBuddy( teCommand );
  l->setText( i18n("S&cript:") );
  l->setAlignment( Qt::AlignTop | Qt::AlignRight );
  lo->addWidget( l, 2, 1 );
  if ( tool ) teCommand->setText( tool->command );
  teCommand->setWhatsThis(i18n(
                            "<p>The script to execute to invoke the tool. The script is passed "
                            "to /bin/sh for execution. The following macros "
                            "will be expanded:</p>"
                            "<ul><li><code>%URL</code> - the URL of the current document.</li>"
                            "<li><code>%URLs</code> - a list of the URLs of all open documents.</li>"
                            "<li><code>%directory</code> - the URL of the directory containing "
                            "the current document.</li>"
                            "<li><code>%filename</code> - the filename of the current document.</li>"
                            "<li><code>%line</code> - the current line of the text cursor in the "
                            "current view.</li>"
                            "<li><code>%column</code> - the column of the text cursor in the "
                            "current view.</li>"
                            "<li><code>%selection</code> - the selected text in the current view.</li>"
                            "<li><code>%text</code> - the text of the current document.</li></ul>" ) );


  leExecutable = new QLineEdit( w );
  lo->addWidget( leExecutable, 3, 2, 1, 2 );
  l = new QLabel( w );
  l->setBuddy( leExecutable );
  l->setText( i18n("&Executable:") );
  l->setAlignment( l->alignment() | Qt::AlignRight );
  lo->addWidget( l, 3, 1 );
  if ( tool ) leExecutable->setText( tool->tryexec );
  leExecutable->setWhatsThis(i18n(
                               "The executable used by the command. This is used to check if a tool "
                               "should be displayed; if not set, the first word of <em>command</em> "
                               "will be used.") );

  leMimetypes = new QLineEdit( w );
  lo->addWidget( leMimetypes, 4, 2 );
  l = new QLabel( w );
  l->setBuddy( leMimetypes );
  l->setText( i18n("&Mime types:") );
  l->setAlignment( l->alignment() | Qt::AlignRight );
  lo->addWidget( l, 4, 1 );
  if ( tool ) leMimetypes->setText( tool->mimetypes.join("; ") );
  leMimetypes->setWhatsThis(i18n(
                              "A semicolon-separated list of mime types for which this tool should "
                              "be available; if this is left empty, the tool is always available. "
                              "To choose from known mimetypes, press the button on the right.") );

  QToolButton *btnMTW = new QToolButton(w);
  lo->addWidget( btnMTW, 4, 3 );
  btnMTW->setIcon(QIcon(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));
  btnMTW->setWhatsThis(i18n(
                         "Click for a dialog that can help you creating a list of mimetypes.") );

  cmbSave = new QComboBox(w);
  lo->addWidget( cmbSave, 5, 2, 1, 2 );
  l = new QLabel( w );
  l->setBuddy( cmbSave );
  l->setText( i18n("&Save:") );
  l->setAlignment( l->alignment() | Qt::AlignRight );
  lo->addWidget( l, 5, 1 );
  QStringList sl;
  sl << i18n("None") << i18n("Current Document") << i18n("All Documents");
  cmbSave->addItems( sl );
  if ( tool ) cmbSave->setCurrentIndex( tool->save );
  cmbSave->setWhatsThis(i18n(
                          "You can elect to save the current or all [modified] documents prior to "
                          "running the command. This is helpful if you want to pass URLs to "
                          "an application like, for example, an FTP client.") );


  leCmdLine = new QLineEdit( w );
  lo->addWidget( leCmdLine, 6, 2, 1, 2 );
  l = new QLabel( i18n("&Command line name:"), w );
  l->setBuddy( leCmdLine );
  l->setAlignment( l->alignment() | Qt::AlignRight );
  lo->addWidget( l, 6, 1 );
  if ( tool ) leCmdLine->setText( tool->cmdname );
  leCmdLine->setWhatsThis(i18n(
                            "If you specify a name here, you can invoke the command from the view "
                            "command lines with exttool-the_name_you_specified_here. "
                            "Please do not use spaces or tabs in the name."));
  connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
}

void KateExternalToolServiceEditor::slotOk()
{
  if ( leName->text().isEmpty() ||
       teCommand->text().isEmpty() )
  {
    KMessageBox::information( this, i18n("You must specify at least a name and a command") );
    return;
  }

  KDialog::accept();
}

void KateExternalToolServiceEditor::showMTDlg()
{
  QString text = i18n("Select the MimeTypes for which to enable this tool.");
  QStringList list = leMimetypes->text().split( QRegExp("\\s*;\\s*"), QString::SkipEmptyParts );
  KMimeTypeChooserDialog d( i18n("Select Mime Types"), text, list, "text", this );
  if ( d.exec() == KDialog::Accepted )
  {
    leMimetypes->setText( d.chooser()->mimeTypes().join(";") );
  }
}
//END KateExternalToolServiceEditor

//BEGIN KateExternalToolsConfigWidget
KateExternalToolsConfigWidget::KateExternalToolsConfigWidget( QWidget *parent, const char* )
    : KTextEditor::ConfigPage( parent ),
    m_changed( false )
{
  QGridLayout *lo = new QGridLayout( this );
  lo->setMargin( 0 );
  lo->setSpacing( KDialog::spacingHint() );

  lbTools = new KListWidget( this );
  lo->addWidget( lbTools, 1, 0, 4, 4 );
  connect( lbTools, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()) );

  btnNew = new QPushButton( i18n("&New..."), this );
  lo->addWidget( btnNew, 5, 0 );
  connect( btnNew, SIGNAL(clicked()), this, SLOT(slotNew()) );

  btnRemove = new QPushButton( i18n("&Remove"), this );
  lo->addWidget( btnRemove, 5, 2 );
  connect( btnRemove, SIGNAL(clicked()), this, SLOT(slotRemove()) );

  btnEdit = new QPushButton( i18n("&Edit..."), this );
  lo->addWidget( btnEdit, 5, 1 );
  connect( btnEdit, SIGNAL(clicked()), this, SLOT(slotEdit()) );

  QPushButton *b = new QPushButton( i18n("Insert &Separator"), this );
  lo->addWidget( b, 5, 3 );
  connect( b, SIGNAL(clicked()), this, SLOT(slotInsertSeparator()) );

  btnMoveUp = new QPushButton( KIcon("go-up"), "", this );
  lo->addWidget( btnMoveUp, 2, 4 );
  connect( btnMoveUp, SIGNAL(clicked()), this, SLOT(slotMoveUp()) );

  btnMoveDwn = new QPushButton( KIcon("go-down"), "", this );
  lo->addWidget( btnMoveDwn, 3, 4 );
  connect( btnMoveDwn, SIGNAL(clicked()), this, SLOT(slotMoveDown()) );

  connect( lbTools, SIGNAL( doubleClicked ( QListWidgetItem * ) ), this, SLOT( slotEdit() ) );

  lo->setRowStretch( 1, 1 );
  lo->setRowStretch( 4, 1 );
  lo->setColumnStretch( 0, 1 );
  lo->setColumnStretch( 1, 1 );
  lo->setColumnStretch( 2, 1 );


  lbTools->setWhatsThis(i18n(
                          "This list shows all the configured tools, represented by their menu text.") );

  config = new KConfig( "externaltools", KConfig::NoGlobals, "appdata" );
  reset();
  slotSelectionChanged();
}

KateExternalToolsConfigWidget::~KateExternalToolsConfigWidget()
{
  delete config;
}

void KateExternalToolsConfigWidget::reset()
{
  //m_tools.clear();
  lbTools->clear();

  // load the files from a KConfig
  QStringList tools = config->group("Global").readEntry("tools", QStringList());

  for( QStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
    {
      new QListWidgetItem( "---" , lbTools);
    }
    else
    {
      KConfigGroup cg( config, *it );

      KateExternalTool *t = new KateExternalTool(
                              cg.readEntry( "name", "" ),
                              cg.readEntry( "command", ""),
                              cg.readEntry( "icon", ""),
                              cg.readEntry( "executable", ""),
                              cg.readEntry( "mimetypes", QStringList() ),
                              cg.readEntry( "acname" ),
                              cg.readEntry( "cmdname"),
                              cg.readEntry( "save", 0 ) );

      if ( t->hasexec ) // we only show tools that are also in the menu.
        new ToolItem( lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t );
      else
        delete t;
    }
  }
  m_changed = false;
}

QPixmap KateExternalToolsConfigWidget::blankIcon()
{
  QPixmap pm( KIconLoader::SizeSmall, KIconLoader::SizeSmall );
  pm.fill();
  pm.setMask( pm.createHeuristicMask() );
  return pm;
}

void KateExternalToolsConfigWidget::apply()
{
  if ( ! m_changed )
    return;
  m_changed = false;

  // save a new list
  // save each item
  QStringList tools;
  for ( int i = 0; i < lbTools->count(); i++ )
  {
    if ( lbTools->item(i)->text() == "---" )
    {
      tools << "---";
      continue;
    }
    KateExternalTool *t = ((ToolItem*)lbTools->item( i ))->tool;
//     kDebug(13001)<<"adding tool: "<<t->name;
    tools << t->acname;

    KConfigGroup cg( config, t->acname );

    cg.writeEntry( "name", t->name );
    cg.writeEntry( "command", t->command );
    cg.writeEntry( "icon", t->icon );
    cg.writeEntry( "executable", t->tryexec );
    cg.writeEntry( "mimetypes", t->mimetypes );
    cg.writeEntry( "acname", t->acname );
    cg.writeEntry( "cmdname", t->cmdname );
    cg.writeEntry( "save", t->save );
  }

  config->group("Global").writeEntry( "tools", tools );

  // if any tools was removed, try to delete their groups, and
  // add the group names to the list of removed items.
  if ( m_removed.count() )
  {
    for ( QStringList::iterator it = m_removed.begin(); it != m_removed.end(); ++it )
    {
      if ( config->hasGroup( *it ) )
        config->deleteGroup( *it  );
    }
    QStringList removed = config->group("Global").readEntry( "removed", QStringList() );
    removed += m_removed;

    // clean up the list of removed items, so that it does not contain
    // non-existing groups (we can't remove groups from a non-owned global file).
    config->sync();
    QStringList::iterator it1 = removed.begin();
    while ( it1 != removed.end() )
    {
      if ( ! config->hasGroup( *it1 ) )
        it1 = removed.erase( it1 );
      else
        ++it1;
    }
    config->group("Global").writeEntry( "removed", removed );
  }

  config->sync();
}

void KateExternalToolsConfigWidget::slotSelectionChanged()
{
  // update button state
  bool hs =  lbTools->currentItem() != 0;
  btnEdit->setEnabled( hs && dynamic_cast<ToolItem*>(lbTools->currentItem()) );
  btnRemove->setEnabled( hs );
  btnMoveUp->setEnabled( ( lbTools->currentRow() > 0 ) && hs );
  btnMoveDwn->setEnabled( ( lbTools->currentRow() < (int)lbTools->count() - 1 ) && hs );
}

void KateExternalToolsConfigWidget::slotNew()
{
  // display a editor, and if it is OK'd, create a new tool and
  // create a listbox item for it
  KateExternalToolServiceEditor editor( 0, this );

  if ( editor.exec() )
  {
    KateExternalTool *t = new KateExternalTool(
                            editor.leName->text(),
                            editor.teCommand->text(),
                            editor.btnIcon->icon(),
                            editor.leExecutable->text(),
                            editor.leMimetypes->text().split( QRegExp("\\s*;\\s*"), QString::SkipEmptyParts ) );

    // This is sticky, it does not change again, so that shortcuts sticks
    // TODO check for dups
    t->acname = "externaltool_" + QString(t->name).replace( QRegExp("\\W+"), "" );

    new ToolItem ( lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t );

    emit changed();
    m_changed = true;
  }
}

void KateExternalToolsConfigWidget::slotRemove()
{
  // add the tool action name to a list of removed items,
  // remove the current listbox item
  if ( lbTools->currentRow() > -1 )
  {
    ToolItem *i = dynamic_cast<ToolItem*>(lbTools->currentItem());
    if ( i )
      m_removed << i->tool->acname;

    delete lbTools->takeItem( lbTools->currentRow() );
    emit changed();
    m_changed = true;
  }
}

void KateExternalToolsConfigWidget::slotEdit()
{
  if( !dynamic_cast<ToolItem*>(lbTools->currentItem()) ) return;
  // show the item in an editor
  KateExternalTool *t = ((ToolItem*)lbTools->currentItem())->tool;
  KateExternalToolServiceEditor editor( t, this);
  editor.resize( config->group("Editor").readEntry( "Size", QSize() ) );
  if ( editor.exec() /*== KDialog::Ok*/ )
  {

    bool elementChanged = ( ( editor.btnIcon->icon() != t->icon ) || (editor.leName->text() != t->name ) ) ;

    t->name = editor.leName->text();
    t->cmdname = editor.leCmdLine->text();
    t->command = editor.teCommand->text();
    t->icon = editor.btnIcon->icon();
    t->tryexec = editor.leExecutable->text();
    t->mimetypes = editor.leMimetypes->text().split( QRegExp("\\s*;\\s*"), QString::SkipEmptyParts );
    t->save = editor.cmbSave->currentIndex();

    //if the icon has changed or name changed, I have to renew the listbox item :S
    if ( elementChanged )
    {
      int idx = lbTools->row( lbTools->currentItem() );
      delete lbTools->takeItem( idx );
      lbTools->insertItem( idx , new ToolItem( 0, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t ));
    }

    emit changed();
    m_changed = true;
  }

  config->group("Editor").writeEntry( "Size", editor.size() );
  config->sync();
}

void KateExternalToolsConfigWidget::slotInsertSeparator()
{
  lbTools->insertItem( lbTools->currentRow() + 1 , "---" );
  emit changed();
  m_changed = true;
}

void KateExternalToolsConfigWidget::slotMoveUp()
{
  // move the current item in the listbox upwards if possible
  QListWidgetItem *item = lbTools->currentItem();
  if ( ! item ) return;

  int idx = lbTools->row( item );

  if ( idx < 1 ) return;

  if ( dynamic_cast<ToolItem*>(item) )
  {
    KateExternalTool *tool = ((ToolItem*)item)->tool;
    delete lbTools->takeItem( idx );
    lbTools->insertItem( idx - 1 , new ToolItem( 0, tool->icon.isEmpty() ? blankIcon() : SmallIcon( tool->icon ), tool ) );
  }
  else // a separator!
  {
    delete lbTools->takeItem( idx );
    lbTools->insertItem( idx - 1 , new QListWidgetItem( "---"  ));
  }

  lbTools->setCurrentRow( idx - 1 );
  slotSelectionChanged();
  emit changed();
  m_changed = true;
}

void KateExternalToolsConfigWidget::slotMoveDown()
{
  // move the current item in the listbox downwards if possible
  QListWidgetItem *item = lbTools->currentItem();
  if ( ! item ) return;

  int idx = lbTools->row( item );

  if ( idx > lbTools->count() - 1 ) return;

  if ( dynamic_cast<ToolItem*>(item) )
  {
    KateExternalTool *tool = ((ToolItem*)item)->tool;
    delete lbTools->takeItem( idx );
    lbTools->insertItem( idx + 1 , new ToolItem( 0, tool->icon.isEmpty() ? blankIcon() : SmallIcon( tool->icon ), tool ) );
  }
  else // a separator!
  {
    delete lbTools->takeItem( idx );
    lbTools->insertItem( idx + 1 , new QListWidgetItem( "---" ) );
  }

  lbTools->setCurrentRow( idx + 1 );
  slotSelectionChanged();
  emit changed();
  m_changed = true;
}
//END KateExternalToolsConfigWidget
// kate: space-indent on; indent-width 2; replace-tabs on;
