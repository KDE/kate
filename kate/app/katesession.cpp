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

#include "katesession.h"
#include "katesession.moc"

#include "kateapp.h"
#include "katemainwindow.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"

#include <KStandardDirs>
#include <KLocale>
#include <kdebug.h>
#include <KDirWatch>
#include <KInputDialog>
#include <KIconLoader>
#include <KMessageBox>
#include <KCodecs>
#include <KStandardGuiItem>
#include <KPushButton>
#include <KMenu>
#include <KActionCollection>
#include <KIO/NetAccess>
#include <KIO/CopyJob>
#include <KStringHandler>

#include <QDir>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QtAlgorithms>

#include <unistd.h>
#include <time.h>


bool katesessions_compare_sessions_ptr(const KateSession::Ptr &s1, const KateSession::Ptr &s2) {
    return KStringHandler::naturalCompare(s1->sessionName(),s2->sessionName())==-1;
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
  if (!m_sessionFileRel.isEmpty() && KGlobal::dirs()->exists(sessionFile ()))
  {
    KConfig config (sessionFile (), KConfig::SimpleConfig);

    // get the document count
    m_documents = config.group("Open Documents").readEntry("Count", 0);

    return;
  }

  if (!m_sessionFileRel.isEmpty() && !KGlobal::dirs()->exists(sessionFile ()))
    kDebug() << "Warning, session file not found: " << m_sessionFileRel;
}

KateSession::~KateSession ()
{
  delete m_readConfig;
  delete m_writeConfig;
}

QString KateSession::sessionFile () const
{
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
  if (KGlobal::dirs()->exists(sessionFile ()))
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
  if (KGlobal::dirs()->exists(sessionFile ()))
  {
    m_sessionFileRel = oldRel;
    return false;
  }
  KUrl srcUrl(QString("file://"));
  srcUrl.addPath(oldSessionFile);
  KUrl destUrl(QString("file://"));
  destUrl.addPath(sessionFile());
  KIO::CopyJob *job = KIO::move(srcUrl, destUrl, KIO::HideProgressInfo);
  if ( ! KIO::NetAccess::synchronousRun(job, 0) )
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
    return 0;

  if (m_readConfig)
    return m_readConfig;

  return m_readConfig = new KConfig (sessionFile (), KConfig::SimpleConfig);
}

KConfig *KateSession::configWrite ()
{
  if (m_sessionFileRel.isEmpty())
    return 0;

  if (m_writeConfig)
    return m_writeConfig;

  m_writeConfig = new KConfig (sessionFile (), KConfig::SimpleConfig);
//  m_writeConfig->group("General").writeEntry ("Name", m_sessionName);

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
    , m_sessionsDir (KStandardDirs::locateLocal( "data", "kate/sessions"))
    , m_activeSession (new KateSession (this, QString()))
    , m_defaultSessionFile(KStandardDirs::locate( "data", "kate/default.katesession"))
{
  kDebug() << "LOCAL SESSION DIR: " << m_sessionsDir;

  // create dir if needed
  KGlobal::dirs()->makeDir (m_sessionsDir);
}

KateSessionManager::~KateSessionManager()
{}

KateSessionManager *KateSessionManager::self()
{
  return KateApp::self()->sessionManager ();
}

QString KateSessionManager::defaultSessionFile() const
{
  return m_defaultSessionFile;
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

    //kDebug () << "FOUND SESSION: " << session->sessionName() << " FILE: " << session->sessionFile() << " dir[i];" << dir[i];
  }

  qSort(m_sessionList.begin(), m_sessionList.end(), katesessions_compare_sessions_ptr);
}

