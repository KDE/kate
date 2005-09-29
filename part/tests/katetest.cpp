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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katetest.h"
#include "katetest.moc"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/editorchooser.h>

#include <kio/netaccess.h>

#include <kdeversion.h>
#include <kaboutapplication.h>
#include <dcopclient.h>
#include <kurl.h>
#include <kencodingfiledialog.h>
#include <kdiroperator.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kapplication.h>
#include <klocale.h>
#include <kurl.h>
#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kparts/event.h>
#include <kmenubar.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kstringhandler.h>

#include <QStackedWidget>
#include <qpainter.h>
#include <qlabel.h>
#include <qcursor.h>
#include <QMenu>
#include <qpixmap.h>
#include <qtimer.h>
#include <q3dropsite.h>
#include <q3dragobject.h>
#include <qtextcodec.h>
#include <qlayout.h>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <Q3ValueList>
#include <QDropEvent>
#include <Q3PopupMenu>
#include <kxmlguifactory.h>

// StatusBar field IDs
#define KWRITE_ID_GEN 1

QList<KTextEditor::Document*> KWrite::docList;
QList<KWrite*> KWrite::winList;

KWrite::KWrite (KTextEditor::Document *doc)
    : m_view(0),
      m_recentFiles(0),
      m_paShowPath(0),
      m_paShowStatusBar(0)
{
  setMinimumSize(200,200);

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

  setCentralWidget(m_view);

  setupActions();
  setupStatusBar();

  // signals for the statusbar
  connect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View *)), this, SLOT(cursorPositionChanged(KTextEditor::View *)));
  connect(m_view, SIGNAL(viewModeChanged(KTextEditor::View *)), this, SLOT(viewModeChanged(KTextEditor::View *)));
  connect(m_view, SIGNAL(selectionChanged (KTextEditor::View *)), this, SLOT(selectionChanged (KTextEditor::View *)));
  connect(m_view, SIGNAL(informationMessage (KTextEditor::View *, const QString &)), this, SLOT(informationMessage (KTextEditor::View *, const QString &)));
  connect(m_view->document(), SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(modifiedChanged()));
  connect(m_view->document(), SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(modifiedChanged()) );
  connect(m_view->document(), SIGNAL(documentNameChanged(KTextEditor::Document *)), this, SLOT(documentNameChanged()));

  setAcceptDrops(true);
  connect(m_view,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(slotDropEvent(QDropEvent *)));

  KGlobal::dirs()->addResourceDir( "data", QDir::currentPath() );
  setXMLFile( "katetest.rc" );
  createShellGUI( true );
  guiFactory()->addClient( m_view );

  // install a working kate part popup dialog thingy
  m_view->setContextMenu ((QMenu*)(factory()->container("ktexteditor_popup", this)) );

  // call it as last thing, must be sure everything is already set up ;)
  setAutoSaveSettings ("MainWindow Settings");

  // init with more usefull size, stolen from konq :)
  if ( !initialGeometrySet() && !kapp->config()->hasGroup("MainWindow Settings"))
    resize( 700, 480 );

  readConfig ();

  winList.append (this);

  updateStatus ();
  show ();
}

KWrite::~KWrite()
{
  winList.remove (this);

  if (m_view->document()->views().count() == 1)
  {
    docList.remove(m_view->document());
    delete m_view->document();
  }

  kapp->config()->sync ();
}

void KWrite::setupActions()
{
  KStdAction::close( this, SLOT(slotFlush()), actionCollection(), "file_close" )->setWhatsThis(i18n("Use this to close the current document"));

  // setup File menu
  KStdAction::print(this, SLOT(printDlg()), actionCollection())->setWhatsThis(i18n("Use this command to print the current document"));
  KStdAction::openNew( this, SLOT(slotNew()), actionCollection(), "file_new" )->setWhatsThis(i18n("Use this command to create a new document"));
  KStdAction::open( this, SLOT( slotOpen() ), actionCollection(), "file_open" )->setWhatsThis(i18n("Use this command to open an existing document for editing"));

  m_recentFiles = KStdAction::openRecent(this, SLOT(slotOpen(const KURL&)),
                                         actionCollection());
  m_recentFiles->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  KAction *a=new KAction(i18n("&New Window"), "window_new", 0, this, SLOT(newView()),
              actionCollection(), "view_new_view");
  a->setWhatsThis(i18n("Create another view containing the current document"));

  a=new KAction(i18n("Choose Editor..."),0,this,SLOT(changeEditor()),
		actionCollection(),"settings_choose_editor");
  a->setWhatsThis(i18n("Override the system wide setting for the default editing component"));

  KStdAction::quit(this, SLOT(close()), actionCollection())->setWhatsThis(i18n("Close the current document view"));

  // setup Settings menu
  setStandardToolBarMenuEnabled(true);

  m_paShowStatusBar = KStdAction::showStatusbar(this, SLOT(toggleStatusBar()), actionCollection(), "settings_show_statusbar");
  m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));

  m_paShowPath = new KToggleAction(i18n("Sho&w Path"), 0, this, SLOT(documentNameChanged()),
                    actionCollection(), "set_showPath");
  m_paShowPath->setCheckedState(i18n("Hide Path"));
  m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));
  a=KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection(), "set_configure_toolbars");
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  a=new KAction(i18n("&About Editor Component"),0,this,SLOT(aboutEditor()),actionCollection(),"help_about_editor");

}

