/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#include "config.h"

#include "katesession.h"
#include "katesession.moc"

#include "kateapp.h"
#include "katemainwindow.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "katerunninginstanceinfo.h"
#include "katedebug.h"

#include <KIconLoader>
#include <KMessageBox>
#include <KCodecs>
#include <KStandardGuiItem>
#include <KActionCollection>
#include <KIO/CopyJob>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <kjobwidgets.h>

#include <QCollator>
#include <QDir>
#include <QtAlgorithms>
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStyle>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <unistd.h>
#include <time.h>


bool katesessions_compare_sessions_ptr(const KateSession::Ptr &s1, const KateSession::Ptr &s2) {
    return QCollator().compare (s1->sessionName(),s2->sessionName())==-1;
}

//BEGIN KateSession

KateSession::KateSession (KateSessionManager *manager, const QString &fileName)
    : m_sessionFileRel (fileName)
    , m_documents (0)
    , m_manager (manager)
    , m_readConfig (0)
    , m_writeConfig (0)
{
  m_sessionName = QUrl::fromPercentEncoding(QFile::encodeName(fileName));
  m_sessionName.chop(12);//.katesession==12
  init ();
}

void KateSession::init ()
{
  // given file exists, use it to load some stuff ;)
  if (!m_sessionFileRel.isEmpty() && QFile::exists(sessionFile()))
  {
    KConfig config (sessionFile (), KConfig::SimpleConfig);

    // get the document count
    m_documents = config.group("Open Documents").readEntry("Count", 0);

    return;
  }

  if (!m_sessionFileRel.isEmpty() && !QFile::exists(sessionFile()))
    qCDebug(LOG_KATE) << "Warning, session file not found: " << m_sessionFileRel;
}

KateSession::~KateSession ()
{
  delete m_readConfig;
  delete m_writeConfig;
}

QString KateSession::sessionFile () const
{
  if (m_sessionFileRel.isEmpty()) {
    return QString();
  }

  return m_manager->sessionsDir() + '/' + m_sessionFileRel;
}

bool KateSession::create (const QString &name, bool force)
{
  if (!force && (name.isEmpty() || !m_sessionFileRel.isEmpty()))
    return false;

  delete m_writeConfig;
  m_writeConfig = 0;

  delete m_readConfig;
  m_readConfig = 0;

  m_sessionName = name;
  QString oldSessionFileRel = m_sessionFileRel;
  m_sessionFileRel = QUrl::toPercentEncoding(name, "", ".") + QString(".katesession");
  if ( QFile::exists(sessionFile()) )
  {
    m_sessionFileRel = oldSessionFileRel;
    return false;
  }

  // create the file, write name to it!
  KConfig config (sessionFile (), KConfig::SimpleConfig);
//  config.group("General").writeEntry ("Name", m_sessionName);
  config.sync ();

  // reinit ourselfs ;)
  init ();

  return true;
}

bool KateSession::rename (const QString &name)
{
  if (name.isEmpty () || m_sessionFileRel.isEmpty())
    return false;

  if (name == m_sessionName) return true;
  QString oldRel = m_sessionFileRel;
  QString oldSessionFile = sessionFile();
  m_sessionFileRel = QUrl::toPercentEncoding(name, "", ".") + QString(".katesession");
  if ( QFile::exists(sessionFile()) )
  {
    m_sessionFileRel = oldRel;
    return false;
  }

  QUrl srcUrl = QUrl::fromLocalFile( oldSessionFile );
  QUrl destUrl = QUrl::fromLocalFile( sessionFile() );

  KIO::CopyJob *job = KIO::move(srcUrl, destUrl, KIO::HideProgressInfo);
  if ( !job->exec() )
  {
    m_sessionFileRel = oldRel;
    return false;
  }
  m_sessionName = name;
  delete m_writeConfig;
  m_writeConfig=0;
  delete m_readConfig;
  m_readConfig=0;
  return true;
}

KConfig *KateSession::configRead ()
{
  if (m_sessionFileRel.isEmpty())
    return KSharedConfig::openConfig().data();

  if (m_readConfig)
    return m_readConfig;

  return m_readConfig = new KConfig (sessionFile (), KConfig::SimpleConfig);
}

