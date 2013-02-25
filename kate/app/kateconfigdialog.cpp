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
#include "katesession.h"

#include <KTextEditor/ConfigPage>
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
  setHelp( QString(), KGlobal::mainComponent().componentName() );

  KSharedConfig::Ptr config = KGlobal::config();
  KConfigGroup cgGeneral = KConfigGroup( config, "General" );

  enableButton( Apply, false );

  KPageWidgetItem *applicationItem = addPage( new QWidget, i18n("Application") );
  applicationItem->setIcon( KIcon( "preferences-other" ) );
  applicationItem->setHeader( i18n("Application Options") );
  m_applicationPage = applicationItem;

  //BEGIN General page
  QFrame* generalFrame = new QFrame;
  KPageWidgetItem *item = addSubPage( applicationItem, generalFrame, i18n("General") );
  item->setHeader( i18n("General Options") );
  item->setIcon( KIcon( "go-home" ) );

  QVBoxLayout *layout = new QVBoxLayout( generalFrame );
  layout->setMargin(0);

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
                                       "when that file is tried to be saved.") );
  connect( m_modNotifications, SIGNAL(toggled(bool)),
           this, SLOT(slotChanged()) );

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
  connect( m_saveMetaInfos, SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );

  vbox->addWidget(m_saveMetaInfos);

  // meta infos days
  KHBox *metaInfos = new KHBox( buttonGroup );
  metaInfos->setEnabled(KateDocManager::self()->getSaveMetaInfos());
  QLabel *label = new QLabel( i18n("&Delete unused meta-information after:"), metaInfos );
  m_daysMetaInfos = new QSpinBox( metaInfos );
  m_daysMetaInfos->setMaximum( 180 );
  m_daysMetaInfos->setSpecialValueText(i18n("(never)"));
  m_daysMetaInfos->setValue( KateDocManager::self()->getDaysMetaInfos() );
  label->setBuddy( m_daysMetaInfos );
  connect( m_saveMetaInfos, SIGNAL(toggled(bool)), metaInfos, SLOT(setEnabled(bool)) );
  connect( m_daysMetaInfos, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()) );

  vbox->addWidget(metaInfos);
  buttonGroup->setLayout(vbox);

  layout->addStretch(1); // :-] works correct without autoadd
  //END General page


  //BEGIN Session page
  QFrame* sessionsFrame = new QFrame;
  item = addSubPage( applicationItem, sessionsFrame, i18n("Sessions") );
  item->setHeader( i18n("Session Management") );
  item->setIcon( KIcon( "view-history" ) );

  layout = new QVBoxLayout( sessionsFrame );
  layout->setMargin(0);

  // GROUP with the one below: "Startup"
  buttonGroup = new QGroupBox( i18n("Elements of Sessions"), sessionsFrame );
  vbox = new QVBoxLayout;
  layout->addWidget( buttonGroup );

  // restore view  config
  m_restoreVC = new QCheckBox( buttonGroup );
  m_restoreVC->setText(i18n("Include &window configuration"));
  m_restoreVC->setChecked( cgGeneral.readEntry("Restore Window Configuration", true) );
  m_restoreVC->setWhatsThis( i18n("Check this if you want all your views and frames restored each time you open Kate"));
  connect( m_restoreVC, SIGNAL(toggled(bool)), this, SLOT(slotChanged()) );

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

  layout->addStretch(1); // :-] works correct without autoadd
  //END Session page

  //BEGIN Plugins page
  KVBox *page = new KVBox();
  page->setSpacing( -1 );
  KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
  connect( configPluginPage, SIGNAL(changed()), this, SLOT(slotChanged()) );

  item = addSubPage( applicationItem, page, i18n("Plugins") );
  item->setHeader( i18n("Plugin Manager") );
  item->setIcon( KIcon( "preferences-plugin" ) );

  KatePluginList &pluginList (KatePluginManager::self()->pluginList());
  foreach (const KatePluginInfo &plugin, pluginList)
  {
    if  ( plugin.load
          && Kate::pluginConfigPageInterface(plugin.plugin) )
      addPluginPage (plugin.plugin);
  }
  //END Plugins page

// editor widgets from kwrite/kwdialog
  m_editorPage = addPage( new QWidget, i18n("Editor Component") );
  m_editorPage->setIcon( KIcon( "accessories-text-editor" ) );
  m_editorPage->setHeader( i18n("Editor Component Options") );

  addEditorPages();

  connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
  connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));
  connect(this, SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)),
          this, SLOT(slotCurrentPageChanged(KPageWidgetItem*,KPageWidgetItem*)));
  m_dataChanged = false;

  resize(minimumSizeHint());
  slotChanged();
}

KateConfigDialog::~KateConfigDialog()
{}