void KWrite::setupStatusBar()
{
  // statusbar stuff
  m_lineColLabel = new QLabel( statusBar() );
  statusBar()->addWidget( m_lineColLabel, 0, false );
  m_lineColLabel->setAlignment( Qt::AlignCenter );

  m_modifiedLabel = new QLabel( QString("   "), statusBar() );
  statusBar()->addWidget( m_modifiedLabel, 0, false );
  m_modifiedLabel->setAlignment( Qt::AlignCenter );

  m_insertModeLabel = new QLabel( i18n(" INS "), statusBar() );
  statusBar()->addWidget( m_insertModeLabel, 0, false );
  m_insertModeLabel->setAlignment( Qt::AlignCenter );

  m_selectModeLabel = new QLabel( i18n(" NORM "), statusBar() );
  statusBar()->addWidget( m_selectModeLabel, 0, false );
  m_selectModeLabel->setAlignment( Qt::AlignCenter );

  m_fileNameLabel=new KSqueezedTextLabel( statusBar() );
  statusBar()->addWidget( m_fileNameLabel, 1, true );
  m_fileNameLabel->setMinimumSize( 0, 0 );
  m_fileNameLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
  m_fileNameLabel->setAlignment( /*Qt::AlignRight*/Qt::AlignLeft );

  m_modPm = SmallIcon("modified");
  m_modDiscPm = SmallIcon("modonhd");
  m_modmodPm = SmallIcon("modmod");
  m_noPm = SmallIcon("null");
}

// load on url
void KWrite::loadURL(const KURL &url)
{
  m_view->document()->openURL(url);
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
   m_view->document()->closeURL();
}

void KWrite::slotNew()
{
  if (m_view->document()->isModified() || !m_view->document()->url().isEmpty())
    new KWrite();
  else
    m_view->document()->openURL(KURL());
}

void KWrite::slotOpen()
{
	KEncodingFileDialog::Result r=KEncodingFileDialog::getOpenURLsAndEncoding(m_view->document()->encoding(), m_view->document()->url().url(),QString::null,this,i18n("Open File"));

  for (KURL::List::Iterator i=r.URLs.begin(); i != r.URLs.end(); ++i)
  {
    encoding = r.encoding;
    slotOpen ( *i );
  }
}

void KWrite::slotOpen( const KURL& url )
{
  if (url.isEmpty()) return;

  if (!KIO::NetAccess::exists(url, true, this))
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

void KWrite::slotFileNameChanged()
{
  if ( ! m_view->document()->url().isEmpty() )
    m_recentFiles->addURL( m_view->document()->url() );
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
  KKeyDialog dlg;
  dlg.insert(actionCollection());
  if( m_view )
    dlg.insert(m_view->actionCollection());
  dlg.configure();
}

void KWrite::editToolbars()
{
  KEditToolbar *dlg = new KEditToolbar(guiFactory());

  if (dlg->exec())
  {
    KParts::GUIActivateEvent ev1( false );
    QApplication::sendEvent( m_view, &ev1 );
    guiFactory()->removeClient( m_view );
    createShellGUI( false );
    createShellGUI( true );
    guiFactory()->addClient( m_view );
    KParts::GUIActivateEvent ev2( true );
    QApplication::sendEvent( m_view, &ev2 );
  }

  delete dlg;
}

void KWrite::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept(KURL::List::canDecode(event->mimeData()));
}

void KWrite::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}

void KWrite::slotDropEvent( QDropEvent *event )
{
  const KURL::List textlist = KURL::List::fromMimeData(event->mimeData());

  if (textlist.isEmpty())
    return;

  for (KURL::List::ConstIterator i=textlist.begin(); i != textlist.end(); ++i)
    slotOpen (*i);
}