KConfig *KateSession::configWrite ()
{
  if (m_sessionFileRel.isEmpty())
    return KSharedConfig::openConfig().data();

  if (m_writeConfig)
    return m_writeConfig;

  m_writeConfig = new KConfig (sessionFile (), KConfig::SimpleConfig);

  return m_writeConfig;
}

void KateSession::makeAnonymous()
{
  delete m_readConfig;
  m_readConfig = 0;

  delete m_writeConfig;
  m_writeConfig = 0;

  m_sessionFileRel.clear();
  m_sessionName.clear();
}

//END KateSession


//BEGIN KateSessionManager

KateSessionManager::KateSessionManager (QObject *parent)
    : QObject (parent)
    , m_sessionsDir (QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kate/sessions")
    , m_activeSession (new KateSession (this, QString()))
{
  qCDebug(LOG_KATE) << "LOCAL SESSION DIR: " << m_sessionsDir;

  // create dir if needed
  QDir(m_sessionsDir).mkpath(".");
}

KateSessionManager::~KateSessionManager()
{}

KateSessionManager *KateSessionManager::self()
{
  return KateApp::self()->sessionManager ();
}

void KateSessionManager::dirty (const QString &)
{
  updateSessionList ();
}

void KateSessionManager::updateSessionList ()
{
  m_sessionList.clear ();

  // Let's get a list of all session we have atm
  QDir dir (m_sessionsDir, "*.katesession");

  for (unsigned int i = 0; i < dir.count(); ++i)
  {
    KateSession *session = new KateSession (this, dir[i]);
    if (m_activeSession)
      if (session->sessionName()==m_activeSession->sessionName())
      {
        delete session;
        session=m_activeSession.data();
      }
    m_sessionList.append (KateSession::Ptr(session));

    //qCDebug(LOG_KATE) << "FOUND SESSION: " << session->sessionName() << " FILE: " << session->sessionFile() << " dir[i];" << dir[i];
  }

  qSort(m_sessionList.begin(), m_sessionList.end(), katesessions_compare_sessions_ptr);
}

bool KateSessionManager::activateSession (KateSession::Ptr session,
                                          bool closeLast,
                                          bool saveLast,
                                          bool loadNew)
{
  if ( (!session->sessionName().isEmpty()) && (m_activeSession!=session)) {
    //check if the requested session is already open in another instance
    KateRunningInstanceMap instances;
    if (!fillinRunningKateAppInstances(&instances))
    {
      KMessageBox::error(0,i18n("Internal error: there is more than one instance open for a given session."));
      return false;
    }

    if (instances.contains(session->sessionName()))
    {
      if (KMessageBox::questionYesNo(0,i18n("Session '%1' is already opened in another kate instance, change there instead of reopening?",session->sessionName()),
            QString(),KStandardGuiItem::yes(),KStandardGuiItem::no(),"katesessionmanager_switch_instance")==KMessageBox::Yes)
      {
        instances[session->sessionName()]->dbus_if->call("activate");
        cleanupRunningKateAppInstanceMap(&instances);
        return false;
      }
    }

    cleanupRunningKateAppInstanceMap(&instances);
  }
  // try to close last session
  if (closeLast)
  {
    if (KateApp::self()->activeMainWindow())
    {
      if (!KateApp::self()->activeMainWindow()->queryClose_internal())
        return true;
    }
  }

  // save last session or not?
  if (saveLast)
    saveActiveSession ();

  // really close last
  if (closeLast)
    KateDocManager::self()->closeAllDocuments ();

  // set the new session
  m_activeSession = session;

  if (loadNew)
  {
    // open the new session
    KConfig *sc = activeSession()->configRead();
    KSharedConfigPtr sharedConfig = KSharedConfig::openConfig();
    const bool loadDocs = (sc != sharedConfig.data()); // do not load docs for new sessions

    // if we have no session config object, try to load the default
    // (anonymous/unnamed sessions)
    if ( !sc )
      sc = sharedConfig.data();
    // load plugin config + plugins
    KatePluginManager::self()->loadConfig (sc);

    if (sc && loadDocs)
      KateApp::self()->documentManager()->restoreDocumentList (sc);

    // window config
    KConfigGroup c(sharedConfig, "General");

    if (c.readEntry("Restore Window Configuration", true))
    {
      // a new, named session, read settings of the default session.
      if ( ! sc->hasGroup("Open MainWindows") )
        sc = sharedConfig.data();

      int wCount = sc->group("Open MainWindows").readEntry("Count", 1);

      for (int i = 0; i < wCount; ++i)
      {
        if (i >= KateApp::self()->mainWindows())
        {
          KateApp::self()->newMainWindow(sc, QString ("MainWindow%1").arg(i));
        }
        else
        {
          KateApp::self()->mainWindow(i)->readProperties(KConfigGroup(sc, QString ("MainWindow%1").arg(i) ));
        }

        KateApp::self()->mainWindow(i)->restoreWindowConfig(KConfigGroup(sc, QString ("MainWindow%1 Settings").arg(i)));
      }

      // remove mainwindows we need no longer...
      if (wCount > 0)
      {
        while (wCount < KateApp::self()->mainWindows())
          delete KateApp::self()->mainWindow(KateApp::self()->mainWindows() - 1);
      }
    }
  }

  emit sessionChanged();
  return true;
}

KateSession::Ptr KateSessionManager::giveSession (const QString &name)
{
  if (name.isEmpty())
    return KateSession::Ptr(new KateSession (this, QString()));

  updateSessionList();

  for (int i = 0; i < m_sessionList.count(); ++i)
  {
    if (m_sessionList[i]->sessionName() == name)
      return m_sessionList[i];
  }

  KateSession::Ptr s(new KateSession (this, QString()));
  s->create (name);
  return s;
}

// helper function to save the session to a given config object
static void saveSessionTo(KConfig *sc)
{
  // save plugin configs and which plugins to load
  KatePluginManager::self()->writeConfig(sc);

  // save document configs + which documents to load
  KateDocManager::self()->saveDocumentList (sc);

  sc->group("Open MainWindows").writeEntry ("Count", KateApp::self()->mainWindows ());

  // save config for all windows around ;)
  bool saveWindowConfig = KConfigGroup(KSharedConfig::openConfig(), "General").readEntry("Restore Window Configuration", true);
  for (int i = 0; i < KateApp::self()->mainWindows (); ++i )
  {
    KConfigGroup cg(sc, QString ("MainWindow%1").arg(i) );
    KateApp::self()->mainWindow(i)->saveProperties (cg);
    if (saveWindowConfig)
      KateApp::self()->mainWindow(i)->saveWindowConfig(KConfigGroup(sc, QString ("MainWindow%1 Settings").arg(i)));
  }

  sc->sync();

  /**
   * try to sync file to disk
   */
  QFile fileToSync (sc->name());
  if (fileToSync.open (QIODevice::ReadOnly)) {
#ifndef Q_OS_WIN
    // ensure that the file is written to disk
#ifdef HAVE_FDATASYNC
    fdatasync (fileToSync.handle());
#else
    fsync (fileToSync.handle());
#endif
#endif
  }
}

bool KateSessionManager::saveActiveSession (bool rememberAsLast)
{
//  if (activeSession()->isAnonymous())
//    newSessionName();

  KConfig *sc = activeSession()->configWrite();

  if (!sc)
    return false;

  saveSessionTo(sc);

  if (rememberAsLast)
  {
    KSharedConfigPtr c = KSharedConfig::openConfig();
    c->group("General").writeEntry ("Last Session", activeSession()->sessionFileRelative());
    c->sync ();
  }
  return true;
}

bool KateSessionManager::chooseSession ()
{
  bool success = true;

  // app config
  KConfigGroup c(KSharedConfig::openConfig(), "General");

  // get last used session, default to default session
  QString lastSession (c.readEntry ("Last Session", QString()));
  QString sesStart (c.readEntry ("Startup Session", "manual"));

  // uhh, just open last used session, show no chooser
  if (sesStart == "last")
  {
    activateSession (KateSession::Ptr(new KateSession (this, lastSession)), false, false);
    return success;
  }

  // start with empty new session
  // also, if no sessions exist
  if (sesStart == "new" || sessionList().size() == 0)
  {
    activateSession (KateSession::Ptr(new KateSession (this, QString())), false, false);
    return success;
  }

  KateSessionChooser *chooser = new KateSessionChooser (0, lastSession);

  bool retry = true;
  int res = 0;
  while (retry)
  {
    res = chooser->exec ();

    switch (res)
    {
      case KateSessionChooser::resultOpen:
        {
          KateSession::Ptr s = chooser->selectedSession ();

          if (!s)
          {
            KMessageBox::error (chooser, i18n("No session selected to open."), i18n ("No Session Selected"));
            break;
          }

          success = activateSession (s, false, false);
          retry = false;
          break;
        }

      // exit the app lateron
      case KateSessionChooser::resultQuit:
        success = false;
        retry = false;
        break;

      case KateSessionChooser::resultNew: {
        success = true;
        retry = false;
        activateSession (KateSession::Ptr(new KateSession (this, QString())), false, false);
        break;
      }

      case KateSessionChooser::resultCopy: {
        KateSession::Ptr s = chooser->selectedSession ();
        if (!s) {
          KMessageBox::error (chooser, i18n("No session selected to copy."), i18n ("No Session Selected"));
          break;
        }

        success = true;
        retry = false;
        activateSession (s, false, false);
        s->makeAnonymous();
        // emit signal again, now we are anonymous
        emit sessionChanged();
        break;
      }

      default:
        activateSession (KateSession::Ptr(new KateSession (this, QString())), false, false) ;
        retry = false;
        break;
    }
  }

  // write back our nice boolean :)
  if (success && chooser->reopenLastSession ())
  {
    KConfigGroup generalConfig(KSharedConfig::openConfig(), "General");

    if (res == KateSessionChooser::resultOpen)
      generalConfig.writeEntry ("Startup Session", "last");
    else if (res == KateSessionChooser::resultNew)
      generalConfig.writeEntry ("Startup Session", "new");

    generalConfig.sync ();
  }

  delete chooser;

  return success;
}

void KateSessionManager::sessionNew ()
{
  activateSession (KateSession::Ptr(new KateSession (this, "")));
}

void KateSessionManager::sessionOpen ()
{
  KateSessionOpenDialog *chooser = new KateSessionOpenDialog (0);

  int res = chooser->exec ();

  if (res == KateSessionOpenDialog::resultCancel)
  {
    delete chooser;
    return;
  }

  KateSession::Ptr s = chooser->selectedSession ();

  if (s)
    activateSession (s);

  delete chooser;
}

void KateSessionManager::sessionSave ()
{
  // if the active session is valid, just save it :)
  if (saveActiveSession ())
    return;

  sessionSaveAs();
}

void KateSessionManager::sessionSaveAs ()
{
  newSessionName();
  saveActiveSession ();
  emit sessionChanged();
}

bool KateSessionManager::newSessionName()
{
  bool alreadyExists = false;
  QString name;
  do {
    bool ok = false;
    name = QInputDialog::getText (QApplication::activeWindow(), // nasty trick:)
             i18n("Specify New Name for Current Session"),
             alreadyExists ? i18n("There is already an existing session with your chosen name.\nPlease choose a different one\nSession name:") : i18n("Session name:"),
             QLineEdit::Normal, name, &ok);

    if (!ok)
      return false;

    if (name.isEmpty())
      KMessageBox::sorry (0, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));

    alreadyExists = true;
  }
  while (!activeSession()->create (name, true));
  return true;
}

