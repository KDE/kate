/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

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

#include "kateconfigdialog.h"
#include "kateconfigdialog.moc"

#include "katemainwindow.h"

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katefilelist.h"

#include <KTextEditor/ConfigPage>
#include <KTextEditor/EditorChooser>

#include <KComponentData>
#include <kdebug.h>
#include <KGlobal>
#include <KGlobalSettings>
#include <KIconLoader>
#include <KListWidget>
#include <KLocale>
#include <KConfig>
#include <kstandardaction.h>
#include <KStandardDirs>
#include <KWindowSystem>
#include <KSeparator>
#include <KPageWidgetModel>
#include <KVBox>

#include <q3buttongroup.h>
#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <kpushbutton.h>
#include <QRadioButton>
#include <QSpinBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QFrame>

KateConfigDialog::KateConfigDialog ( KateMainWindow *parent, KTextEditor::View *view )
    : KPageDialog( parent )
    , m_mainWindow( parent )
    , m_view( view )
{
  setFaceType( Tree );
  setCaption( i18n("Configure") );
  setButtons( Ok | Apply | Cancel | Help );
  setDefaultButton( Ok );
  setObjectName( "configdialog" );

  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup cgGeneral = KConfigGroup( config, "General" );

  enableButton( Apply, false );

  KPageWidgetItem *applicationItem = addPage( new QWidget, i18n("Application") );
  applicationItem->setIcon( KIcon( "kate" ) );
  applicationItem->setHeader( i18n("Application Options") );

  //BEGIN General page
  QFrame* generalFrame = new QFrame;
  KPageWidgetItem *item = addSubPage( applicationItem, generalFrame, i18n("General") );
  item->setHeader( i18n("General Options") );
  item->setIcon( KIcon( "go-home" ) );

  QVBoxLayout *layout = new QVBoxLayout( generalFrame );
  layout->setMargin(0);
  layout->setSpacing(KDialog::spacingHint());

  // GROUP with the one below: "Behavior"
  Q3ButtonGroup *buttonGroup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("&Behavior"), generalFrame );
  layout->addWidget( buttonGroup );

  // modified files notification
  m_modNotifications = new QCheckBox(
                          i18n("Wa&rn about files modified by foreign processes"), buttonGroup );
  m_modNotifications->setChecked( parent->modNotification );
  m_modNotifications->setWhatsThis( i18n(
                                       "If enabled, when Kate receives focus you will be asked what to do with "
                                       "files that have been modified on the hard disk. If not enabled, you will "
                                       "be asked what to do with a file that has been modified on the hard disk only "
                                       "when that file gains focus inside Kate.") );
  connect( m_modNotifications, SIGNAL( toggled( bool ) ),
           this, SLOT( slotChanged() ) );

  // GROUP with the one below: "Meta-information"
  buttonGroup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Meta-Information"), generalFrame );
  layout->addWidget( buttonGroup );

  // save meta infos
  m_saveMetaInfos = new QCheckBox( buttonGroup );
  m_saveMetaInfos->setText(i18n("Keep &meta-information past sessions"));
  m_saveMetaInfos->setChecked(KateDocManager::self()->getSaveMetaInfos());
  m_saveMetaInfos->setWhatsThis( i18n(
                                    "Check this if you want document configuration like for example "
                                    "bookmarks to be saved past editor sessions. The configuration will be "
                                    "restored if the document has not changed when reopened."));
  connect( m_saveMetaInfos, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // meta infos days
  KHBox *metaInfos = new KHBox( buttonGroup );
  metaInfos->setEnabled(KateDocManager::self()->getSaveMetaInfos());
  m_daysMetaInfos = new QSpinBox( metaInfos );
  m_daysMetaInfos->setMaximum( 180 );
  m_daysMetaInfos->setSpecialValueText(i18n("(never)"));
  m_daysMetaInfos->setSuffix(i18n(" day(s)"));
  m_daysMetaInfos->setValue( KateDocManager::self()->getDaysMetaInfos() );
  QLabel *label = new QLabel( i18n("&Delete unused meta-information after:"), metaInfos );
  label->setBuddy( m_daysMetaInfos );
  connect( m_saveMetaInfos, SIGNAL( toggled( bool ) ), metaInfos, SLOT( setEnabled( bool ) ) );
  connect( m_daysMetaInfos, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );


  // editor component
  m_editorChooser = new KTextEditor::EditorChooser(generalFrame);
  m_editorChooser->readAppSetting();
  connect(m_editorChooser, SIGNAL(changed()), this, SLOT(slotChanged()));
  layout->addWidget(m_editorChooser);
  layout->addStretch(1); // :-] works correct without autoadd

  //END General page


  //BEGIN Session page
  QFrame* sessionsFrame = new QFrame;
  item = addSubPage( applicationItem, sessionsFrame, i18n("Sessions") );
  item->setHeader( i18n("Session Management") );
  item->setIcon( KIcon( "history" ) );

  layout = new QVBoxLayout( sessionsFrame );
  layout->setMargin(0);
  layout->setSpacing(KDialog::spacingHint());

  // GROUP with the one below: "Startup"
  buttonGroup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Elements of Sessions"), sessionsFrame );
  layout->addWidget( buttonGroup );

  // restore view  config
  m_restoreVC = new QCheckBox( buttonGroup );
  m_restoreVC->setText(i18n("Include &window configuration"));
  m_restoreVC->setChecked( cgGeneral.readEntry("Restore Window Configuration", true) );
  m_restoreVC->setWhatsThis( i18n("Check this if you want all your views and frames restored each time you open Kate"));
  connect( m_restoreVC, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  QRadioButton *rb1, *rb2, *rb3;

  m_sessionsStart = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Behavior on Application Startup"), sessionsFrame );
  layout->addWidget(m_sessionsStart);

  m_sessionsStart->setRadioButtonExclusive( true );
  m_sessionsStart->insert( rb1 = new QRadioButton( i18n("&Start new session"), m_sessionsStart ), 0 );
  m_sessionsStart->insert( rb2 = new QRadioButton( i18n("&Load last-used session"), m_sessionsStart ), 1 );
  m_sessionsStart->insert( rb3 = new QRadioButton( i18n("&Manually choose a session"), m_sessionsStart ), 2 );

  QString sesStart (cgGeneral.readEntry ("Startup Session", "manual"));
  if (sesStart == "new")
    m_sessionsStart->setButton (0);
  else if (sesStart == "last")
    m_sessionsStart->setButton (1);
  else
    m_sessionsStart->setButton (2);

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  m_sessionsExit = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Behavior on Application Exit or Session Switch"), sessionsFrame );
  layout->addWidget(m_sessionsExit);

  m_sessionsExit->setRadioButtonExclusive( true );
  m_sessionsExit->insert( rb1 = new QRadioButton( i18n("&Do not save session"), m_sessionsExit ), 0 );
  m_sessionsExit->insert( rb2 = new QRadioButton( i18n("&Save session"), m_sessionsExit ), 1 );
  m_sessionsExit->insert( rb3 = new QRadioButton( i18n("&Ask user"), m_sessionsExit ), 2 );

  QString sesExit (cgGeneral.readEntry ("Session Exit", "save"));
  if (sesExit == "discard")
    m_sessionsExit->setButton (0);
  else if (sesExit == "save")
    m_sessionsExit->setButton (1);
  else
    m_sessionsExit->setButton (2);

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  layout->addStretch(1); // :-] works correct without autoadd
  //END Session page

  // file selector page
#if 0
  path << i18n("Application") << i18n("File Selector");

  KVBox *page = addVBoxPage( path, i18n("File Selector Settings"),
                             BarIcon("document-open", KIconLoader::SizeSmall) );
  fileSelConfigPage = new KFSConfigPage( page, "file selector config page",
                                         m_mainWindow->fileselector );
  connect( fileSelConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
  path.clear();
#endif
#ifdef __GNUC__
#warning portme
#endif
#if 0
  filelistConfigPage = new KFLConfigPage( page, m_mainWindow->filelist );
  connect( filelistConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
#endif

  //BEGIN Document List page
  filelistConfigPage = new KateFileListConfigPage(this, m_mainWindow->m_fileList);
  item = addSubPage( applicationItem, filelistConfigPage, i18n("Document List") );
  item->setHeader( i18n("Document List Settings") );
  item->setIcon( KIcon( "fileview-text" ) );
  connect( filelistConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
  //END Document List page

  //BEGIN Plugins page
  KVBox *page = new KVBox();
  KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
  connect( configPluginPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );

  m_pluginPage = addSubPage( applicationItem, page, i18n("Plugins") );
  m_pluginPage->setHeader( i18n("Plugin Manager") );
  m_pluginPage->setIcon( KIcon( "connection-established" ) );

  KatePluginList &pluginList (KatePluginManager::self()->pluginList());
  foreach (const KatePluginInfo &plugin, pluginList)
  {
    if  ( plugin.load
          && Kate::pluginConfigPageInterface(plugin.plugin) )
      addPluginPage (plugin.plugin);
  }
  //END Plugins page

  //BEGIN Editors page
  // editor widgets from kwrite/kwdialog
  KPageWidgetItem *editorItem = addPage( new QWidget, i18n("Editor Component") );
  editorItem->setIcon( KIcon( "edit" ) );
  editorItem->setHeader( i18n("Editor Component Options") );

  for (int i = 0; i < KateDocManager::self()->editor()->configPages (); ++i)
  {
    page = new KVBox();
    KTextEditor::ConfigPage *cPage = KateDocManager::self()->editor()->configPage(i, page);
    connect( cPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
    m_editorPages.append (cPage);

    item = addSubPage( editorItem, page, KateDocManager::self()->editor()->configPageName(i) );
    item->setHeader( KateDocManager::self()->editor()->configPageFullName(i) );
    item->setIcon( KateDocManager::self()->editor()->configPageIcon(i) );
  }
  //END Editors page

  connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
  connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));
  m_dataChanged = false;

  resize(minimumSizeHint());
}

KateConfigDialog::~KateConfigDialog()
{}

void KateConfigDialog::addPluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigPageInterface(plugin))
    return;

  for (uint i = 0; i < Kate::pluginConfigPageInterface(plugin)->configPages(); i++)
  {
    KVBox *page = new KVBox();

    KPageWidgetItem *item = addSubPage( m_pluginPage, page, Kate::pluginConfigPageInterface(plugin)->configPageName(i) );
    item->setHeader( Kate::pluginConfigPageInterface(plugin)->configPageFullName(i) );
    item->setIcon( Kate::pluginConfigPageInterface(plugin)->configPageIcon(i));

    PluginPageListItem *info = new PluginPageListItem;
    info->plugin = plugin;
    info->page = Kate::pluginConfigPageInterface(plugin)->configPage (i, page);
    info->pageWidgetItem = item;
    connect( info->page, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
    m_pluginPages.append(info);
  }
}

void KateConfigDialog::removePluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigPageInterface(plugin))
    return;

  for (int i = 0; i < m_pluginPages.count(); i++)
  {
    if  ( m_pluginPages[i]->plugin == plugin )
    {
      QWidget *w = m_pluginPages[i]->page->parentWidget();
      delete m_pluginPages[i]->page;
      delete w;
      removePage(m_pluginPages[i]->pageWidgetItem);
      m_pluginPages.removeAt(i);
      i--;
    }
  }
}

