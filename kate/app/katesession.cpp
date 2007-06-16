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

#include "katetooltipmenu.h"

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

#include <QDir>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QtAlgorithms>

#include <unistd.h>
#include <time.h>

// used to sort the session list (qSort)
bool caseInsensitiveLessThan(const KateSession::Ptr& a, const KateSession::Ptr& b)
{
  return a->sessionName().toLower() < b->sessionName().toLower();
}

KateSession::KateSession (KateSessionManager *manager, const QString &fileName)
    : m_sessionFileRel (fileName)
    , m_documents (0)
    , m_manager (manager)
    , m_readConfig (0)
    , m_writeConfig (0)
{
  m_sessionName = QUrl::fromPercentEncoding(fileName.toLatin1());
  m_sessionName.chop(12);//.katesession==12
  init ();
}

void KateSession::init ()
{
  // given file exists, use it to load some stuff ;)
  if (!m_sessionFileRel.isEmpty() && KGlobal::dirs()->exists(sessionFile ()))
  {
    KConfig config (sessionFile (), KConfig::OnlyLocal);

    // get the document count
    m_documents = config.group("Open Documents").readEntry("Count", 0);

    return;
  }

  if (!m_sessionFileRel.isEmpty() && !KGlobal::dirs()->exists(sessionFile ()))
    kDebug() << "Warning, session file not found: " << m_sessionFileRel << endl;
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
#if 0
  // get a usable filename
  int s = time(0);
  QByteArray tname;
  //USE QUrl::toPercentEncoding("{a fishy string?}", "", "."); instead
  while (true)
  {
    tname.setNum (s++);
    KMD5 md5 (tname);
    m_sessionFileRel = QString ("%1.katesession").arg (md5.hexDigest().data());

    if (!KGlobal::dirs()->exists(sessionFile ()))
      break;
  }
#else
  m_sessionFileRel = QUrl::toPercentEncoding(name, "", ".") + QString(".katesession");
  if (KGlobal::dirs()->exists(sessionFile ()))
  {
    m_sessionFileRel = oldSessionFileRel;
    //Q_ASSERT(!KGlobal::dirs()->exists(sessionFile ()));
    return false;
  }
#endif



  // create the file, write name to it!
  KConfig config (sessionFile (), KConfig::OnlyLocal);
  config.group("General").writeEntry ("Name", m_sessionName);
  config.sync ();

  // reinit ourselfs ;)
  init ();

  return true;
}

bool KateSession::rename (const QString &name)
{
  if (name.isEmpty () || m_sessionFileRel.isEmpty() || m_sessionFileRel == m_manager->defaultSessionFileName())
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
  if (!KIO::NetAccess::file_move(KUrl(QString("file://") + oldSessionFile), KUrl(QString("file://") + sessionFile())) )
  {
    m_sessionFileRel = oldRel;
    return false;
  }
  m_sessionName = name;

  return true;
}

KConfig *KateSession::configRead ()
{
  if (m_sessionFileRel.isEmpty())
    return 0;

  if (m_readConfig)
    return m_readConfig;

  return m_readConfig = new KConfig (sessionFile (), KConfig::OnlyLocal);
}

KConfig *KateSession::configWrite ()
{
  if (m_sessionFileRel.isEmpty())
    return 0;

  if (m_writeConfig)
    return m_writeConfig;

  m_writeConfig = new KConfig (sessionFile (), KConfig::OnlyLocal);
  m_writeConfig->group("General").writeEntry ("Name", m_sessionName);

  return m_writeConfig;
}

