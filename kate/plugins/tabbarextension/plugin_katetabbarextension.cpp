/***************************************************************************
                           plugin_katetabbarextension.cpp
                           -------------------
    begin                : 2004-04-20
    copyright            : (C) 2004-2005 by Dominik Haumann
    email                : dhdev@gmx.de
 ***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/


//BEGIN INCLUDES
#include "plugin_katetabbarextension.h"
#include "ktinytabbar.h"

#include <kate/documentmanager.h>
#include <kate/application.h>

#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <kglobal.h>
#include <kgenericfactory.h>

#include <kdebug.h>
#include <ktoolbar.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kfiledialog.h>

#include <QPushButton>
#include <QLayout>

#include <q3groupbox.h>
#include <QCheckBox>
#include <q3vbox.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QBoxLayout>
//END


K_EXPORT_COMPONENT_FACTORY( katetabbarextensionplugin,
                            KGenericFactory<KatePluginTabBarExtension>( "katetabbarextension" ) )



//BEGIN PluginView
PluginView::PluginView( Kate::MainWindow* mainwindow )
  : Kate::PluginView( mainwindow )
{
  tabbar = new KTinyTabBar( mainWindow()->centralWidget() );

  QBoxLayout* layout = qobject_cast<QBoxLayout*>(mainWindow()->centralWidget()->layout());
  layout->insertWidget( 0, tabbar );

  connect( Kate::application()->documentManager(), SIGNAL( documentCreated( KTextEditor::Document* ) ),
           this, SLOT( slotDocumentCreated( KTextEditor::Document* ) ) );
  connect( Kate::application()->documentManager(), SIGNAL( documentDeleted( KTextEditor::Document* ) ),
           this, SLOT( slotDocumentDeleted( KTextEditor::Document* ) ) );
  connect( mainWindow(), SIGNAL( viewChanged() ),
           this, SLOT( slotViewChanged() ) );

  connect( tabbar, SIGNAL( currentChanged( int ) ),
           this, SLOT( currentTabChanged( int ) ) );
  connect( tabbar, SIGNAL( closeRequest( int ) ),
           this, SLOT( closeTabRequest( int ) ) );
  // add already existing documents
  foreach( KTextEditor::Document* doc, Kate::application()->documentManager()->documents() )
    slotDocumentCreated( doc );
}

PluginView::~PluginView()
{
  delete tabbar;
}

void PluginView::readSessionConfig (KConfig* config, const QString& groupPrefix)
{
  tabbar->load( config, groupPrefix + ":view" );
  updateLocation();
}

void PluginView::writeSessionConfig (KConfig* config, const QString& groupPrefix)
{
  tabbar->save( config, groupPrefix + ":view" );
}

void PluginView::updateLocation()
{
  QBoxLayout* layout = qobject_cast<QBoxLayout*>(mainWindow()->centralWidget()->layout());
  if( !layout ) return;

  layout->removeWidget( tabbar );
  layout->insertWidget( tabbar->locationTop()?0:-1, tabbar );
}


void PluginView::currentTabChanged( int button_id )
{
  mainWindow()->activateView( id2doc[button_id] );
}

void PluginView::closeTabRequest( int button_id )
{
  Kate::application()->documentManager()->closeDocument( id2doc[button_id] );
}

void PluginView::slotDocumentCreated( KTextEditor::Document* document )
{
  if( !document )
    return;

  connect( document, SIGNAL( modifiedChanged( KTextEditor::Document*) ),
           this, SLOT( slotDocumentChanged( KTextEditor::Document* ) ) );
  connect( document, SIGNAL( modifiedOnDisk( KTextEditor::Document*, bool,
                             KTextEditor::ModificationInterface::ModifiedOnDiskReason ) ),
           this, SLOT( slotModifiedOnDisc( KTextEditor::Document*, bool,
                       KTextEditor::ModificationInterface::ModifiedOnDiskReason ) ) );
  connect( document, SIGNAL( documentNameChanged( KTextEditor::Document* ) ),
           this, SLOT( slotNameChanged( KTextEditor::Document* ) ) );


  int tabID = tabbar->addTab( document->url().prettyUrl(), document->documentName() );
  id2doc[tabID] = document;
  doc2id[document] = tabID;
}

void PluginView::slotDocumentDeleted( KTextEditor::Document* document )
{
  //  kDebug() << "slotDocumentDeleted " << endl;
  int tabID = doc2id[document];

  tabbar->removeTab( tabID );
  doc2id.remove( document );
  id2doc.remove( tabID );
}

void PluginView::slotViewChanged()
{
  KTextEditor::View *view = mainWindow()->activeView();
  if( !view )
    return;

  int tabID = doc2id[view->document()];
  tabbar->setCurrentTab( tabID );
}

void PluginView::slotDocumentChanged( KTextEditor::Document* document )
{
  if( !document )
    return;

  int tabID = doc2id[document];
  if( document->isModified() )
    tabbar->setTabIcon( tabID, KIconLoader::global()
            ->loadIcon( "modified", K3Icon::Small, 16 ) );
  else
    tabbar->setTabIcon( tabID, QIcon() );

  tabbar->setTabModified( tabID, document->isModified() );
}

void PluginView::slotNameChanged( KTextEditor::Document* document )
{
  if( !document )
    return;

  int tabID = doc2id[document];
  tabbar->setTabText( tabID, document->documentName() );
  if( document->url().prettyUrl() != tabbar->tabURL( tabID ) )
    tabbar->setTabURL( tabID, document->url().prettyUrl() );
}

void PluginView::slotModifiedOnDisc( KTextEditor::Document* document, bool modified,
                                     KTextEditor::ModificationInterface::ModifiedOnDiskReason reason )
{
  kDebug() << "modified: " << modified << ", id: " << reason << endl;
  int tabID = doc2id[document];
  if( !modified )
  {
    tabbar->setTabIcon( tabID, QIcon() );
    tabbar->setTabModified( tabID, false );
  }
  else
  {
    // NOTE: right now the size 16 pixel is hard coded. If I omit the size
    // parameter it would use KDE defaults. So if a user requests standard
    // sized icons (because he changed his KDE defaults) the solution is to
    // omit the parameter.
    switch( reason )
    {
    case KTextEditor::ModificationInterface::OnDiskModified:
      tabbar->setTabIcon( tabID, KIconLoader::global()
            ->loadIcon( "cancel", K3Icon::Small, 16 ) );
      break;
    case KTextEditor::ModificationInterface::OnDiskCreated:
      tabbar->setTabIcon( tabID, KIconLoader::global()
            ->loadIcon( "modified", K3Icon::Small, 16 ) );
    break;
    case KTextEditor::ModificationInterface::OnDiskDeleted:
      tabbar->setTabIcon( tabID, KIconLoader::global()
          ->loadIcon( "cancel", K3Icon::Small, 16 ) );
      break;
    default:
      tabbar->setTabIcon( tabID, KIconLoader::global()
            ->loadIcon( "cancel", K3Icon::Small, 16 ) );
    }

    tabbar->setTabModified( tabID, true );
  }
}
//END PluginView



//BEGIN KatePluginTabBarExtension
KatePluginTabBarExtension::KatePluginTabBarExtension(
    QObject* parent, const QStringList& )
  : Kate::Plugin ( (Kate::Application*)parent),
    pConfig(new KConfig("katetabbarextensionpluginrc"))
{
  // ### seems pConfig is never used!
//   pConfig->setGroup("global");
}

KatePluginTabBarExtension::~KatePluginTabBarExtension()
{
//  pConfig->sync();
//  delete pConfig;
}

Kate::PluginView *KatePluginTabBarExtension::createView (Kate::MainWindow *mainWindow)
{
  PluginView *view = new PluginView( mainWindow );
  connect( view->tabbar, SIGNAL( settingsChanged( KTinyTabBar* ) ),
           this, SLOT( tabbarSettingsChanged( KTinyTabBar* ) ) );
  connect( view->tabbar, SIGNAL( highlightMarksChanged( KTinyTabBar* ) ),
           this, SLOT( tabbarHighlightMarksChanged( KTinyTabBar* ) ) );
  m_views.append( view );
  return view;
}


void KatePluginTabBarExtension::readSessionConfig (KConfig* config, const QString& groupPrefix)
{
}

void KatePluginTabBarExtension::writeSessionConfig (KConfig* config, const QString& groupPrefix)
{
}

void KatePluginTabBarExtension::tabbarHighlightMarksChanged( KTinyTabBar* tabbar )
{
  // synchronize all tabbars
  foreach( PluginView* view, m_views )
  {
    view->updateLocation();
    if( view->tabbar != tabbar )
    {
      view->tabbar->setHighlightMarks( tabbar->highlightMarks() );
    }
  }
}

void KatePluginTabBarExtension::tabbarSettingsChanged( KTinyTabBar* tabbar )
{
  // synchronize all tabbars
  foreach( PluginView* view, m_views )
  {
    view->updateLocation();
    if( view->tabbar != tabbar )
    {
      view->tabbar->setLocationTop( tabbar->locationTop() );
      view->updateLocation();
      view->tabbar->setNumRows( tabbar->numRows() );
      view->tabbar->setMinimumTabWidth( tabbar->minimumTabWidth() );
      view->tabbar->setMaximumTabWidth( tabbar->maximumTabWidth() );
      view->tabbar->setTabHeight( tabbar->tabHeight() );
      view->tabbar->setTabButtonStyle( tabbar->tabButtonStyle() );
      view->tabbar->setFollowCurrentTab( tabbar->followCurrentTab() );
      view->tabbar->setTabSortType( tabbar->tabSortType() );
      view->tabbar->setHighlightModifiedTabs( tabbar->highlightModifiedTabs() );
      view->tabbar->setHighlightActiveTab( tabbar->highlightActiveTab() );
      view->tabbar->setHighlightPreviousTab( tabbar->highlightPreviousTab() );
      view->tabbar->setHighlightOpacity( tabbar->highlightOpacity() );
      view->tabbar->setPlainColorPressed( tabbar->plainColorPressed() );
      view->tabbar->setPlainColorHovered( tabbar->plainColorHovered() );
      view->tabbar->setPlainColorActivated( tabbar->plainColorActivated() );
      view->tabbar->setModifiedTabsColor( tabbar->modifiedTabsColor() );
      view->tabbar->setActiveTabColor( tabbar->activeTabColor() );
      view->tabbar->setPreviousTabColor( tabbar->previousTabColor() );
    }
  }
}

Kate::PluginConfigPage* KatePluginTabBarExtension::configPage(
    uint, QWidget *w, const char* /*name*/)
{
  KateTabBarExtensionConfigPage* p = new KateTabBarExtensionConfigPage(this, w);
  initConfigPage( p );
  connect( p, SIGNAL(configPageApplyRequest(KateTabBarExtensionConfigPage*)),
      SLOT(applyConfig(KateTabBarExtensionConfigPage*)) );
  return (Kate::PluginConfigPage*)p;
}

