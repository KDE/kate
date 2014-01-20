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

#include "kwrite.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/editor.h>

#include <KAboutApplicationDialog>
#include <KActionCollection>
#include <KToggleAction>
#include <KEditToolBar>
#include <KEncodingFileDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KSqueezedTextLabel>
#include <KStringHandler>
#include <KXMLGUIFactory>
#include <KConfig>
#include <KConfigGui>
#include <KIO/Job>
#include <KJobWidgets>

#ifdef KActivities_FOUND
#include <kactivities/resourceinstance.h>
#endif

#include <QTimer>
#include <QTextCodec>
#include <QMimeData>
#include <QApplication>
#include <QLabel>
#include <QStatusBar>
#include <QDragEnterEvent>

QList<KTextEditor::Document*> KWrite::docList;
QList<KWrite*> KWrite::winList;

KWrite::KWrite (KTextEditor::Document *doc)
    : m_view(0),
      m_recentFiles(0),
      m_paShowPath(0),
      m_paShowStatusBar(0)
#ifdef KActivities_FOUND
      , m_activityResource(0)
#endif
{
  if ( !doc )
  {
    doc = KTextEditor::Editor::instance()->createDocument(0);

    // enable the modified on disk warning dialogs if any
    if (qobject_cast<KTextEditor::ModificationInterface *>(doc))
      qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning (true);

    docList.append(doc);
  }

  m_view = doc->createView (this);

  setCentralWidget(m_view);

  setupActions();
  setupStatusBar();

  // signals for the statusbar
  connect(m_view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), this, SLOT(cursorPositionChanged(KTextEditor::View*)));
  connect(m_view, SIGNAL(viewModeChanged(KTextEditor::View*)), this, SLOT(viewModeChanged(KTextEditor::View*)));
  connect(m_view, SIGNAL(selectionChanged(KTextEditor::View*)), this, SLOT(selectionChanged(KTextEditor::View*)));
  connect(m_view, SIGNAL(informationMessage(KTextEditor::View*,QString)), this, SLOT(informationMessage(KTextEditor::View*,QString)));
  connect(m_view->document(), SIGNAL(modifiedChanged(KTextEditor::Document*)), this, SLOT(modifiedChanged()));
  connect(m_view->document(), SIGNAL(modifiedOnDisk(KTextEditor::Document*,bool,KTextEditor::ModificationInterface::ModifiedOnDiskReason)), this, SLOT(modifiedChanged()) );
  connect(m_view->document(), SIGNAL(documentNameChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged()));
  connect(m_view->document(), SIGNAL(readWriteChanged(KTextEditor::Document*)), this, SLOT(documentNameChanged()));
  connect(m_view->document(),SIGNAL(documentUrlChanged(KTextEditor::Document*)), this, SLOT(urlChanged()));
  connect(m_view->document(), SIGNAL(modeChanged(KTextEditor::Document*)), this, SLOT(modeChanged(KTextEditor::Document*)));

  setAcceptDrops(true);
  connect(m_view,SIGNAL(dropEventPass(QDropEvent*)),this,SLOT(slotDropEvent(QDropEvent*)));

  setXMLFile(QLatin1String("kwriteui.rc"));
  createShellGUI( true );
  guiFactory()->addClient( m_view );

  // init with more useful size, stolen from konq :)
/* FIXME KF5  if (!initialGeometrySet())
    resize( QSize(700, 480).expandedTo(minimumSizeHint()));
*/

  // FIXME: make sure the config dir exists, any idea how to do it more cleanly?
  QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).mkpath(QLatin1String("."));

  // call it as last thing, must be sure everything is already set up ;)
  setAutoSaveSettings ();

  readConfig ();

  winList.append (this);

  updateStatus ();
  show ();

  // give view focus
  m_view->setFocus (Qt::OtherFocusReason);
}

KWrite::~KWrite()
{
  guiFactory()->removeClient(m_view);

  winList.removeAll(this);
  
  KTextEditor::Document *doc = m_view->document();
  delete m_view;

  // kill document, if last view is closed
  if (doc->views().isEmpty()) {
    docList.removeAll(doc);
    delete doc;
  }

  KSharedConfig::openConfig()->sync();
}

