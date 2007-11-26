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

#include "kwritemain.h"
#include "kwritemain.moc"
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/editorchooser.h>

#include <kio/netaccess.h>

#include <kaboutapplicationdialog.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdeversion.h>
#include <kdiroperator.h>
#include <kedittoolbar.h>
#include <kencodingfiledialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krecentfilesaction.h>
#include <kshortcutsdialog.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <ksqueezedtextlabel.h>
#include <kstringhandler.h>
#include <kxmlguifactory.h>

#include <QtCore/QTimer>
#include <QtCore/QTextCodec>

QList<KTextEditor::Document*> KWrite::docList;
QList<KWrite*> KWrite::winList;

KWrite::KWrite (KTextEditor::Document *doc)
    : m_view(0),
      m_recentFiles(0),
      m_paShowPath(0),
      m_paShowStatusBar(0)
{
  if ( !doc )
  {
    KTextEditor::Editor *editor = KTextEditor::EditorChooser::editor();

    if ( !editor )
    {
      KMessageBox::error(this, i18n("A KDE text-editor component could not be found;\n"
                                    "please check your KDE installation."));
      kapp->exit(1);
    }

    doc = editor->createDocument(0);

    // enable the modified on disk warning dialogs if any
    if (qobject_cast<KTextEditor::ModificationInterface *>(doc))
      qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning (true);

    docList.append(doc);
  }

  m_view = qobject_cast<KTextEditor::View*>(doc->createView (this));

  // WORKAROUND: both lines are required; 1st one for about dialog and 2nd one for window
  QApplication::setWindowIcon(KIcon("accessories-text-editor"));
  setWindowIcon(KIcon("accessories-text-editor"));

  setCentralWidget(m_view);

  setupActions();
  setupStatusBar();

  // signals for the statusbar
  connect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View *, const KTextEditor::Cursor &)), this, SLOT(cursorPositionChanged(KTextEditor::View *)));
  connect(m_view, SIGNAL(viewModeChanged(KTextEditor::View *)), this, SLOT(viewModeChanged(KTextEditor::View *)));
  connect(m_view, SIGNAL(selectionChanged (KTextEditor::View *)), this, SLOT(selectionChanged (KTextEditor::View *)));
  connect(m_view, SIGNAL(informationMessage (KTextEditor::View *, const QString &)), this, SLOT(informationMessage (KTextEditor::View *, const QString &)));
  connect(m_view->document(), SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(modifiedChanged()));
  connect(m_view->document(), SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(modifiedChanged()) );
  connect(m_view->document(), SIGNAL(documentNameChanged(KTextEditor::Document *)), this, SLOT(documentNameChanged()));
  connect(m_view->document(),SIGNAL(documentUrlChanged(KTextEditor::Document *)), this, SLOT(urlChanged()));
  connect(m_view->document(), SIGNAL(modeChanged(KTextEditor::Document *)), this, SLOT(modeChanged(KTextEditor::Document *)));

  setAcceptDrops(true);
  connect(m_view,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(slotDropEvent(QDropEvent *)));

  setXMLFile( "kwriteui.rc" );
  createShellGUI( true );
  guiFactory()->addClient( m_view );

  // install a working kate part popup dialog thingy
  m_view->setContextMenu ((QMenu*)(factory()->container("ktexteditor_popup", this)) );

  // init with more useful size, stolen from konq :)
  if (!initialGeometrySet())
    resize( QSize(700, 480).expandedTo(minimumSizeHint()));

  // call it as last thing, must be sure everything is already set up ;)
  setAutoSaveSettings ();

  readConfig ();

  winList.append (this);

  updateStatus ();
  show ();
}

KWrite::~KWrite()
{
  winList.removeAll(this);

  if (m_view->document()->views().count() == 1)
  {
    docList.removeAll(m_view->document());
    delete m_view->document();
  }

  KGlobal::config()->sync ();
}