void KateSessionManager::activateSession (KateSession::Ptr session,
                                          bool closeLast,
                                          bool saveLast,
                                          bool loadNew)
{
  // try to close last session
  if (closeLast)
  {
    if (KateApp::self()->activeMainWindow())
    {
      if (!KateApp::self()->activeMainWindow()->queryClose_internal())
        return;
    }
  }

  // save last session or not?
  if (saveLast)
    saveActiveSession (true);

  // really close last
  if (closeLast)
    KateDocManager::self()->closeAllDocuments ();

  // set the new session
  m_activeSession = session;

  if (loadNew)
  {
    // open the new session
    KConfig *sc = activeSession()->configRead();
    const bool loadDocs = (sc != 0); // do not load docs for new sessions

    // if we have no session config object, try to load the default
    // (anonymous/unnamed sessions)
    if ( !sc )
      sc = new KConfig( defaultSessionFile(), KConfig::SimpleConfig );

    // load plugin config + plugins
    KatePluginManager::self()->loadConfig (sc);

    if (sc && loadDocs)
      KateApp::self()->documentManager()->restoreDocumentList (sc);

    // window config
    KConfigGroup c(KGlobal::config(), "General");

    if (c.readEntry("Restore Window Configuration", true))
    {
      // a new, named session, read settings of the default session.
      if ( ! sc->hasGroup("Open MainWindows") )
        sc = new KConfig( defaultSessionFile(), KConfig::SimpleConfig );

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
  bool saveWindowConfig = KConfigGroup(KGlobal::config(), "General").readEntry("Restore Window Configuration", true);
  for (int i = 0; i < KateApp::self()->mainWindows (); ++i )
  {
    KConfigGroup cg(sc, QString ("MainWindow%1").arg(i) );
    KateApp::self()->mainWindow(i)->saveProperties (cg);
    if (saveWindowConfig)
      KateApp::self()->mainWindow(i)->saveWindowConfig(KConfigGroup(sc, QString ("MainWindow%1 Settings").arg(i)));
  }

  sc->sync();
}

bool KateSessionManager::saveActiveSession (bool tryAsk, bool rememberAsLast)
{
  if (tryAsk)
  {
    // app config
    KConfigGroup c(KGlobal::config(), "General");

    QString sesExit (c.readEntry ("Session Exit", "save"));

    if (sesExit == "discard")
      return true;

    if (sesExit == "ask")
    {
      KDialog *dlg = new KDialog( 0 );
      dlg->setCaption( i18n ("Save Session?") );
      dlg->setButtons( KDialog::Yes | KDialog::No );
      dlg->setDefaultButton( KDialog::Yes );
      dlg->setEscapeButton( KDialog::No );

      bool dontAgain = false;
      int res = KMessageBox::createKMessageBox(dlg, QMessageBox::Question,
                i18n("Save current session?"), QStringList(),
                i18n("Do not ask again"), &dontAgain, KMessageBox::Notify);

      // remember to not ask again with right setting
      if (dontAgain)
      {
        KConfigGroup generalConfig(KGlobal::config(), "General");
        if (res == KDialog::No)
          generalConfig.writeEntry ("Session Exit", "discard");
        else
          generalConfig.writeEntry ("Session Exit", "save");
      }

      if (res == KDialog::No)
        return true;
    }
  }

//  if (activeSession()->isAnonymous())
//    newSessionName();

  KConfig *sc = activeSession()->configWrite();

  if (!sc)
    return false;

  saveSessionTo(sc);

  if (rememberAsLast)
  {
    KSharedConfig::Ptr c = KGlobal::config();
    c->group("General").writeEntry ("Last Session", activeSession()->sessionFileRelative());
    c->sync ();
  }
  return true;
}

bool KateSessionManager::chooseSession ()
{
  bool success = true;

  // app config
  KConfigGroup c(KGlobal::config(), "General");

  // get last used session, default to default session
  QString lastSession (c.readEntry ("Last Session", defaultSessionFile()));
  QString sesStart (c.readEntry ("Startup Session", "manual"));

  // if this is the first time Kate App starts, create a i18ned default.katesession
  bool firstStart = c.readEntry ("First Start", true);
  if (firstStart) {
    c.writeEntry ("First Start", false);
    QFile::copy(defaultSessionFile(), sessionsDir() + "/default.katesession");
    KateSession *ds = new KateSession(this, "default.katesession");
    ds->rename(i18n("Default Session"));
    lastSession=ds->sessionFileRelative();
    delete ds;
  }

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

          activateSession (s, false, false);
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
    KConfigGroup generalConfig(KGlobal::config(), "General");

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
    name = KInputDialog::getText (
             i18n("Specify New Name for Current Session"),
             alreadyExists ? i18n("There is already an existing session with your chosen name.\nPlease choose a different one\nSession name:") : i18n("Session name:")
             , name, &ok);

    if (!ok)
      return false;

    if (name.isEmpty())
      KMessageBox::sorry (0, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));

    alreadyExists = true;
  }
  while (!activeSession()->create (name, true));
  return true;
}