KateSessionManager::KateSessionManager (QObject *parent)
    : QObject (parent)
    , m_sessionsDir (KStandardDirs::locateLocal( "data", "kate/sessions"))
    , m_activeSession (new KateSession (this, ""))
    , m_defaultSessionFileName(QUrl::toPercentEncoding(i18n("Default Session"), "", ".") + QString(".katesession"))
{
  kDebug() << "LOCAL SESSION DIR: " << m_sessionsDir << endl;

  // create dir if needed
  KGlobal::dirs()->makeDir (m_sessionsDir);
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

  bool foundDefault = false;
  kDebug() << "default session file name:" << m_defaultSessionFileName << endl;
  for (unsigned int i = 0; i < dir.count(); ++i)
  {
    KateSession *session = new KateSession (this, dir[i]);
    m_sessionList.append (KateSession::Ptr(session));

    kDebug () << "FOUND SESSION: " << session->sessionName() << " FILE: " << session->sessionFile() << " dir[i];" << dir[i] << endl;
    if (!foundDefault && (dir[i] == m_defaultSessionFileName))
      foundDefault = true;
  }

  // add default session, if not there
  if (!foundDefault)
  {
    KSharedPtr<KateSession> s(new KateSession (this, ""));
    s->create(i18n("Default Session"));
    m_sessionList.append (s);
  }

  qSort(m_sessionList.begin(), m_sessionList.end(), caseInsensitiveLessThan);
}

void KateSessionManager::activateSession (KateSession::Ptr session, bool closeLast, bool saveLast, bool loadNew)
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

    // load plugin config + plugins
    KatePluginManager::self()->loadConfig (sc);

    if (sc)
      KateApp::self()->documentManager()->restoreDocumentList (sc);

    // if we have no session config object, try to load the default
    // (anonymous/unnamed sessions)
    if ( ! sc )
      sc = new KConfig( sessionsDir() + "/default.katesession", KConfig::OnlyLocal );

    // window config
    KConfigGroup c(KGlobal::config(), "General");

    if (c.readEntry("Restore Window Configuration", true))
    {
      // a new, named session, read settings of the default session.
      if ( ! sc->hasGroup("Open MainWindows") )
        sc = new KConfig( sessionsDir() + "/default.katesession", KConfig::OnlyLocal );

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
      }

      // remove mainwindows we need no longer...
      if (wCount > 0)
      {
        while (wCount < KateApp::self()->mainWindows())
          delete KateApp::self()->mainWindow(KateApp::self()->mainWindows() - 1);
      }
    }
  }
}