void KWrite::setupActions()
{
  actionCollection()->addAction( KStandardAction::Close, "file_close", this, SLOT(slotFlush()) )
    ->setWhatsThis(i18n("Use this to close the current document"));

  // setup File menu
  actionCollection()->addAction( KStandardAction::New, "file_new", this, SLOT(slotNew()) )
    ->setWhatsThis(i18n("Use this command to create a new document"));
  actionCollection()->addAction( KStandardAction::Open, "file_open", this, SLOT(slotOpen()) )
    ->setWhatsThis(i18n("Use this command to open an existing document for editing"));

  m_recentFiles = KStandardAction::openRecent(this, SLOT(slotOpen(const KUrl&)), this);
  actionCollection()->addAction(m_recentFiles->objectName(), m_recentFiles);
  m_recentFiles->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  QAction *a = actionCollection()->addAction( "view_new_view" );
  a->setIcon( KIcon("window-new") );
  a->setText( i18n("&New Window") );
  connect( a, SIGNAL(triggered()), this, SLOT(newView()) );
  a->setWhatsThis(i18n("Create another view containing the current document"));

  a = actionCollection()->addAction( "settings_choose_editor" );
  a->setText( i18n("Choose Editor...") );
  connect( a, SIGNAL(triggered()), this, SLOT(changeEditor()) );
  a->setWhatsThis(i18n("Override the system wide setting for the default editing component"));

  actionCollection()->addAction( KStandardAction::Quit, this, SLOT(close()) )
    ->setWhatsThis(i18n("Close the current document view"));

  // setup Settings menu
  setStandardToolBarMenuEnabled(true);

  m_paShowStatusBar = KStandardAction::showStatusbar(this, SLOT(toggleStatusBar()), this);
  actionCollection()->addAction( "settings_show_statusbar", m_paShowStatusBar);
  m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));

  m_paShowPath = new KToggleAction( i18n("Sho&w Path"), this );
  actionCollection()->addAction( "set_showPath", m_paShowPath );
  connect( m_paShowPath, SIGNAL(triggered()), this, SLOT(documentNameChanged()) );
  m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));

  a= actionCollection()->addAction( KStandardAction::KeyBindings, this, SLOT(editKeys()) );
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a = actionCollection()->addAction( KStandardAction::ConfigureToolbars, "options_configure_toolbars",
                                     this, SLOT(editToolbars()) );
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  a = actionCollection()->addAction( "help_about_editor" );
  a->setText( i18n("&About Editor Component") );
  connect( a, SIGNAL(triggered()), this, SLOT(aboutEditor()) );

}

void KWrite::setupStatusBar()
{
  // statusbar stuff
  m_lineColLabel = new QLabel( statusBar() );
  statusBar()->addWidget( m_lineColLabel, 0 );
  m_lineColLabel->setAlignment( Qt::AlignCenter );

  m_modifiedLabel = new QLabel( QString("   "), statusBar() );
  statusBar()->addWidget( m_modifiedLabel, 0 );
  m_modifiedLabel->setAlignment( Qt::AlignCenter );

  m_insertModeLabel = new QLabel( i18n(" INS "), statusBar() );
  statusBar()->addWidget( m_insertModeLabel, 0 );
  m_insertModeLabel->setAlignment( Qt::AlignCenter );

  m_selectModeLabel = new QLabel( i18n(" LINE "), statusBar() );
  statusBar()->addWidget( m_selectModeLabel, 0 );
  m_selectModeLabel->setAlignment( Qt::AlignCenter );

  m_modeLabel = new QLabel( QString(), statusBar() );
  statusBar()->addWidget( m_modeLabel, 0 );
  m_modeLabel->setAlignment( Qt::AlignCenter );

  m_fileNameLabel=new KSqueezedTextLabel( statusBar() );
  statusBar()->addPermanentWidget( m_fileNameLabel, 1 );
  m_fileNameLabel->setMinimumSize( 0, 0 );
  m_fileNameLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
  m_fileNameLabel->setAlignment( /*Qt::AlignRight*/Qt::AlignLeft );

  m_modPm = SmallIcon("modified");
  m_modDiscPm = SmallIcon("modonhd");
  m_modmodPm = SmallIcon("modmod");
  m_noPm = SmallIcon("null");
}

// load on url
void KWrite::loadURL(const KUrl &url)
{
  m_view->document()->openUrl(url);
}

// is closing the window wanted by user ?
bool KWrite::queryClose()
{
  if (m_view->document()->views().count() > 1)
    return true;

  if (m_view->document()->queryClose())
  {
    writeConfig();

    return true;
  }

  return false;
}