void KWrite::setupActions()
{
  actionCollection()->addAction( KStandardAction::Close, QLatin1String("file_close"), this, SLOT(slotFlush()) )
    ->setWhatsThis(i18n("Use this command to close the current document"));

  // setup File menu
  actionCollection()->addAction( KStandardAction::New, QLatin1String("file_new"), this, SLOT(slotNew()) )
    ->setWhatsThis(i18n("Use this command to create a new document"));
  actionCollection()->addAction( KStandardAction::Open, QLatin1String("file_open"), this, SLOT(slotOpen()) )
    ->setWhatsThis(i18n("Use this command to open an existing document for editing"));

  m_recentFiles = KStandardAction::openRecent(this, SLOT(slotOpen(QUrl)), this);
  actionCollection()->addAction(m_recentFiles->objectName(), m_recentFiles);
  m_recentFiles->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  QAction *a = actionCollection()->addAction( QLatin1String("view_new_view") );
  a->setIcon( QIcon::fromTheme(QLatin1String("window-new")) );
  a->setText( i18n("&New Window") );
  connect( a, SIGNAL(triggered()), this, SLOT(newView()) );
  a->setWhatsThis(i18n("Create another view containing the current document"));

  actionCollection()->addAction( KStandardAction::Quit, this, SLOT(close()) )
    ->setWhatsThis(i18n("Close the current document view"));

  // setup Settings menu
  setStandardToolBarMenuEnabled(true);

  m_paShowStatusBar = KStandardAction::showStatusbar(this, SLOT(toggleStatusBar()), this);
  actionCollection()->addAction( QLatin1String("settings_show_statusbar"), m_paShowStatusBar);
  m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));

  m_paShowPath = new KToggleAction( i18n("Sho&w Path"), this );
  actionCollection()->addAction( QLatin1String("set_showPath"), m_paShowPath );
  connect( m_paShowPath, SIGNAL(triggered()), this, SLOT(documentNameChanged()) );
  m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));

  a= actionCollection()->addAction( KStandardAction::KeyBindings, this, SLOT(editKeys()) );
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a = actionCollection()->addAction( KStandardAction::ConfigureToolbars, QLatin1String("options_configure_toolbars"),
                                     this, SLOT(editToolbars()) );
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  a = actionCollection()->addAction( QLatin1String("help_about_editor") );
  a->setText( i18n("&About Editor Component") );
  connect( a, SIGNAL(triggered()), this, SLOT(aboutEditor()) );

}

void KWrite::setupStatusBar()
{
  // statusbar stuff
  QString lineColText = i18nc("@info:status Statusbar label for cursor line and column position",
    " Line: %1 Col: %2 ", 4444, 44);

  m_lineColLabel = new QLabel( statusBar() );
  m_lineColLabel->setMinimumWidth( m_lineColLabel->fontMetrics().width( lineColText ) );
  statusBar()->addWidget( m_lineColLabel, 0 );

  m_modifiedLabel = new QLabel( statusBar() );
  m_modifiedLabel->setFixedSize( 16, 16 );
  statusBar()->addWidget( m_modifiedLabel, 0 );
  m_modifiedLabel->setAlignment( Qt::AlignCenter );

  m_selectModeLabel = new QLabel( i18nc("@info:status Statusbar label for line selection mode", " LINE "), statusBar() );
  statusBar()->addWidget( m_selectModeLabel, 0 );
  m_selectModeLabel->setAlignment( Qt::AlignCenter );

  m_insertModeLabel = new QLabel( i18n(" INS "), statusBar() );
  statusBar()->addWidget( m_insertModeLabel, 0 );
  m_insertModeLabel->setAlignment( Qt::AlignCenter );

  m_modeLabel = new QLabel( QString(), statusBar() );
  statusBar()->addWidget( m_modeLabel, 0 );
  m_modeLabel->setAlignment( Qt::AlignVCenter | Qt::AlignLeft );

  m_fileNameLabel=new KSqueezedTextLabel( statusBar() );
  statusBar()->addPermanentWidget( m_fileNameLabel, 1 );
  m_fileNameLabel->setTextFormat(Qt::PlainText);
  m_fileNameLabel->setMinimumSize( 0, 0 );
  m_fileNameLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
  m_fileNameLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
}

