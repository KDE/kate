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

#include <KTextEditor/EditorChooser>

#include <kdebug.h>
#include <KGlobal>
#include <KIconLoader>
#include <KLocale>
#include <KConfig>
#include <kstandardaction.h>
#include <KVBox>

#include <QCheckBox>
#include <QLabel>
#include <kpushbutton.h>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFrame>
#include <QGroupBox>

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
  applicationItem->setIcon( KIcon( "preferences-other" ) );
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
  QGroupBox *buttonGroup = new QGroupBox( i18n("&Behavior"), generalFrame );
  QVBoxLayout *vbox = new QVBoxLayout;
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

  vbox->addWidget(m_modNotifications);
  buttonGroup->setLayout(vbox);

  // GROUP with the one below: "Meta-information"
  buttonGroup = new QGroupBox( i18n("Meta-Information"), generalFrame );
  vbox = new QVBoxLayout;
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

  vbox->addWidget(m_saveMetaInfos);

  // meta infos days
  KHBox *metaInfos = new KHBox( buttonGroup );
  metaInfos->setEnabled(KateDocManager::self()->getSaveMetaInfos());
  QLabel *label = new QLabel( i18n("&Delete unused meta-information after:"), metaInfos );
  m_daysMetaInfos = new QSpinBox( metaInfos );
  m_daysMetaInfos->setMaximum( 180 );
  m_daysMetaInfos->setSpecialValueText(i18n("(never)"));
  m_daysMetaInfos->setSuffix(i18n(" day(s)"));
  m_daysMetaInfos->setValue( KateDocManager::self()->getDaysMetaInfos() );
  label->setBuddy( m_daysMetaInfos );
  connect( m_saveMetaInfos, SIGNAL( toggled( bool ) ), metaInfos, SLOT( setEnabled( bool ) ) );
  connect( m_daysMetaInfos, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );

  vbox->addWidget(metaInfos);
  buttonGroup->setLayout(vbox);

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
  item->setIcon( KIcon( "view-history" ) );

  layout = new QVBoxLayout( sessionsFrame );
  layout->setMargin(0);
  layout->setSpacing(KDialog::spacingHint());

  // GROUP with the one below: "Startup"
  buttonGroup = new QGroupBox( i18n("Elements of Sessions"), sessionsFrame );
  vbox = new QVBoxLayout;
  layout->addWidget( buttonGroup );

  // restore view  config
  m_restoreVC = new QCheckBox( buttonGroup );
  m_restoreVC->setText(i18n("Include &window configuration"));
  m_restoreVC->setChecked( cgGeneral.readEntry("Restore Window Configuration", true) );
  m_restoreVC->setWhatsThis( i18n("Check this if you want all your views and frames restored each time you open Kate"));
  connect( m_restoreVC, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );
  
  vbox->addWidget(m_restoreVC);
  buttonGroup->setLayout(vbox);

  QGroupBox* sessionsStart = new QGroupBox( i18n("Behavior on Application Startup"), sessionsFrame );
  vbox = new QVBoxLayout;
  layout->addWidget(sessionsStart);

  m_startNewSessionRadioButton = new QRadioButton( i18n("&Start new session"), sessionsStart );
  m_loadLastUserSessionRadioButton = new QRadioButton( i18n("&Load last-used session"), sessionsStart );
  m_manuallyChooseSessionRadioButton = new QRadioButton( i18n("&Manually choose a session"), sessionsStart );

  QString sesStart (cgGeneral.readEntry ("Startup Session", "manual"));
  if (sesStart == "new")
    m_startNewSessionRadioButton->setChecked (true);
  else if (sesStart == "last")
    m_loadLastUserSessionRadioButton->setChecked (true);
  else
    m_manuallyChooseSessionRadioButton->setChecked (true);

  connect(m_startNewSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_loadLastUserSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_manuallyChooseSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  
  vbox->addWidget(m_startNewSessionRadioButton);
  vbox->addWidget(m_loadLastUserSessionRadioButton);
  vbox->addWidget(m_manuallyChooseSessionRadioButton);
  sessionsStart->setLayout(vbox);

  QGroupBox *sessionsExit = new QGroupBox( i18n("Behavior on Application Exit or Session Switch"), sessionsFrame );
  vbox = new QVBoxLayout;
  layout->addWidget(sessionsExit);

  m_doNotSaveSessionRadioButton = new QRadioButton( i18n("&Do not save session"), sessionsExit );
  m_saveSessionRadioButton = new QRadioButton( i18n("&Save session"), sessionsExit );
  m_askUserRadioButton = new QRadioButton( i18n("&Ask user"), sessionsExit );

  QString sesExit (cgGeneral.readEntry ("Session Exit", "save"));
  if (sesExit == "discard")
    m_doNotSaveSessionRadioButton->setChecked (true);
  else if (sesExit == "save")
    m_saveSessionRadioButton->setChecked (true);
  else
    m_askUserRadioButton->setChecked (true);

  connect(m_doNotSaveSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_saveSessionRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(m_askUserRadioButton, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  
  vbox->addWidget(m_doNotSaveSessionRadioButton);
  vbox->addWidget(m_saveSessionRadioButton);
  vbox->addWidget(m_askUserRadioButton);
  sessionsExit->setLayout(vbox);

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
  item->setIcon( KIcon( "view-list-text" ) );
  connect( filelistConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
  //END Document List page

  //BEGIN Plugins page
  KVBox *page = new KVBox();
  KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
  connect( configPluginPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );

  m_pluginPage = addSubPage( applicationItem, page, i18n("Plugins") );
  m_pluginPage->setHeader( i18n("Plugin Manager") );
  m_pluginPage->setIcon( KIcon( "preferences-plugin" ) );

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
  editorItem->setIcon( KIcon( "accessories-text-editor" ) );
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

    if (m_startNewSessionRadioButton->isChecked())
      cg.writeEntry ("Startup Session", "new");
    else if (m_loadLastUserSessionRadioButton->isChecked())
      cg.writeEntry ("Startup Session", "last");
    else
      cg.writeEntry ("Startup Session", "manual");

    if (m_doNotSaveSessionRadioButton->isChecked())
      cg.writeEntry ("Session Exit", "discard");
    else if (m_saveSessionRadioButton->isChecked())
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