void KateSessionManager::sessionManage ()
{
  KateSessionManageDialog *dlg = new KateSessionManageDialog (0);

  dlg->exec ();

  delete dlg;
}

//END KateSessionManager

//BEGIN CHOOSER DIALOG

class KateSessionChooserItem : public QTreeWidgetItem
{
  public:
    KateSessionChooserItem (QTreeWidget *tw, KateSession::Ptr s)
        : QTreeWidgetItem (tw, QStringList(s->sessionName()))
        , session (s)
    {
      QString docs;
      docs.setNum (s->documents());
      setText (1, docs);
    }

    KateSession::Ptr session;
};

KateSessionChooser::KateSessionChooser (QWidget *parent, const QString &lastSession)
  : QDialog (parent)
{
  setWindowTitle(i18n("Session Chooser"));
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  setLayout(mainLayout);

  // Main Tree
  QFrame *page = new QFrame (this);
  QVBoxLayout *tll = new QVBoxLayout(page);
  page->setMinimumSize (400, 200);
  mainLayout->addWidget(page);

  m_sessions = new QTreeWidget (page);
  tll->addWidget(m_sessions);
  QStringList header;
  header << i18n("Session Name");
  header << i18nc("The number of open documents", "Open Documents");
  m_sessions->setHeaderLabels(header);
  m_sessions->setRootIsDecorated( false );
  m_sessions->setItemsExpandable( false );
  m_sessions->setAllColumnsShowFocus( true );
  m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_sessions->setSelectionMode (QAbstractItemView::SingleSelection);

  const KateSessionList &slist (KateSessionManager::self()->sessionList());
  qCDebug(LOG_KATE)<<"Last session is:"<<lastSession;
  for (int i = 0; i < slist.count(); ++i)
  {
    KateSessionChooserItem *item = new KateSessionChooserItem (m_sessions, slist[i]);

    qCDebug(LOG_KATE)<<"Session added to chooser:"<<slist[i]->sessionName()<<"........"<<slist[i]->sessionFileRelative();
    if (slist[i]->sessionFileRelative() == lastSession)
      m_sessions->setCurrentItem (item);
  }

  connect(m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(m_sessions, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(slotOpen()));

  // bottom box
  QHBoxLayout *hb = new QHBoxLayout(this);
  mainLayout->addLayout(hb);

  m_useLast = new QCheckBox (i18n ("&Always use this choice"), page);
  hb->addWidget(m_useLast);

  // buttons
  QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
  hb->addWidget(buttonBox);

  QPushButton *cancelButton = new QPushButton();
  KGuiItem::assign(cancelButton, KStandardGuiItem::quit());
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(slotCancel()));
  buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

  m_openButton = new QPushButton(QIcon::fromTheme("document-open"), i18n("Open Session"));
  m_openButton->setEnabled(m_sessions->currentIndex().isValid());
  m_openButton->setDefault(true);
  m_openButton->setFocus();
  buttonBox->addButton(m_openButton, QDialogButtonBox::ActionRole);
  connect(m_openButton, SIGNAL(clicked()), this, SLOT(slotOpen()));

  QMenu* popup = new QMenu(this);
  m_openButton->setMenu(popup); // KF5 FIXME: setDelayedMenu is not supported by QPushButton
  QAction *a = popup->addAction(i18n("Use selected session as template"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotCopySession()));

  QPushButton *newButton = new QPushButton(QIcon::fromTheme("document-new"), i18n("New Session"));
  buttonBox->addButton(newButton, QDialogButtonBox::ActionRole);
  connect(newButton, SIGNAL(clicked()), this, SLOT(slotNew()));

  setResult (resultNone);
  m_sessions->resizeColumnToContents(0);
  selectionChanged (NULL, NULL);
}