// load on url
void KWrite::loadURL(const QUrl &url)
{
#ifdef KActivities_FOUND
  if (!m_activityResource) {
    m_activityResource = new KActivities::ResourceInstance(winId(), this);
  }
  m_activityResource->setUri(url);
#endif
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
  const KEncodingFileDialog::Result r=KEncodingFileDialog::getOpenUrlsAndEncoding(KTextEditor::Editor::instance()->defaultEncoding(), m_view->document()->url(),QString(),this,i18n("Open File"));
  Q_FOREACH (QUrl url, r.URLs) {
    encoding = r.encoding;
    slotOpen ( url );
  }
}

void KWrite::slotOpen( const QUrl& url )
{
  if (url.isEmpty()) return;

  KIO::StatJob *job = KIO::stat(url, KIO::StatJob::SourceSide, 0);
  KJobWidgets::setWindow(job, this);
  if (!job->exec())
  {
    KMessageBox::error (this, i18n("The file given could not be read; check whether it exists or is readable for the current user."));
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
  
  // update caption
  documentNameChanged ();
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
  KConfigGroup cfg = KSharedConfig::openConfig()->group( "MainWindow" );
  saveMainWindowSettings(cfg);
  KEditToolBar dlg(guiFactory(), this);

  connect( &dlg, SIGNAL(newToolBarConfig()), this, SLOT(slotNewToolbarConfig()) );
  dlg.exec();
}

void KWrite::slotNewToolbarConfig()
{
    applyMainWindowSettings( KSharedConfig::openConfig()->group( "MainWindow" ) );
}

void KWrite::dragEnterEvent( QDragEnterEvent *event )
{
  const QList<QUrl> uriList = event->mimeData()->urls();
  event->setAccepted( ! uriList.isEmpty());
}

void KWrite::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}

void KWrite::slotDropEvent( QDropEvent *event )
{
  const QList<QUrl> textlist = event->mimeData()->urls();

  foreach (const QUrl & url, textlist)
    slotOpen (url);
}

void KWrite::slotEnableActions( bool enable )
{
  QList<QAction *> actions = actionCollection()->actions();
  QList<QAction *>::ConstIterator it = actions.constBegin();
  QList<QAction *>::ConstIterator end = actions.constEnd();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );

  actions = m_view->actionCollection()->actions();
  it = actions.constBegin();
  end = actions.constEnd();

  for (; it != end; ++it )
      (*it)->setEnabled( enable );
}

//common config
void KWrite::readConfig(KSharedConfigPtr config)
{
  KConfigGroup cfg( config, "General Options");

  m_paShowStatusBar->setChecked( cfg.readEntry("ShowStatusBar", true) );
  m_paShowPath->setChecked( cfg.readEntry("ShowPath", false) );

  m_recentFiles->loadEntries( config->group( "Recent Files" ));

  // editor config already read from KSharedConfig::openConfig() in KWriteApp constructor.
  // so only load, if the config is a different one (this is only the case on
  // session restore)
  if (config != KSharedConfig::openConfig())
    KTextEditor::Editor::instance()->readConfig(config.data());

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
  KTextEditor::Editor::instance()->writeConfig(config.data());

  config->sync ();
}

//config file
void KWrite::readConfig()
{
  readConfig(KSharedConfig::openConfig());
}

void KWrite::writeConfig()
{
  writeConfig(KSharedConfig::openConfig());
}

// session management
void KWrite::restore(KConfig *config, int n)
{
  readPropertiesInternal(config, n);
}

void KWrite::readProperties(const KConfigGroup &config)
{
  readConfig();

  if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(m_view))
    iface->readSessionConfig(KConfigGroup(&config, QLatin1String("General Options")));
}

void KWrite::saveProperties(KConfigGroup &config)
{
  writeConfig();

  config.writeEntry("DocumentNumber",docList.indexOf(m_view->document()) + 1);

  if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(m_view)) {
    KConfigGroup cg(&config, QLatin1String("General Options"));
    iface->writeSessionConfig(cg);
  }
}