void KWrite::changeEditor()
{
  KWriteEditorChooser choose(this);
  choose.exec();
}

void KWrite::slotFlush ()
{
   m_view->document()->closeUrl();
}

void KWrite::slotNew()
{
  new KWrite();
}

void KWrite::slotOpen()
{
	KEncodingFileDialog::Result r=KEncodingFileDialog::getOpenUrlsAndEncoding(m_view->document()->encoding(), m_view->document()->url().url(),QString(),this,i18n("Open File"));

  for (KUrl::List::Iterator i=r.URLs.begin(); i != r.URLs.end(); ++i)
  {
    encoding = r.encoding;
    slotOpen ( *i );
  }
}

void KWrite::slotOpen( const KUrl& url )
{
  if (url.isEmpty()) return;

  if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, this))
  {
    KMessageBox::error (this, i18n("The given file could not be read, check if it exists or if it is readable for the current user."));
    return;
  }

  if (m_view->document()->isModified() || !m_view->document()->url().isEmpty())
  {
    KWrite *t = new KWrite();
    t->m_view->document()->setEncoding(encoding);
    t->loadURL(url);
  }
  else
  {
    m_view->document()->setEncoding(encoding);
    loadURL(url);
  }
}

void KWrite::urlChanged()
{
  if ( ! m_view->document()->url().isEmpty() )
    m_recentFiles->addUrl( m_view->document()->url() );
}

void KWrite::newView()
{
  new KWrite(m_view->document());
}

void KWrite::toggleStatusBar()
{
  if( m_paShowStatusBar->isChecked() )
    statusBar()->show();
  else
    statusBar()->hide();
}

void KWrite::editKeys()
{
  KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
  dlg.addCollection(actionCollection());
  if( m_view )
    dlg.addCollection(m_view->actionCollection());
  dlg.configure();
}

void KWrite::editToolbars()
{
  saveMainWindowSettings( KGlobal::config()->group( "MainWindow" ) );
  KEditToolBar dlg(guiFactory(), this);

  connect( &dlg, SIGNAL(newToolBarConfig()), this, SLOT(slotNewToolbarConfig()) );
  dlg.exec();
}

void KWrite::slotNewToolbarConfig()
{
    applyMainWindowSettings( KGlobal::config()->group( "MainWindow" ) );
}

void KWrite::dragEnterEvent( QDragEnterEvent *event )
{
  KUrl::List uriList = KUrl::List::fromMimeData( event->mimeData() );
  if(!uriList.isEmpty())
      event->accept();
}

void KWrite::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}

void KWrite::slotDropEvent( QDropEvent *event )
{
  KUrl::List textlist = KUrl::List::fromMimeData( event->mimeData() );

  if (textlist.isEmpty())
    return;

  for (KUrl::List::Iterator i=textlist.begin(); i != textlist.end(); ++i)
    slotOpen (*i);
}

void KWrite::slotEnableActions( bool enable )
{
  QList<QAction *> actions = actionCollection()->actions();
  QList<QAction *>::ConstIterator it = actions.begin();
  QList<QAction *>::ConstIterator end = actions.end();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );

  actions = m_view->actionCollection()->actions();
  it = actions.begin();
  end = actions.end();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );
}

//common config
void KWrite::readConfig(KSharedConfigPtr config)
{
  KConfigGroup cfg( config, "General Options");

  m_paShowStatusBar->setChecked( cfg.readEntry("ShowStatusBar", false) );
  m_paShowPath->setChecked( cfg.readEntry("ShowPath", false) );

  m_recentFiles->loadEntries( config->group( "Recent Files" ));

  m_view->document()->editor()->readConfig(config.data());

  if( m_paShowStatusBar->isChecked() )
    statusBar()->show();
  else
    statusBar()->hide();
}

void KWrite::writeConfig(KSharedConfigPtr config)
{
  KConfigGroup generalOptions( config, "General Options");

  generalOptions.writeEntry("ShowStatusBar",m_paShowStatusBar->isChecked());
  generalOptions.writeEntry("ShowPath",m_paShowPath->isChecked());

  m_recentFiles->saveEntries(KConfigGroup(config, "Recent Files"));

  // Writes into its own group
  m_view->document()->editor()->writeConfig(config.data());

  config->sync ();
}