void KWrite::slotEnableActions( bool enable )
{
  Q3ValueList<KAction *> actions = actionCollection()->actions();
  Q3ValueList<KAction *>::ConstIterator it = actions.begin();
  Q3ValueList<KAction *>::ConstIterator end = actions.end();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );

  actions = m_view->actionCollection()->actions();
  it = actions.begin();
  end = actions.end();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );
}

//common config
void KWrite::readConfig(KConfig *config)
{
  config->setGroup("General Options");

  m_paShowStatusBar->setChecked( config->readBoolEntry("ShowStatusBar") );
  m_paShowPath->setChecked( config->readBoolEntry("ShowPath") );

  m_recentFiles->loadEntries(config, "Recent Files");

  m_view->document()->editor()->readConfig(config);

  if( m_paShowStatusBar->isChecked() )
    statusBar()->show();
  else
    statusBar()->hide();
}

void KWrite::writeConfig(KConfig *config)
{
  config->setGroup("General Options");

  config->writeEntry("ShowStatusBar",m_paShowStatusBar->isChecked());
  config->writeEntry("ShowPath",m_paShowPath->isChecked());

  m_recentFiles->saveEntries(config, "Recent Files");

  m_view->document()->editor()->writeConfig(config);

  config->sync ();
}

//config file
void KWrite::readConfig()
{
  KConfig *config = kapp->config();
  readConfig(config);
}

void KWrite::writeConfig()
{
  KConfig *config = kapp->config();
  writeConfig(config);
}

// session management
void KWrite::restore(KConfig *config, int n)
{
  readPropertiesInternal(config, n);
}

void KWrite::readProperties(KConfig *config)
{
  readConfig(config);

  if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(m_view))
    iface->readSessionConfig(config);
}

void KWrite::saveProperties(KConfig *config)
{
  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.indexOf(m_view->document()) + 1);

  if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(m_view))
    iface->writeSessionConfig(config);
}

void KWrite::saveGlobalProperties(KConfig *config) //save documents
{
  config->setGroup("Number");
  config->writeEntry("NumberOfDocuments",docList.count());

  for (int z = 1; z <= docList.count(); z++)
  {
     QString buf = QString("Document %1").arg(z);
     config->setGroup(buf);

     KTextEditor::Document *doc = docList.at(z - 1);

     if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
       iface->writeSessionConfig(config);
  }

  for (int z = 1; z <= winList.count(); z++)
  {
     QString buf = QString("Window %1").arg(z);
     config->setGroup(buf);

     config->writeEntry("DocumentNumber",docList.indexOf(winList.at(z-1)->view()->document()) + 1);
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

  config->setGroup("Number");
  docs = config->readNumEntry("NumberOfDocuments");
  windows = config->readNumEntry("NumberOfWindows");

  for (int z = 1; z <= docs; z++)
  {
     buf = QString("Document %1").arg(z);
     config->setGroup(buf);
     doc=editor->createDocument(0);

     if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
       iface->readSessionConfig(config);
     docList.append(doc);
  }

  for (int z = 1; z <= windows; z++)
  {
    buf = QString("Window %1").arg(z);
    config->setGroup(buf);
    t = new KWrite(docList.at(config->readNumEntry("DocumentNumber") - 1));
    t->restore(config,z);
  }
}

void KWrite::aboutEditor()
{
  KAboutApplication *ad = new KAboutApplication (m_view->document()->editor()->aboutData(), this);

  ad->exec();

  delete ad;
}

void KWrite::updateStatus ()
{
  viewModeChanged (m_view);
  cursorPositionChanged (m_view);
  selectionChanged (m_view);
  modifiedChanged ();
  documentNameChanged ();
}

void KWrite::viewModeChanged ( KTextEditor::View *view )
{
  m_insertModeLabel->setText( view->viewMode() );
}

void KWrite::cursorPositionChanged ( KTextEditor::View *view )
{
  KTextEditor::Cursor position (view->cursorPositionVirtual());

  m_lineColLabel->setText(
    i18n(" Line: %1 Col: %2 ").arg(KGlobal::locale()->formatNumber(position.line()+1, 0))
                              .arg(KGlobal::locale()->formatNumber(position.column()+1, 0)) );
}

void KWrite::selectionChanged (KTextEditor::View *view)
{
  m_selectModeLabel->setText( view->blockSelection() ? i18n(" BLK ") : i18n(" NORM ") );
}

void KWrite::informationMessage (KTextEditor::View *, const QString &message)
{
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
    bool modOnHD = false; //info && info->modifiedOnDisc;

    m_modifiedLabel->setPixmap(
        mod ? m_modPm : m_noPm
          /*info && modOnHD ?
            m_modmodPm :
            m_modPm :
          info && modOnHD ?
            m_modDiscPm :
        m_noPm*/
        );
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
      c = m_view->document()->url().prettyURL();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = "..." + c.right(64);
    }

    setCaption (c, m_view->document()->isModified());
  }
}