void KateConfigDialog::slotOk()
{
  slotApply();
  accept();
}

void KateConfigDialog::slotApply()
{
  KSharedConfig::Ptr config = KGlobal::config();

  // if data changed apply the kate app stuff
  if( m_dataChanged )
  {
    KConfigGroup cg = KConfigGroup( config, "General" );

    cg.writeEntry("Restore Window Configuration", m_restoreVC->isChecked());

    int bu = m_sessionsStart->id (m_sessionsStart->selected());

    if (bu == 0)
      cg.writeEntry ("Startup Session", "new");
    else if (bu == 1)
      cg.writeEntry ("Startup Session", "last");
    else
      cg.writeEntry ("Startup Session", "manual");

    bu = m_sessionsExit->id (m_sessionsExit->selected());

    if (bu == 0)
      cg.writeEntry ("Session Exit", "discard");
    else if (bu == 1)
      cg.writeEntry ("Session Exit", "save");
    else
      cg.writeEntry ("Session Exit", "ask");

    m_editorChooser->writeAppSetting();

    cg.writeEntry("Save Meta Infos", m_saveMetaInfos->isChecked());
    KateDocManager::self()->setSaveMetaInfos(m_saveMetaInfos->isChecked());

    cg.writeEntry("Days Meta Infos", m_daysMetaInfos->value() );
    KateDocManager::self()->setDaysMetaInfos(m_daysMetaInfos->value());

    cg.writeEntry("Modified Notification", m_modNotifications->isChecked());
    m_mainWindow->modNotification = m_modNotifications->isChecked();

#ifdef __GNUC__
#warning portme
#endif
    filelistConfigPage->apply();
    /*
        KateExternalToolsCommand::self()->reload();
        for (int i=0; i < KateApp::self()->mainWindows(); i++)
        {
          KateMainWindow *win = KateApp::self()->mainWindow (i);
          win->externalTools->reload();
        }*/
    //m_mainWindow->externalTools->reload();

    m_mainWindow->saveOptions ();

    // save plugin config !!
    KateApp::self()->pluginManager()->writeConfig (KGlobal::config().data());
  }

  foreach (KTextEditor::ConfigPage *page, m_editorPages)
  {
      page->apply();
  }

  // write back the editor config
  KateDocManager::self()->editor()->writeConfig(config.data());

  foreach (PluginPageListItem *plugin, m_pluginPages)
  {
    plugin->page->apply();
  }

  config->sync();

  m_dataChanged = false;
  enableButton( Apply, false );
}

void KateConfigDialog::slotChanged()
{
  m_dataChanged = true;
  enableButton( Apply, true );
}

// kate: space-indent on; indent-width 2; replace-tabs on;