//config file
void KWrite::readConfig()
{
  readConfig(KGlobal::config());
}

void KWrite::writeConfig()
{
  writeConfig(KGlobal::config());
}

// session management
void KWrite::restore(KConfig *config, int n)
{
  readPropertiesInternal(config, n);
}

void KWrite::readProperties(KSharedConfigPtr config)
{
  readConfig(config);

  if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(m_view))
    iface->readSessionConfig(KConfigGroup(config, "General Options"));
}

void KWrite::saveProperties(KSharedConfigPtr config)
{
  writeConfig(config);

  KConfigGroup group(config, QString());
  group.writeEntry("DocumentNumber",docList.indexOf(m_view->document()) + 1);

  if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(m_view)) {
    KConfigGroup cg( config, "General Options" );
    iface->writeSessionConfig(cg);
  }
}

void KWrite::saveGlobalProperties(KConfig *config) //save documents
{
  config->group("Number").writeEntry("NumberOfDocuments",docList.count());

  for (int z = 1; z <= docList.count(); z++)
  {
     QString buf = QString("Document %1").arg(z);
     KConfigGroup cg( config, buf );
     KTextEditor::Document *doc = docList.at(z - 1);

     if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
       iface->writeSessionConfig(cg);
  }

  for (int z = 1; z <= winList.count(); z++)
  {
     QString buf = QString("Window %1").arg(z);
     KConfigGroup cg( config, buf );
     cg.writeEntry("DocumentNumber",docList.indexOf(winList.at(z-1)->view()->document()) + 1);
  }
}

//restore session
void KWrite::restore()
{
  KConfig *config = kapp->sessionConfig();

  if (!config)
    return;

  KTextEditor::Editor *editor = KTextEditor::EditorChooser::editor();

  if ( !editor )
  {
    KMessageBox::error(0, i18n("A KDE text-editor component could not be found;\n"
                                  "please check your KDE installation."));
    kapp->exit(1);
  }

  int docs, windows;
  QString buf;
  KTextEditor::Document *doc;
  KWrite *t;

  KConfigGroup numberConfig(config, "Number");
  docs = numberConfig.readEntry("NumberOfDocuments", 0);
  windows = numberConfig.readEntry("NumberOfWindows", 0);

  for (int z = 1; z <= docs; z++)
  {
     buf = QString("Document %1").arg(z);
     KConfigGroup cg(config, buf);
     doc=editor->createDocument(0);

     if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
       iface->readSessionConfig(cg);
     docList.append(doc);
  }

  for (int z = 1; z <= windows; z++)
  {
    buf = QString("Window %1").arg(z);
    KConfigGroup cg(config, buf);
    t = new KWrite(docList.at(cg.readEntry("DocumentNumber", 0) - 1));
    t->restore(config,z);
  }
}

void KWrite::aboutEditor()
{
  KAboutApplicationDialog dlg(m_view->document()->editor()->aboutData(), this);
  dlg.exec();
}

void KWrite::updateStatus ()
{
  viewModeChanged (m_view);
  cursorPositionChanged (m_view);
  selectionChanged (m_view);
  modifiedChanged ();
  documentNameChanged ();
  modeChanged (m_view->document());
}

void KWrite::viewModeChanged ( KTextEditor::View *view )
{
  m_insertModeLabel->setText( view->viewMode() );
}

void KWrite::cursorPositionChanged ( KTextEditor::View *view )
{
  KTextEditor::Cursor position (view->cursorPositionVirtual());

  m_lineColLabel->setText(
    i18n(" Line: %1 Col: %2 ", KGlobal::locale()->formatNumber(position.line()+1, 0),
                               KGlobal::locale()->formatNumber(position.column()+1, 0)) );
}

void KWrite::selectionChanged (KTextEditor::View *view)
{
  m_selectModeLabel->setText( view->blockSelection() ? i18n(" BLOCK ") : i18n(" LINE ") );
}