KateSessionChooser::~KateSessionChooser ()
{}

void KateSessionChooser::slotCopySession()
{
  done(resultCopy);
}

KateSession::Ptr KateSessionChooser::selectedSession ()
{
  KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem ());

  if (!item)
    return KateSession::Ptr();

  return item->session;
}

bool KateSessionChooser::reopenLastSession ()
{
  return m_useLast->isChecked ();
}

void KateSessionChooser::slotOpen()
{
  done(resultOpen);
}

void KateSessionChooser::slotNew()
{
  done(resultNew);
}

void KateSessionChooser::slotCancel()
{
  done(resultQuit);
}

void KateSessionChooser::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
  Q_UNUSED(current);
  m_openButton->setEnabled(true);
}

//END CHOOSER DIALOG

//BEGIN OPEN DIALOG

KateSessionOpenDialog::KateSessionOpenDialog (QWidget *parent)
  : QDialog(parent)

{
  setWindowTitle(i18n("Open Session"));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  setLayout(mainLayout);

  m_sessions = new QTreeWidget (this);
  m_sessions->setMinimumSize(400, 200);
  mainLayout->addWidget(m_sessions);

  QStringList header;
  header << i18n("Session Name");
  header << i18nc("The number of open documents", "Open Documents");
  m_sessions->setHeaderLabels(header);
  m_sessions->setRootIsDecorated( false );
  m_sessions->setItemsExpandable( false );
  m_sessions->setAllColumnsShowFocus( true );
  m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_sessions->setSelectionMode (QAbstractItemView::SingleSelection);

  const KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i = 0; i < slist.count(); ++i)
  {
    new KateSessionChooserItem (m_sessions, slist[i]);
  }
  m_sessions->resizeColumnToContents(0);

  connect(m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(m_sessions, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(slotOpen()));

  // buttons
  QDialogButtonBox *buttons = new QDialogButtonBox(this);
  mainLayout->addWidget(buttons);

  QPushButton *cancelButton = new QPushButton;
  KGuiItem::assign(cancelButton, KStandardGuiItem::cancel());
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(slotCanceled()));
  buttons->addButton(cancelButton, QDialogButtonBox::RejectRole);

  m_openButton = new QPushButton(QIcon::fromTheme("document-open"), i18n("&Open"));
  m_openButton->setDefault(true);
  m_openButton->setEnabled(false);
  connect(m_openButton, SIGNAL(clicked()), this, SLOT(slotOpen()));
  buttons->addButton(m_openButton, QDialogButtonBox::AcceptRole);

  setResult (resultCancel);
}