static KCmdLineOptions options[] =
{
  { "stdin",    I18N_NOOP("Read the contents of stdin"), 0},
  { "encoding <argument>",      I18N_NOOP("Set encoding for the file to open"), 0 },
  { "line <argument>",      I18N_NOOP("Navigate to this line"), 0 },
  { "column <argument>",      I18N_NOOP("Navigate to this column"), 0 },
  { "+[URL]",   I18N_NOOP("Document to open"), 0 },
  KCmdLineLastOption
};

extern "C" KDE_EXPORT int main(int argc, char **argv)
{
  KLocale::setMainCatalog("kate");         //lukas: set this to have the kwritepart translated using kate message catalog

  // here we go, construct the KWrite version
  QString kWriteVersion  = QString ("%1.%2.%3").arg(KDE::versionMajor() + 1).arg(KDE::versionMinor()).arg(KDE::versionRelease());

  KAboutData aboutData ( "kwrite",
                         I18N_NOOP("KWrite"),
                         kWriteVersion.latin1(),
                         I18N_NOOP( "KWrite - Text Editor" ), KAboutData::License_LGPL_V2,
                         I18N_NOOP( "(c) 2000-2005 The Kate Authors" ), 0, "http://kate.kde.org" );

  aboutData.addAuthor ("Christoph Cullmann", I18N_NOOP("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
  aboutData.addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  aboutData.addAuthor ("Hamish Rodda",I18N_NOOP("Core Developer"), "rodda@kde.org");
  aboutData.addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
  aboutData.addAuthor ("Charles Samuels", I18N_NOOP("The Editing Commands"), "charles@kde.org");
  aboutData.addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor ("Michael Bartl", I18N_NOOP("Former Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
  aboutData.addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
  aboutData.addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  aboutData.addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");

  aboutData.addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  aboutData.addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  aboutData.addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  aboutData.addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  aboutData.addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  aboutData.addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  aboutData.addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  aboutData.addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  aboutData.addCredit ("Daniel Naber","","");
  aboutData.addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  aboutData.addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  aboutData.addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  aboutData.addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

  aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication a;

  KGlobal::locale()->insertCatalog("katepart");

  DCOPClient *client = KApplication::dcopClient();
  if (!client->isRegistered())
  {
    client->attach();
    client->registerAs("kwrite");
  }

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (qApp->isSessionRestored())
  {
    KWrite::restore();
  }
  else
  {
    bool nav = false;
    int line = 0, column = 0;

    QTextCodec *codec = args->isSet("encoding") ? QTextCodec::codecForName(args->getOption("encoding")) : 0;

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
          QTextIStream input(stdin);

          // set chosen codec
          if (codec)
            input.setCodec (codec);

          QString line;
          QString text;

          do
          {
            line = input.readLine();
            text.append( line + "\n" );
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
        KWrite *t = new KWrite();

        // this file is no local dir, open it, else warn
        bool noDir = !args->url(z).isLocalFile() || !QDir (args->url(z).path()).exists();

        if (noDir)
        {
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
          KMessageBox::sorry( t, i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(args->url(z).url()) );
      }
    }
  }

  // no window there, uh, ohh, for example borked session config !!!
  // create at least one !!
  if (KWrite::noWindows())
    new KWrite();

  return a.exec ();
}


KWriteEditorChooser::KWriteEditorChooser(QWidget *):
	KDialogBase(KDialogBase::Plain,i18n("Choose Editor Component"),KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Cancel)
{
	(new QVBoxLayout(plainPage()))->setAutoAdd(true);
	m_chooser=new KTextEditor::EditorChooser(plainPage());
	setMainWidget(m_chooser);
	m_chooser->readAppSetting();
}

KWriteEditorChooser::~KWriteEditorChooser() {
;
}

void KWriteEditorChooser::slotOk() {
	m_chooser->writeAppSetting();
	KDialogBase::slotOk();
}