void KWrite::informationMessage (KTextEditor::View *view, const QString &message)
{
  Q_UNUSED(view)

  m_fileNameLabel->setText( message );

  // timer to reset this after 4 seconds
  QTimer::singleShot(4000, this, SLOT(documentNameChanged()));
}

void KWrite::modifiedChanged()
{
    bool mod = m_view->document()->isModified();

   /* const KateDocumentInfo *info
      = KateDocManager::self()->documentInfo ( m_view->document() );
*/
//    bool modOnHD = false; //info && info->modifiedOnDisc;

    m_modifiedLabel->setPixmap(
        mod ? m_modPm : m_noPm
          /*info && modOnHD ?
            m_modmodPm :
            m_modPm :
          info && modOnHD ?
            m_modDiscPm :
        m_noPm*/
        );

    documentNameChanged(); // update the modified flag in window title
}

void KWrite::documentNameChanged ()
{
  m_fileNameLabel->setText( KStringHandler::lsqueeze(m_view->document()->documentName (), 64) );

  if (m_view->document()->url().isEmpty()) {
    setCaption(i18n("Untitled"),m_view->document()->isModified());
  }
  else
  {
    QString c;
    if (!m_paShowPath->isChecked())
    {
      c = m_view->document()->url().fileName();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = c.left(64) + "...";
    }
    else
    {
      c = m_view->document()->url().prettyUrl();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = "..." + c.right(64);
    }

    setCaption (c, m_view->document()->isModified());
  }
}

void KWrite::modeChanged ( KTextEditor::Document *document )
{
  m_modeLabel->setText (document->mode());
}