void KatePluginTabBarExtension::initConfigPage( KateTabBarExtensionConfigPage* p )
{
  p->pSortAlpha->setChecked( true );
}

void KatePluginTabBarExtension::applyConfig( KateTabBarExtensionConfigPage* p )
{
//  m_tabbar->setSortByName(p->pSortAlpha->isChecked());
  // sync m_config in destructor
}
//END KatePluginTabBarExtension


//BEGIN KateTabBarExtensionConfigPage
KateTabBarExtensionConfigPage::KateTabBarExtensionConfigPage(
    QObject* /*parent*/ /*= 0L*/, QWidget *parentWidget /*= 0L*/)
  : Kate::PluginConfigPage( parentWidget )
{
  QVBoxLayout* top = new QVBoxLayout(this, 0,
      KDialog::spacingHint());

  Q3GroupBox* gb = new Q3GroupBox( i18n("Sorting Behavior"),
      this, "tab_bar_extension_config_page_layout" );
  gb->setColumnLayout(1, Qt::Vertical);
  gb->setInsideSpacing(KDialog::spacingHint());
  pSortAlpha = new QCheckBox(i18n("Sort files alphabetically"), gb);

  top->add(gb);
  top->addStretch(1);
//  throw signal changed
  connect(pSortAlpha, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
}

KateTabBarExtensionConfigPage::~KateTabBarExtensionConfigPage() {}

void KateTabBarExtensionConfigPage::apply()
{
    emit configPageApplyRequest( this );
}
//END KateTabBarExtensionConfigPage

#include "plugin_katetabbarextension.moc"

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs off; eol unix;