void KWrite::saveGlobalProperties(KConfig *config) //save documents
{
  config->group("Number").writeEntry("NumberOfDocuments",docList.count());

  for (int z = 1; z <= docList.count(); z++)
  {
     QString buf = QString::fromLatin1("Document %1").arg(z);
     KConfigGroup cg( config, buf );
     KTextEditor::Document *doc = docList.at(z - 1);

     if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
       iface->writeSessionConfig(cg);
  }

  for (int z = 1; z <= winList.count(); z++)
  {
     QString buf = QString::fromLatin1("Window %1").arg(z);
     KConfigGroup cg( config, buf );
     cg.writeEntry("DocumentNumber",docList.indexOf(winList.at(z-1)->view()->document()) + 1);
  }
}

//restore session
void KWrite::restore()
{
  KConfig *config = KConfigGui::sessionConfig();

  if (!config)
    return;

  int docs, windows;
  QString buf;
  KTextEditor::Document *doc;
  KWrite *t;

  KConfigGroup numberConfig(config, "Number");
  docs = numberConfig.readEntry("NumberOfDocuments", 0);
  windows = numberConfig.readEntry("NumberOfWindows", 0);

  for (int z = 1; z <= docs; z++)
  {
     buf = QString::fromLatin1("Document %1").arg(z);
     KConfigGroup cg(config, buf);
     doc=KTextEditor::Editor::instance()->createDocument(0);

     if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
       iface->readSessionConfig(cg);
     docList.append(doc);
  }

  for (int z = 1; z <= windows; z++)
  {
    buf = QString::fromLatin1("Window %1").arg(z);
    KConfigGroup cg(config, buf);
    t = new KWrite(docList.at(cg.readEntry("DocumentNumber", 0) - 1));
    t->restore(config,z);
  }
}

void KWrite::aboutEditor()
{
  KAboutApplicationDialog dlg(KTextEditor::Editor::instance()->aboutData(), this);
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
    i18nc("@info:status Statusbar label for cursor line and column position",
    	" Line: %1 Col: %2 ", position.line()+1, position.column()+1) ) ;
}

void KWrite::selectionChanged (KTextEditor::View *view)
{
  m_selectModeLabel->setText(
  	view->blockSelection() ? i18nc("@info:status Statusbar label for block selection mode", " BLOCK ") :
				i18nc("@info:status Statusbar label for line selection mode", " LINE ") );
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

    if (mod && m_modPm.isNull()) {
        m_modPm = QIcon::fromTheme(QLatin1String("document-properties")).pixmap(16);
    }

   /* const KateDocumentInfo *info
      = KateDocManager::self()->documentInfo ( m_view->document() );
*/
//    bool modOnHD = false; //info && info->modifiedOnDisc;

    m_modifiedLabel->setPixmap(
        mod ? m_modPm : QPixmap()
          /*info && modOnHD ?
            m_modmodPm :
            m_modPm :
          info && modOnHD ?
            m_modDiscPm :
        QPixmap()*/
        );

    documentNameChanged(); // update the modified flag in window title
}

void KWrite::documentNameChanged ()
{
  m_fileNameLabel->setText( QString::fromLatin1(" %1 ").arg (KStringHandler::lsqueeze(m_view->document()->documentName (), 64)));

  QString readOnlyCaption;
  if  (!m_view->document()->isReadWrite())
    readOnlyCaption=i18n(" [read only]");
  
  if (m_view->document()->url().isEmpty()) {
    setCaption(i18n("Untitled")+readOnlyCaption,m_view->document()->isModified());
  }
  else
  {
    QString c;
    if (!m_paShowPath->isChecked())
    {
      c = m_view->document()->url().fileName();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = c.left(64) + QLatin1String("...");
    }
    else
    {
      c = m_view->document()->url().toString();

      //File name shouldn't be too long - Maciek
      if (c.length() > 64)
        c = QLatin1String("...") + c.right(64);
    }

    setCaption (c+readOnlyCaption, m_view->document()->isModified());
  }
}

void KWrite::modeChanged ( KTextEditor::Document *document )
{
  QString mode = document->mode();
  if (!mode.isEmpty())
    mode = i18nc("Language", document->mode().toUtf8().data());
  m_modeLabel->setText(mode);
}