KateSessionOpenDialog::~KateSessionOpenDialog ()
{}

KateSession::Ptr KateSessionOpenDialog::selectedSession ()
{
  KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem ());

  if (!item)
    return KateSession::Ptr();

  return item->session;
}

void KateSessionOpenDialog::slotCanceled()
{
  done(resultCancel);
}

void KateSessionOpenDialog::slotOpen()
{
  done(resultOk);
}

void KateSessionOpenDialog::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
  Q_UNUSED(current);
  m_openButton->setEnabled(true);
}

//END OPEN DIALOG

//BEGIN MANAGE DIALOG

KateSessionManageDialog::KateSessionManageDialog (QWidget *parent)
    : QDialog(parent)
{
  setWindowTitle(i18n("Manage Sessions"));

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  setLayout(mainLayout);

  QFrame *page = new QFrame (this);
  page->setMinimumSize(400, 200);
  mainLayout->addWidget(page);

  QHBoxLayout *hb = new QHBoxLayout(page);

  m_sessions = new QTreeWidget (page);
  hb->addWidget(m_sessions);
  m_sessions->setColumnCount(2);
  QStringList header;
  header << i18n("Session Name");
  header << i18nc("The number of open documents", "Open Documents");
  m_sessions->setHeaderLabels(header);
  m_sessions->setRootIsDecorated( false );
  m_sessions->setItemsExpandable( false );
  m_sessions->setAllColumnsShowFocus( true );
  m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_sessions->setSelectionMode (QAbstractItemView::SingleSelection);

  connect (m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

  updateSessionList ();
  m_sessions->resizeColumnToContents(0);

  // right column buttons
  QDialogButtonBox *rightButtons = new QDialogButtonBox(this);
  rightButtons->setOrientation(Qt::Vertical);
  hb->addWidget(rightButtons);

  m_rename = new QPushButton(i18n("&Rename..."));
  connect (m_rename, SIGNAL(clicked()), this, SLOT(rename()));
  rightButtons->addButton(m_rename, QDialogButtonBox::ApplyRole);

  m_del = new QPushButton();
  KGuiItem::assign(m_del, KStandardGuiItem::del());
  connect (m_del, SIGNAL(clicked()), this, SLOT(del()));
  rightButtons->addButton(m_del, QDialogButtonBox::ApplyRole);

  // dialog buttons
  QDialogButtonBox *bottomButtons = new QDialogButtonBox(this);
  mainLayout->addWidget(bottomButtons);

  QPushButton *closeButton = new QPushButton;
  KGuiItem::assign(closeButton, KStandardGuiItem::close());
  closeButton->setDefault(true);
  bottomButtons->addButton(closeButton, QDialogButtonBox::RejectRole);
  connect(closeButton, SIGNAL(clicked()), this, SLOT(slotClose()));

  m_openButton = new QPushButton(QIcon::fromTheme("document-open"), i18n("&Open"));
  bottomButtons->addButton(m_openButton, QDialogButtonBox::AcceptRole);
  connect(m_openButton, SIGNAL(clicked()), this, SLOT(open()));

  // trigger action update
  selectionChanged (NULL, NULL);
}

KateSessionManageDialog::~KateSessionManageDialog ()
{}

void KateSessionManageDialog::slotClose()
{
  done(0);
}

void KateSessionManageDialog::selectionChanged (QTreeWidgetItem *current, QTreeWidgetItem *)
{
  const bool validItem = (current != NULL);

  m_rename->setEnabled (validItem);
  m_del->setEnabled (validItem && (static_cast<KateSessionChooserItem*>(current))->session!=KateSessionManager::self()->activeSession());
  m_openButton->setEnabled(true);
}

void KateSessionManageDialog::rename ()
{
  KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem ());

  if (!item)
    return;

  bool ok = false;
  QString name = QInputDialog::getText(QApplication::activeWindow(), // nasty trick:)
                    i18n("Specify New Name for Session"), i18n("Session name:"),
                    QLineEdit::Normal, item->session->sessionName(), &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::sorry (this, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));
    return;
  }

  if (item->session->rename (name)) {
    if (item->session==KateSessionManager::self()->activeSession()) {
      emit KateSessionManager::self()->sessionChanged();
    }
    updateSessionList ();
  }
  else
    KMessageBox::sorry(this, i18n("The session could not be renamed to \"%1\", there already exists another session with the same name", name), i18n("Session Renaming"));
}