extern "C" KDE_EXPORT int kdemain(int argc, char **argv)
{
  KAboutData aboutData ( "kwrite", "kate",
                         ki18n("KWrite"),
                         KDE_VERSION_STRING,
                         ki18n( "KWrite - Text Editor" ), KAboutData::License_LGPL_V2,
                         ki18n( "(c) 2000-2005 The Kate Authors" ), KLocalizedString(), "http://www.kate-editor.org" );
  aboutData.addAuthor (ki18n("Christoph Cullmann"), ki18n("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
  aboutData.addAuthor (ki18n("Anders Lund"), ki18n("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor (ki18n("Joseph Wenninger"), ki18n("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  aboutData.addAuthor (ki18n("Hamish Rodda"),ki18n("Core Developer"), "rodda@kde.org");
  aboutData.addAuthor (ki18n("Dominik Haumann"), ki18n("Developer & Highlight wizard"), "dhdev@gmx.de");
  aboutData.addAuthor (ki18n("Waldo Bastian"), ki18n( "The cool buffersystem" ), "bastian@kde.org" );
  aboutData.addAuthor (ki18n("Charles Samuels"), ki18n("The Editing Commands"), "charles@kde.org");
  aboutData.addAuthor (ki18n("Matt Newell"), ki18n("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor (ki18n("Michael Bartl"), ki18n("Former Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor (ki18n("Michael McCallum"), ki18n("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor (ki18n("Jochen Wilhemly"), ki18n( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor (ki18n("Michael Koch"),ki18n("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor (ki18n("Christian Gebauer"), KLocalizedString(), "gebauer@kde.org" );
  aboutData.addAuthor (ki18n("Simon Hausmann"), KLocalizedString(), "hausmann@kde.org" );
  aboutData.addAuthor (ki18n("Glen Parker"),ki18n("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor (ki18n("Scott Manson"),ki18n("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  aboutData.addAuthor (ki18n("John Firebaugh"),ki18n("Patches and more"), "jfirebaugh@kde.org");

  aboutData.addCredit (ki18n("Matteo Merli"),ki18n("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  aboutData.addCredit (ki18n("Rocky Scaletta"),ki18n("Highlighting for VHDL"), "rocky@purdue.edu");
  aboutData.addCredit (ki18n("Yury Lebedev"),ki18n("Highlighting for SQL"));
  aboutData.addCredit (ki18n("Chris Ross"),ki18n("Highlighting for Ferite"));
  aboutData.addCredit (ki18n("Nick Roux"),ki18n("Highlighting for ILERPG"));
  aboutData.addCredit (ki18n("Carsten Niehaus"), ki18n("Highlighting for LaTeX"));
  aboutData.addCredit (ki18n("Per Wigren"), ki18n("Highlighting for Makefiles, Python"));
  aboutData.addCredit (ki18n("Jan Fritz"), ki18n("Highlighting for Python"));
  aboutData.addCredit (ki18n("Daniel Naber"));
  aboutData.addCredit (ki18n("Roland Pabel"),ki18n("Highlighting for Scheme"));
  aboutData.addCredit (ki18n("Cristi Dumitrescu"),ki18n("PHP Keyword/Datatype list"));
  aboutData.addCredit (ki18n("Carsten Pfeiffer"), ki18n("Very nice help"));
  aboutData.addCredit (ki18n("All people who have contributed and I have forgotten to mention"));

  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineOptions options;
  options.add("stdin", ki18n("Read the contents of stdin"));
  options.add("encoding <argument>", ki18n("Set encoding for the file to open"));
  options.add("line <argument>", ki18n("Navigate to this line"));
  options.add("column <argument>", ki18n("Navigate to this column"));
  options.add("+[URL]", ki18n("Document to open"));
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication a;

  KGlobal::locale()->insertCatalog("katepart4");
#if 0
  DCOPClient *client = kapp->dcopClient();
  if (!client->isRegistered())
  {
    client->attach();
    client->registerAs("kwrite");
  }
#endif
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (a.isSessionRestored())
  {
    KWrite::restore();
  }
  else
  {
    bool nav = false;
    int line = 0, column = 0;

    QTextCodec *codec = args->isSet("encoding") ? QTextCodec::codecForName(args->getOption("encoding").toLocal8Bit()) : 0;

    if (args->isSet ("line"))
    {
      line = args->getOption ("line").toInt();
      nav = true;
    }

    if (args->isSet ("column"))
    {
      column = args->getOption ("column").toInt();
      nav = true;
    }

    if ( args->count() == 0 )
    {
        KWrite *t = new KWrite;

        if( args->isSet( "stdin" ) )
        {
          QTextStream input(stdin, QIODevice::ReadOnly);

          // set chosen codec
          if (codec)
            input.setCodec (codec);

          QString line;
          QString text;

          do
          {
            line = input.readLine();
            text.append( line + '\n' );
          } while( !line.isNull() );


          KTextEditor::Document *doc = t->view()->document();
          if( doc )
              doc->setText( text );
        }

        if (nav && t->view())
          t->view()->setCursorPosition (KTextEditor::Cursor (line, column));
    }
    else
    {
      for ( int z = 0; z < args->count(); z++ )
      {
        // this file is no local dir, open it, else warn
        bool noDir = !args->url(z).isLocalFile() || !QDir (args->url(z).path()).exists();

        if (noDir)
        {
          KWrite *t = new KWrite();

//          if (Kate::document (t->view()->document()))
  //          KTextEditor::Document::setOpenErrorDialogsActivated (false);

          if (codec)
            t->view()->document()->setEncoding(codec->name());

          t->loadURL( args->url( z ) );

    //      if (Kate::document (t->view()->document()))
      //      KTextEditor::Document::setOpenErrorDialogsActivated (true);

          if (nav)
            t->view()->setCursorPosition (KTextEditor::Cursor (line, column));
        }
        else
        {
          KMessageBox::sorry(0, i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", args->url(z).url()));
          return 1; // see http://bugs.kde.org/show_bug.cgi?id=124708
        }
      }
    }
  }

  // no window there, uh, ohh, for example borked session config !!!
  // create at least one !!
  if (KWrite::noWindows())
    new KWrite();

  return a.exec ();
}


KWriteEditorChooser::KWriteEditorChooser(QWidget *parent):
  KDialog( parent )
{
    setCaption( i18n("Choose Editor Component") );
    setButtons( Ok | Cancel );
    setDefaultButton(KDialog::Cancel);
    m_chooser = new KTextEditor::EditorChooser(this);
    resizeLayout(m_chooser, 0, spacingHint());
    setMainWidget(m_chooser);
    m_chooser->readAppSetting();

    connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
}

KWriteEditorChooser::~KWriteEditorChooser()
{
}

void KWriteEditorChooser::slotOk()
{
    m_chooser->writeAppSetting();
}

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