KateSession::Ptr KateSessionManager::giveSession (const QString &name)
{
  if (name.isEmpty())
    return KateSession::Ptr(new KateSession (this, ""));

  updateSessionList();

  for (int i = 0; i < m_sessionList.count(); ++i)
  {
    if (m_sessionList[i]->sessionName() == name)
      return m_sessionList[i];
  }

  KateSession::Ptr s(new KateSession (this, ""));
  s->create (name);
  return s;
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
      KDialog *dlg = new KDialog ( 0 );
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
        c.changeGroup("General");

        if (res == KDialog::No)
          c.writeEntry ("Session Exit", "discard");
        else
          c.writeEntry ("Session Exit", "save");
      }

      if (res == KDialog::No)
        return true;
    }
  }

  KConfig *sc = activeSession()->configWrite();

  if (!sc)
    return false;

  // save plugin configs and which plugins to load
  KatePluginManager::self()->writeConfig(sc);

  // save document configs + which documents to load
  KateDocManager::self()->saveDocumentList (sc);

  sc->group("Open MainWindows").writeEntry ("Count", KateApp::self()->mainWindows ());

  // save config for all windows around ;)
  for (int i = 0; i < KateApp::self()->mainWindows (); ++i )
  {
    KConfigGroup cg(sc, QString ("MainWindow%1").arg(i) );
    KateApp::self()->mainWindow(i)->saveProperties (cg);
  }

  sc->sync();

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
  QString lastSession (c.readEntry ("Last Session", m_defaultSessionFileName));
  QString sesStart (c.readEntry ("Startup Session", "manual"));

  // uhh, just open last used session, show no chooser
  if (sesStart == "last")
  {
    activateSession (KateSession::Ptr(new KateSession (this, lastSession)), false, false);
    return success;
  }

  // start with empty new session
  if (sesStart == "new")
  {
    activateSession (KateSession::Ptr(new KateSession (this, "")), false, false);
    return success;
  }

  QList<KateSessionChooserTemplate> templates;
  templates.append(KateSessionChooserTemplate("Profile 1 (default)", "blah.desktop", "<img source=\"/home/jowenn/development/kde/binary/share/icons/hicolor/32x32/actions/show_side_panel.png\"><b>Test 1</b> adfafsdfdsf<br>asdfasdfsdfsdf adsfad adf asdf df<br> adfasdf jasdlfjö"));
  templates.append(KateSessionChooserTemplate("Profile 2", "blah.desktop", " Test 2"));
  templates.append(KateSessionChooserTemplate("Profile 3", "blah.desktop", " Test 3"));
  KateSessionChooser *chooser = new KateSessionChooser (0, lastSession, templates);

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

      default:
        activateSession (KateSession::Ptr(new KateSession (this, "")), false, false) ;
        retry = false;
        break;
    }
  }

  // write back our nice boolean :)
  if (success && chooser->reopenLastSession ())
  {
    c.changeGroup("General");

    if (res == KateSessionChooser::resultOpen)
      c.writeEntry ("Startup Session", "last");
    else if (res == KateSessionChooser::resultNew)
      c.writeEntry ("Startup Session", "new");

    c.sync ();
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
  bool loop = false;
  QString name;
  do
  {
    bool ok = false;
    name = KInputDialog::getText (
             i18n("Specify New Name for Current Session"),
             loop ? i18n("There is already an existing session with your chosen name.\nPlease choose a different one\nSession name:") : i18n("Session name:")
             , name, &ok);

    if (!ok)
      return;

    if (name.isEmpty())
    {
      KMessageBox::sorry (0, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));
      return;
    }
    loop = true;
  }
  while (!activeSession()->create (name, true));
  saveActiveSession ();
}


void KateSessionManager::sessionManage ()
{
  KateSessionManageDialog *dlg = new KateSessionManageDialog (0);

  dlg->exec ();

  delete dlg;
}

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

KateSessionChooser::KateSessionChooser (QWidget *parent, const QString &lastSession,
                                        const QList<KateSessionChooserTemplate> &templates)
    : KDialog ( parent )
    , m_templates(templates)
{
  setCaption( i18n ("Session Chooser") );
  setButtons( User1 | User2 | User3 );
  setButtonGuiItem( User1, KStandardGuiItem::quit() );
  setButtonGuiItem( User2, KGuiItem (i18n ("Open Session"), "document-open") );
  setButtonGuiItem( User3, KGuiItem (i18n ("New Session"), "document-new") );

  setDefaultButton(KDialog::User2);
  setEscapeButton(KDialog::User1);
  //showButtonSeparator(true);
  QFrame *page = new QFrame (this);
  QHBoxLayout *tll = new QHBoxLayout(page);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBoxLayout *hb = new QHBoxLayout ();
  hb->setSpacing (KDialog::spacingHint());
  tll->addItem(hb);

  QLabel *label = new QLabel (page);
  hb->addWidget(label);
  label->setPixmap (UserIcon("sessionchooser"));
  label->setFrameStyle (QFrame::Panel | QFrame::Sunken);
  label->setAlignment(Qt::AlignTop);
  QVBoxLayout *vb = new QVBoxLayout ();
  vb->setSpacing (KDialog::spacingHint());
  tll->addItem(vb);

  m_sessions = new QTreeWidget (page);
  vb->addWidget(m_sessions);
  QStringList header;
  header << i18n("Session Name");
  header << i18nc("The number of open documents", "Open Documents");
  m_sessions->setHeaderLabels(header);
  m_sessions->setRootIsDecorated( false );
  m_sessions->setItemsExpandable( false );
  m_sessions->setSelectionBehavior(QAbstractItemView::SelectItems);
  m_sessions->setSelectionMode (QAbstractItemView::SingleSelection);

  connect (m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i = 0; i < slist.count(); ++i)
  {
    KateSessionChooserItem *item = new KateSessionChooserItem (m_sessions, slist[i]);

    if (slist[i]->sessionFileRelative() == lastSession)
      m_sessions->setCurrentItem (item);
  }

  m_useLast = new QCheckBox (i18n ("&Always use this choice"), page);
  vb->addWidget(m_useLast);

  setResult (resultNone);

  // trigger action update
  selectionChanged (NULL, NULL);
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotUser1()));
  connect(this, SIGNAL(user2Clicked()), this, SLOT(slotUser2()));
  connect(m_sessions, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(slotUser2()));
  connect(this, SIGNAL(user3Clicked()), this, SLOT(slotUser3()));
  enableButton (KDialog::User2, true);
  m_popup = new KateToolTipMenu(this);
  if (templates.count() > 1)
    button(KDialog::User3)->setMenu(m_popup);

  connect(m_popup, SIGNAL(aboutToShow()), this, SLOT(slotProfilePopup()));
  connect(m_popup, SIGNAL(triggered(QAction *)), this, SLOT(slotTemplateAction(QAction*)));
}