void KateConfigDialog::addEditorPages() {

  for (int i = 0; i < KateDocManager::self()->editor()->configPages (); ++i)
  {
    QWidget *page = KateDocManager::self()->editor()->configPage(i, this);
    connect( page, SIGNAL(changed()), this, SLOT(slotChanged()) );
    KPageWidgetItem *item=addSubPage( m_editorPage, page, KateDocManager::self()->editor()->configPageName(i) );
    item->setHeader( KateDocManager::self()->editor()->configPageFullName(i) );
    item->setIcon( KateDocManager::self()->editor()->configPageIcon(i) );
  }
}

void KateConfigDialog::addPluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigPageInterface(plugin))
    return;

  for (uint i = 0; i < Kate::pluginConfigPageInterface(plugin)->configPages(); i++)
  {
    KVBox *page = new KVBox();
    page->setSpacing( -1 );

    KPageWidgetItem *item = addSubPage( m_applicationPage, page, Kate::pluginConfigPageInterface(plugin)->configPageName(i) );
    item->setHeader( Kate::pluginConfigPageInterface(plugin)->configPageFullName(i) );
    item->setIcon( Kate::pluginConfigPageInterface(plugin)->configPageIcon(i));

    PluginPageListItem *info = new PluginPageListItem;
    info->plugin = plugin;
    info->configPageInterface=Kate::pluginConfigPageInterface(plugin);
    info->pageParent=page;
    info->pluginPage = 0; //info->configPageInterface->configPage (i, page);
    info->editorPage = 0;
    info->idInPlugin=i;
    info->pageWidgetItem = item;
    //connect( info->page, SIGNAL(changed()), this, SLOT(slotChanged()) );
    m_pluginPages.insert(item,info);
  }
}

void KateConfigDialog::slotCurrentPageChanged( KPageWidgetItem *current, KPageWidgetItem * /*before*/ )
{
  PluginPageListItem *info=m_pluginPages[current];
  if (!info) return;
  if (info->pluginPage || info->editorPage) return;
  kDebug()<<"creating config page";
  info->pluginPage=info->configPageInterface->configPage(info->idInPlugin,info->pageParent);
  info->pluginPage->show();
  connect( info->pluginPage, SIGNAL(changed()), this, SLOT(slotChanged()) );
}

void KateConfigDialog::removePluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigPageInterface(plugin))
    return;

  QList<KPageWidgetItem*> remove;
  for (QHash<KPageWidgetItem*,PluginPageListItem*>::const_iterator it=m_pluginPages.constBegin();it!=m_pluginPages.constEnd();++it) {
    PluginPageListItem *pli=it.value();
    if (!pli) continue;
    if (pli->plugin==plugin)
      remove.append(it.key());
  }

  kDebug()<<remove.count();
  while (!remove.isEmpty()) {
    KPageWidgetItem *wItem=remove.takeLast();
    PluginPageListItem *pItem=m_pluginPages.take(wItem);
    delete pItem->pluginPage;
    delete pItem->pageParent;
    removePage(wItem);
    delete pItem;
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

    cg.writeEntry("Save Meta Infos", m_saveMetaInfos->isChecked());
    KateDocManager::self()->setSaveMetaInfos(m_saveMetaInfos->isChecked());

    cg.writeEntry("Days Meta Infos", m_daysMetaInfos->value() );
    KateDocManager::self()->setDaysMetaInfos(m_daysMetaInfos->value());

    cg.writeEntry("Modified Notification", m_modNotifications->isChecked());
    m_mainWindow->modNotification = m_modNotifications->isChecked();

    // patch document modified warn state
    const QList<KTextEditor::Document*> &docs = KateDocManager::self()->documentList ();
    foreach (KTextEditor::Document *doc, docs)
      if (qobject_cast<KTextEditor::ModificationInterface *>(doc))
        qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning (!m_modNotifications->isChecked());

    m_mainWindow->saveOptions ();

    // save plugin config !!
    KateSessionManager *sessionmanager = KateSessionManager::self();
    KConfig *sessionConfig = sessionmanager->activeSession()->configWrite();
    KateApp::self()->pluginManager()->writeConfig (sessionConfig);
  }

  foreach (PluginPageListItem *plugin, m_pluginPages.values())
  {
    if (!plugin) continue;
    if (plugin->pluginPage)
      plugin->pluginPage->apply();
    else if (plugin->editorPage)
      plugin->editorPage->apply();
  }

  // write back the editor config
  KateDocManager::self()->editor()->writeConfig(config.data());

  config->sync();

  m_dataChanged = false;
  enableButton( Apply, false );
}

void KateConfigDialog::slotChanged()
{
  m_dataChanged = true;
  enableButton( Apply, true );
  m_daysMetaInfos->setSuffix(i18ncp("The suffix of 'Delete unused meta-information after'", " day", " days", m_daysMetaInfos->value()));
}

void KateConfigDialog::showAppPluginPage(Kate::PluginConfigPageInterface *configpageinterface,uint id)
{
  foreach (PluginPageListItem *plugin, m_pluginPages.values())
  {
    if ((plugin->configPageInterface==configpageinterface) && (id==plugin->idInPlugin))
    {
      setCurrentPage(plugin->pageWidgetItem);
      break;
    }
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;