void KateSessionManager::sessionSaveAsDefault ()
{
  // get path of local default.katesession file
  QString localSessionFile = KStandardDirs::locateLocal("data", "kate/default.katesession");

  // use this local file to store the defaults to
  KConfig defaultConfig(localSessionFile, KConfig::SimpleConfig);
//  defaultConfig.group("General").writeEntry ("Name", QString());

  saveSessionTo(&defaultConfig);

  // in case the local default.katesession did not exist, the KateSessionManager
  // constructor set the default.katesession to the global KDE directory. But
  // now we are sure the local file exists, so point to if from now on...
  m_defaultSessionFile = localSessionFile;
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
    : KDialog ( parent )
{
  setCaption( i18n ("Session Chooser") );
  setButtons( User1 | User2 | User3 );
  setButtonGuiItem( User1, KStandardGuiItem::quit() );
  setButtonGuiItem( User2, KGuiItem (i18n ("Open Session"), "document-open") );
  setButtonGuiItem( User3, KGuiItem (i18n ("New Session"), "document-new") );

  //showButtonSeparator(true);
  QFrame *page = new QFrame (this);
  QVBoxLayout *tll = new QVBoxLayout(page);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

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

  connect (m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));

  QMenu* popup = new QMenu(this);
  button(KDialog::User3)->setDelayedMenu(popup);
  QAction *a = popup->addAction(i18n("Use selected session as template"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotCopySession()));

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  kdDebug()<<"Last session is:"<<lastSession;
  for (int i = 0; i < slist.count(); ++i)
  {
    KateSessionChooserItem *item = new KateSessionChooserItem (m_sessions, slist[i]);

    kdDebug()<<"Session added to chooser:"<<slist[i]->sessionName()<<"........"<<slist[i]->sessionFileRelative();
    if (slist[i]->sessionFileRelative() == lastSession)
      m_sessions->setCurrentItem (item);
  }

  m_sessions->resizeColumnToContents(0);

  m_useLast = new QCheckBox (i18n ("&Always use this choice"), page);
  tll->addWidget(m_useLast);

  setResult (resultNone);

  // trigger action update
  selectionChanged (NULL, NULL);
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotUser1()));
  connect(this, SIGNAL(user2Clicked()), this, SLOT(slotUser2()));
  connect(m_sessions, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slotUser2()));
  connect(this, SIGNAL(user3Clicked()), this, SLOT(slotUser3()));
  enableButton (KDialog::User2, true);

  setDefaultButton(KDialog::User2);
  setEscapeButton(KDialog::User1);
  setButtonFocus(KDialog::User2);
}

KateSessionChooser::~KateSessionChooser ()
{}

void KateSessionChooser::slotCopySession()
{
  done(resultCopy);
}

KateSession::Ptr KateSessionChooser::selectedSession ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->currentItem ();

  if (!item)
    return KateSession::Ptr();

  return item->session;
}

bool KateSessionChooser::reopenLastSession ()
{
  return m_useLast->isChecked ();
}

void KateSessionChooser::slotUser2 ()
{
  done (resultOpen);
}

void KateSessionChooser::slotUser3 ()
{
  done (resultNew);
}

void KateSessionChooser::slotUser1 ()
{
  done (resultQuit);
}

void KateSessionChooser::selectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
  enableButton (KDialog::User2, current);
}

//END CHOOSER DIALOG

//BEGIN OPEN DIALOG

KateSessionOpenDialog::KateSessionOpenDialog (QWidget *parent)
    : KDialog (  parent )

{
  setCaption( i18n ("Open Session") );
  setButtons( User1 | User2 );
  setButtonGuiItem( User1, KStandardGuiItem::cancel() );
  // don't use KStandardGuiItem::open() here which has trailing ellipsis!
  setButtonGuiItem( User2, KGuiItem( i18n("&Open"), "document-open") );
  setDefaultButton(KDialog::User2);
  //showButtonSeparator(true);
  /*QFrame *page = new QFrame (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBoxLayout *hb = new QHBoxLayout (page);

  QVBoxLayout *vb = new QVBoxLayout ();
  hb->addItem(vb);*/
  m_sessions = new QTreeWidget (this);
  m_sessions->setMinimumSize(400, 200);
  setMainWidget(m_sessions);
  //vb->addWidget(m_sessions);
  QStringList header;
  header << i18n("Session Name");
  header << i18nc("The number of open documents", "Open Documents");
  m_sessions->setHeaderLabels(header);
  m_sessions->setRootIsDecorated( false );
  m_sessions->setItemsExpandable( false );
  m_sessions->setAllColumnsShowFocus( true );
  m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_sessions->setSelectionMode (QAbstractItemView::SingleSelection);

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i = 0; i < slist.count(); ++i)
  {
    new KateSessionChooserItem (m_sessions, slist[i]);
  }
  m_sessions->resizeColumnToContents(0);

  setResult (resultCancel);
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotUser1()));
  connect(this, SIGNAL(user2Clicked()), this, SLOT(slotUser2()));
  connect(m_sessions, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slotUser2()));

}