KateSessionChooser::~KateSessionChooser ()
{}

void KateSessionChooser::slotProfilePopup()
{
  m_popup->clear();
  bool defaultA = true;
  foreach(const KateSessionChooserTemplate &t, m_templates)
  {
    QAction *a = m_popup->addAction(t.displayName);
    if (defaultA) m_popup->setDefaultAction(a);
    defaultA = false;
    a->setToolTip(t.toolTip);
    a->setData(t.configFileName);
  }
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
  if (m_templates.count() > 0) m_selectedTemplate = m_templates[0].configFileName;
  done (resultNew);
}
void KateSessionChooser::slotTemplateAction(QAction* a)
{
  m_selectedTemplate = a->data().toString();
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
  m_sessions->setSelectionBehavior(QAbstractItemView::SelectItems);
  m_sessions->setSelectionMode (QAbstractItemView::SingleSelection);

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i = 0; i < slist.count(); ++i)
  {
    new KateSessionChooserItem (m_sessions, slist[i]);
  }

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
  setButtons( User1 );
  setButtonGuiItem( User1, KStandardGuiItem::close() );

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
  m_sessions->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_sessions->setSelectionMode (QAbstractItemView::SingleSelection);

  connect (m_sessions, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));

  updateSessionList ();

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

}

KateSessionManageDialog::~KateSessionManageDialog ()
{}

void KateSessionManageDialog::slotUser1 ()
{
  done (0);
}

void KateSessionManageDialog::selectionChanged (QTreeWidgetItem *current, QTreeWidgetItem *)
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) current;

  QString defFileName = KateSessionManager::self()->defaultSessionFileName();
  m_rename->setEnabled (item && item->session->sessionFileRelative() != defFileName);
  m_del->setEnabled (item && item->session->sessionFileRelative() != defFileName);
}

void KateSessionManageDialog::rename ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->currentItem ();

  if (!item || item->session->sessionFileRelative() == KateSessionManager::self()->defaultSessionFileName())
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

  if (item->session->rename (name))
    updateSessionList ();
  else
    KMessageBox::sorry(this, i18n("The session could not be renamed to \"%1\", there already exists another session with the same name", name), i18n("Session Renaming"));

}

void KateSessionManageDialog::del ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->currentItem ();

  if (!item || item->session->sessionFileRelative() == KateSessionManager::self()->defaultSessionFileName())
    return;

  QFile::remove (item->session->sessionFile());
  KateSessionManager::self()->updateSessionList ();
  updateSessionList ();
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
