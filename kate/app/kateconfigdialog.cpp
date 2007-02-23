/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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
#include <KKeyDialog>
#include <KListWidget>
#include <KLocale>
#include <KConfig>
#include <kstandardaction.h>
#include <KStandardDirs>
#include <KWin>
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
{
  setFaceType( Tree );
  setCaption( i18n("Configure") );
  setButtons( Ok | Apply | Cancel | Help );
  setDefaultButton( Ok );
  setObjectName( "configdialog" );

  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup cgGeneral = KConfigGroup( config, "General" );

  enableButton( Apply, false );

  mainWindow = parent;

  setMinimumSize(600, 400);

  v = view;

  KPageWidgetItem *applicationItem = addPage( new QWidget, i18n("Application") );
  applicationItem->setIcon( KIcon( "kate" ) );
  applicationItem->setHeader( i18n("Application Options") );

  //BEGIN General page
  QFrame* frGeneral = new QFrame();
  KPageWidgetItem *item = addSubPage( applicationItem, frGeneral, i18n("General") );
  item->setHeader( i18n("General Options") );
  item->setIcon( KIcon( "gohome" ) );

  QVBoxLayout *lo = new QVBoxLayout( frGeneral );
  lo->setSpacing(KDialog::spacingHint());

  // GROUP with the one below: "Behavior"
  Q3ButtonGroup *bgStartup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("&Behavior"), frGeneral );
  lo->addWidget( bgStartup );

  // modified files notification
  cb_modNotifications = new QCheckBox(
                          i18n("Wa&rn about files modified by foreign processes"), bgStartup );
  cb_modNotifications->setChecked( parent->modNotification );
  cb_modNotifications->setWhatsThis( i18n(
                                       "If enabled, when Kate receives focus you will be asked what to do with "
                                       "files that have been modified on the hard disk. If not enabled, you will "
                                       "be asked what to do with a file that has been modified on the hard disk only "
                                       "when that file gains focus inside Kate.") );
  connect( cb_modNotifications, SIGNAL( toggled( bool ) ),
           this, SLOT( slotChanged() ) );

  // GROUP with the one below: "Meta-information"
  bgStartup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Meta-Information"), frGeneral );
  lo->addWidget( bgStartup );

  // save meta infos
  cb_saveMetaInfos = new QCheckBox( bgStartup );
  cb_saveMetaInfos->setText(i18n("Keep &meta-information past sessions"));
  cb_saveMetaInfos->setChecked(KateDocManager::self()->getSaveMetaInfos());
  cb_saveMetaInfos->setWhatsThis( i18n(
                                    "Check this if you want document configuration like for example "
                                    "bookmarks to be saved past editor sessions. The configuration will be "
                                    "restored if the document has not changed when reopened."));
  connect( cb_saveMetaInfos, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // meta infos days
  KHBox *hbDmf = new KHBox( bgStartup );
  hbDmf->setEnabled(KateDocManager::self()->getSaveMetaInfos());
  QLabel *lDmf = new QLabel( i18n("&Delete unused meta-information after:"), hbDmf );
  sb_daysMetaInfos = new QSpinBox( hbDmf );
  sb_daysMetaInfos->setMaximum( 180 );
  sb_daysMetaInfos->setSpecialValueText(i18n("(never)"));
  sb_daysMetaInfos->setSuffix(i18n(" day(s)"));
  sb_daysMetaInfos->setValue( KateDocManager::self()->getDaysMetaInfos() );
  lDmf->setBuddy( sb_daysMetaInfos );
  connect( cb_saveMetaInfos, SIGNAL( toggled( bool ) ), hbDmf, SLOT( setEnabled( bool ) ) );
  connect( sb_daysMetaInfos, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );


  // editor component
  m_editorChooser = new KTextEditor::EditorChooser(frGeneral);
  m_editorChooser->readAppSetting();
  connect(m_editorChooser, SIGNAL(changed()), this, SLOT(slotChanged()));
  lo->addWidget(m_editorChooser);
  lo->addStretch(1); // :-] works correct without autoadd

  //END General page


  //BEGIN Session page
  QFrame* frSessions = new QFrame();
  item = addSubPage( applicationItem, frSessions, i18n("Sessions") );
  item->setHeader( i18n("Session Management") );
  item->setIcon( KIcon( "history" ) );

  lo = new QVBoxLayout( frSessions );
  lo->setSpacing(KDialog::spacingHint());

  // GROUP with the one below: "Startup"
  bgStartup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Elements of Sessions"), frSessions );
  lo->addWidget( bgStartup );

  // restore view  config
  cb_restoreVC = new QCheckBox( bgStartup );
  cb_restoreVC->setText(i18n("Include &window configuration"));
  cb_restoreVC->setChecked( cgGeneral.readEntry("Restore Window Configuration", true) );
  cb_restoreVC->setWhatsThis( i18n(
                                "Check this if you want all your views and frames restored each time you open Kate"));
  connect( cb_restoreVC, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  QRadioButton *rb1, *rb2, *rb3;

  sessions_start = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Behavior on Application Startup"), frSessions );
  lo->addWidget(sessions_start);

  sessions_start->setRadioButtonExclusive( true );
  sessions_start->insert( rb1 = new QRadioButton( i18n("&Start new session"), sessions_start ), 0 );
  sessions_start->insert( rb2 = new QRadioButton( i18n("&Load last-used session"), sessions_start ), 1 );
  sessions_start->insert( rb3 = new QRadioButton( i18n("&Manually choose a session"), sessions_start ), 2 );

  QString sesStart (cgGeneral.readEntry ("Startup Session", "manual"));
  if (sesStart == "new")
    sessions_start->setButton (0);
  else if (sesStart == "last")
    sessions_start->setButton (1);
  else
    sessions_start->setButton (2);

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  sessions_exit = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Behavior on Application Exit or Session Switch"), frSessions );
  lo->addWidget(sessions_exit);

  sessions_exit->setRadioButtonExclusive( true );
  sessions_exit->insert( rb1 = new QRadioButton( i18n("&Do not save session"), sessions_exit ), 0 );
  sessions_exit->insert( rb2 = new QRadioButton( i18n("&Save session"), sessions_exit ), 1 );
  sessions_exit->insert( rb3 = new QRadioButton( i18n("&Ask user"), sessions_exit ), 2 );

  QString sesExit (cgGeneral.readEntry ("Session Exit", "save"));
  if (sesExit == "discard")
    sessions_exit->setButton (0);
  else if (sesExit == "save")
    sessions_exit->setButton (1);
  else
    sessions_exit->setButton (2);

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  lo->addStretch(1); // :-] works correct without autoadd
  //END Session page

  // file selector page
#if 0
  path << i18n("Application") << i18n("File Selector");

  KVBox *page = addVBoxPage( path, i18n("File Selector Settings"),
                             BarIcon("fileopen", K3Icon::SizeSmall) );
  fileSelConfigPage = new KFSConfigPage( page, "file selector config page",
                                         mainWindow->fileselector );
  connect( fileSelConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
  path.clear();
#endif
  KVBox *page = new KVBox();
#ifdef __GNUC__
#warning portme
#endif
#if 0
  filelistConfigPage = new KFLConfigPage( page, mainWindow->filelist );
  connect( filelistConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
#endif
  item = addSubPage( applicationItem, page, i18n("Document List") );
  item->setHeader( i18n("Document List Settings") );
  item->setIcon( KIcon( "view_text" ) );

  page = new KVBox();
  KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
  connect( configPluginPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );

  KPageWidgetItem *pluginItem = addSubPage( applicationItem, page, i18n("Plugins") );
  pluginItem->setHeader( i18n("Plugin Manager") );
  pluginItem->setIcon( KIcon( "connect_established" ) );

  // editor widgets from kwrite/kwdialog
  KPageWidgetItem *editorItem = addPage( new QWidget, i18n("Editor Component") );
  editorItem->setIcon( KIcon( "edit" ) );
  editorItem->setHeader( i18n("Editor Component Options") );

  for (int i = 0; i < KateDocManager::self()->editor()->configPages (); ++i)
  {
    page = new KVBox();
    KTextEditor::ConfigPage *cPage = KateDocManager::self()->editor()->configPage(i, page);
    connect( cPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
    editorPages.append (cPage);

    item = addSubPage( editorItem, page, KateDocManager::self()->editor()->configPageName(i) );
    item->setHeader( KateDocManager::self()->editor()->configPageFullName(i) );
    item->setIcon( KateDocManager::self()->editor()->configPageIcon(i) );
  }

  KatePluginList &pluginList (KatePluginManager::self()->pluginList());
  foreach (const KatePluginInfo &plugin, pluginList)
  {
    if  ( plugin.load
          && Kate::pluginConfigPageInterface(plugin.plugin) )
      addPluginPage (plugin.plugin, pluginItem);
  }
  connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
  connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));
  //showButtonSeparator(true);
  dataChanged = false;
}

KateConfigDialog::~KateConfigDialog()
{}

void KateConfigDialog::addPluginPage (Kate::Plugin *plugin, KPageWidgetItem *parentItem)
{
  if (!Kate::pluginConfigPageInterface(plugin))
    return;

  for (uint i = 0; i < Kate::pluginConfigPageInterface(plugin)->configPages(); i++)
  {
    KVBox *page = new KVBox();

    KPageWidgetItem *item = addSubPage( parentItem, page, Kate::pluginConfigPageInterface(plugin)->configPageName(i) );
    item->setHeader( Kate::pluginConfigPageInterface(plugin)->configPageFullName(i) );
    item->setIcon( Kate::pluginConfigPageInterface(plugin)->configPageIcon(i));

    PluginPageListItem *info = new PluginPageListItem;
    info->plugin = plugin;
    info->page = Kate::pluginConfigPageInterface(plugin)->configPage (i, page);
    connect( info->page, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
    pluginPages.append(info);
  }
}

void KateConfigDialog::removePluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigPageInterface(plugin))
    return;

  for (int i = 0; i < pluginPages.count(); i++)
  {
    if  ( pluginPages[i]->plugin == plugin )
    {
      QWidget *w = pluginPages[i]->page->parentWidget();
      delete pluginPages[i]->page;
      delete w;
      pluginPages.removeAt(i);
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
  if( dataChanged )
  {
    KConfigGroup cg = KConfigGroup( config, "General" );

    cg.writeEntry("Restore Window Configuration", cb_restoreVC->isChecked());

    int bu = sessions_start->id (sessions_start->selected());

    if (bu == 0)
      cg.writeEntry ("Startup Session", "new");
    else if (bu == 1)
      cg.writeEntry ("Startup Session", "last");
    else
      cg.writeEntry ("Startup Session", "manual");

    bu = sessions_exit->id (sessions_exit->selected());

    if (bu == 0)
      cg.writeEntry ("Session Exit", "discard");
    else if (bu == 1)
      cg.writeEntry ("Session Exit", "save");
    else
      cg.writeEntry ("Session Exit", "ask");


    m_editorChooser->writeAppSetting();

    cg.writeEntry("Save Meta Infos", cb_saveMetaInfos->isChecked());
    KateDocManager::self()->setSaveMetaInfos(cb_saveMetaInfos->isChecked());

    cg.writeEntry("Days Meta Infos", sb_daysMetaInfos->value() );
    KateDocManager::self()->setDaysMetaInfos(sb_daysMetaInfos->value());

    cg.writeEntry("Modified Notification", cb_modNotifications->isChecked());
    mainWindow->modNotification = cb_modNotifications->isChecked();

#ifdef __GNUC__
#warning portme
#endif
#if 0
    filelistConfigPage->apply();
#endif
    /*
        KateExternalToolsCommand::self()->reload();
        for (int i=0; i < KateApp::self()->mainWindows(); i++)
        {
          KateMainWindow *win = KateApp::self()->mainWindow (i);
          win->externalTools->reload();
        }*/
    //mainWindow->externalTools->reload();

    mainWindow->saveOptions ();

    // save plugin config !!
    KateApp::self()->pluginManager()->writeConfig (KGlobal::config().data());
  }

  //
  // editor config ! (the apply() methode will check the changed state internally)
  //
  for (int i = 0; i < editorPages.count(); i++)
  {
    editorPages.at(i)->apply();
  }

  // write back the editor config
  KateDocManager::self()->editor()->writeConfig(config.data());

  //
  // plugins config ! (the apply() methode SHOULD check the changed state internally)
  //
  for (int i = 0; i < pluginPages.count(); i++)
  {
    pluginPages[i]->page->apply();
  }

  config->sync();

  dataChanged = false;
  enableButton( Apply, false );
}

void KateConfigDialog::slotChanged()
{
  dataChanged = true;
  enableButton( Apply, true );
}
// kate: space-indent on; indent-width 2; replace-tabs on;