KateSessionOpenDialog::~KateSessionOpenDialog ()
{}

KateSession::Ptr KateSessionOpenDialog::selectedSession ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->currentItem ();

  if (!item)
    return KateSession::Ptr();

  return item->session;
}

void KateSessionOpenDialog::slotUser1 ()
{
  done (resultCancel);
}

void KateSessionOpenDialog::slotUser2 ()
{
  done (resultOk);
}

//END OPEN DIALOG

//BEGIN MANAGE DIALOG

KateSessionManageDialog::KateSessionManageDialog (QWidget *parent)
    : KDialog ( parent )
{
  setCaption( i18n ("Manage Sessions") );
  setButtons( User1 | User2 );
  setButtonGuiItem( User1, KStandardGuiItem::close() );
  setButtonGuiItem( User2, KGuiItem( i18n("&Open") ) );

  setDefaultButton(KDialog::User1);
  QFrame *page = new QFrame (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBoxLayout *hb = new QHBoxLayout (page);
  hb->setSpacing (KDialog::spacingHint());

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

  connect (m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));

  updateSessionList ();
  m_sessions->resizeColumnToContents(0);

  QVBoxLayout *vb = new QVBoxLayout ();
  hb->addItem(vb);
  vb->setSpacing (KDialog::spacingHint());

  m_rename = new KPushButton (i18n("&Rename..."), page);
  connect (m_rename, SIGNAL(clicked()), this, SLOT(rename()));
  vb->addWidget (m_rename);

  m_del = new KPushButton (KStandardGuiItem::del (), page);
  connect (m_del, SIGNAL(clicked()), this, SLOT(del()));
  vb->addWidget (m_del);

  vb->addStretch ();

  // trigger action update
  selectionChanged (NULL, NULL);
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotUser1()));
  connect(this, SIGNAL(user2Clicked()), this, SLOT(open()));
}

KateSessionManageDialog::~KateSessionManageDialog ()
{}

void KateSessionManageDialog::slotUser1 ()
{
  done (0);
}

void KateSessionManageDialog::selectionChanged (QTreeWidgetItem *current, QTreeWidgetItem *)
{
  const bool validItem = (current != NULL);

  m_rename->setEnabled (validItem);
  m_del->setEnabled (validItem && ((KateSessionChooserItem*)current)->session!=KateSessionManager::self()->activeSession());
  button(User2)->setEnabled (validItem);
}

void KateSessionManageDialog::rename ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->currentItem ();

  if (!item)
    return;

  bool ok = false;
  QString name = KInputDialog::getText (i18n("Specify New Name for Session"), i18n("Session name:"), item->session->sessionName(), &ok);

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
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->currentItem ();

  if (!item)
    return;

  QFile::remove (item->session->sessionFile());
  KateSessionManager::self()->updateSessionList ();
  updateSessionList ();
}

void KateSessionManageDialog::open ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->currentItem ();

  if (!item)
    return;

  hide();
  KateSessionManager::self()->activateSession (item->session);
  done(0);
}

void KateSessionManageDialog::updateSessionList ()
{
  m_sessions->clear ();

  KateSessionList &slist (KateSessionManager::self()->sessionList());
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
  connect(sessionsGroup, SIGNAL( triggered(QAction *) ), this, SLOT( openSession(QAction *)));
}

void KateSessionsAction::slotAboutToShow()
{
  qDeleteAll( sessionsGroup->actions() );

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i = 0; i < slist.count(); ++i)
  {
    QAction *action = new QAction( slist[i]->sessionName(), sessionsGroup );
    action->setData(QVariant(i));
    menu()->addAction (action);
  }
}

void KateSessionsAction::openSession (QAction *action)
{
  KateSessionList &slist (KateSessionManager::self()->sessionList());

  int i = action->data().toInt();
  if (i >= slist.count())
    return;

  KateSessionManager::self()->activateSession(slist[i]);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