void KateSessionManageDialog::del ()
{
  KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem ());

  if (!item)
    return;

  QFile::remove (item->session->sessionFile());
  KateSessionManager::self()->updateSessionList ();
  updateSessionList ();
}

void KateSessionManageDialog::open ()
{
  KateSessionChooserItem *item = static_cast<KateSessionChooserItem *>(m_sessions->currentItem ());

  if (!item)
    return;

  hide();
  KateSessionManager::self()->activateSession (item->session);
  done(0);
}

void KateSessionManageDialog::updateSessionList ()
{
  m_sessions->clear ();

  const KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i = 0; i < slist.count(); ++i)
  {
    new KateSessionChooserItem (m_sessions, slist[i]);
  }
}

//END MANAGE DIALOG


KateSessionsAction::KateSessionsAction(const QString& text, QObject* parent)
    : KActionMenu(text, parent)
{
  connect(menu(), SIGNAL(aboutToShow()), this, SLOT(slotAboutToShow()));

  sessionsGroup = new QActionGroup( menu() );

  // reason for Qt::QueuedConnection: when switching session with N mainwindows
  // to e.g. 1 mainwindow, the last N - 1 mainwindows are deleted. Invoking
  // a session switch without queued connection deletes a mainwindow in which
  // the current code path is executed ---> crash. See bug #227008.
  connect(sessionsGroup, SIGNAL(triggered(QAction*)), this, SLOT(openSession(QAction*)), Qt::QueuedConnection);
}

void KateSessionsAction::slotAboutToShow()
{
  qDeleteAll( sessionsGroup->actions() );

  const KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i = 0; i < slist.count(); ++i)
  {
    QString sessionName = slist[i]->sessionName();
    sessionName.replace("&", "&&");
    QAction *action = new QAction( sessionName, sessionsGroup ); 
    action->setData(QVariant(i));
    menu()->addAction (action);
  }
}

void KateSessionsAction::openSession (QAction *action)
{
  const KateSessionList &slist (KateSessionManager::self()->sessionList());

  int i = action->data().toInt();
  if (i >= slist.count())
    return;

  KateSessionManager::self()->activateSession(slist[i]);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
