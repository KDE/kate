/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

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

//BEGIN Includes
#include "katefilebrowser.h"
#include "katefilebrowser.moc"

#include "katebookmarkhandler.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KDebug>
#include <KDirOperator>
#include <KFilePlacesModel>
#include <KHistoryComboBox>
#include <KLocale>
#include <KToolBar>
#include <KUrlNavigator>
#include <QAbstractItemView>
#include <QDir>
#include <QLineEdit>
#include <QToolButton>
//END Includes


KateFileBrowser::KateFileBrowser(Kate::MainWindow *mainWindow,
                                 QWidget * parent, const char * name)
  : KVBox (parent)
  , m_mainWindow(mainWindow)
{
  setObjectName(name);

  m_toolbar = new KToolBar(this);
  m_toolbar->setMovable(false);
  m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  m_toolbar->setContextMenuPolicy(Qt::NoContextMenu);

  // includes some actions, but not hooked into the shortcut dialog atm
  m_actionCollection = new KActionCollection(this);
  m_actionCollection->addAssociatedWidget(this);

  KFilePlacesModel* model = new KFilePlacesModel( this );
  m_urlNavigator = new KUrlNavigator(model, KUrl( QDir::homePath() ), this );
  connect(m_urlNavigator, SIGNAL(urlChanged(const KUrl& )), SLOT(updateDirOperator(const KUrl&)));

  m_dirOperator = new KDirOperator(KUrl(), this);
  m_dirOperator->setView(KFile::/* Simple */Detail);
  m_dirOperator->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(m_dirOperator, SIGNAL(viewChanged(QAbstractItemView *)),
          this, SLOT(selectorViewChanged(QAbstractItemView *)));
  setStretchFactor(m_dirOperator, 2);
  m_dirOperator->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  // now all actions exist in dir operator and we can use them in the toolbar
  setupToolbar();

  KHBox* filterBox = new KHBox(this);

  btnFilter = new QToolButton( filterBox );
  btnFilter->setIcon( KIcon("view-filter" ) );
  btnFilter->setCheckable( true );
  filter = new KHistoryComboBox( true, filterBox);
  filter->setMaxCount(10);
  filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  filterBox->setStretchFactor(filter, 2);
  connect( btnFilter, SIGNAL( clicked() ), this, SLOT( filterButtonClicked() ) );

  connect( filter, SIGNAL( activated(const QString&) ),
           SLOT( slotFilterChange(const QString&) ) );
  connect( filter, SIGNAL( returnPressed(const QString&) ),
           filter, SLOT( addToHistory(const QString&) ) );

  connect(m_dirOperator, SIGNAL(urlEntered(const KUrl&)),
           this, SLOT(updateUrlNavigator(const KUrl&)) );

  // Connect the bookmark handler
  connect( m_bookmarkHandler, SIGNAL( openUrl( const QString& )),
           this, SLOT( setDir( const QString& ) ) );

  filter->setWhatsThis(        i18n("<p>Here you can enter a name filter to limit which files are displayed.</p>"
                                    "<p>To clear the filter, toggle off the filter button to the left.</p>"
                                    "<p>To reapply the last filter used, toggle on the filter button.</p>" ) );
  btnFilter->setWhatsThis(        i18n("<p>This button clears the name filter when toggled off, or "
                                       "reapplies the last filter used when toggled on.</p>") );

  connect(m_dirOperator, SIGNAL(fileSelected(const KFileItem&)), this, SLOT(fileSelected(const KFileItem&)));
}

KateFileBrowser::~KateFileBrowser()
{
}
//END Constroctor/Destrctor

//BEGIN Public Methods
void KateFileBrowser::readSessionConfig(KConfigBase *config, const QString & name)
{

  KConfigGroup cgView(config, name + ":view");
  m_dirOperator->setViewConfig(cgView );

  KConfigGroup cgDir(config, name + ":dir");
  m_dirOperator->readConfig(cgDir);
  m_dirOperator->setView(KFile::Default);

  KConfigGroup cg (config, name );
  m_urlNavigator->setUrl( cg.readPathEntry( "location", QDir::homePath() ) );

  KConfigGroup globalConfig( KGlobal::config(), "fileselector" );

  setDir(cg.readPathEntry( "location", QDir::homePath()));

  m_dirOperator->setShowHiddenFiles( cg.readEntry( "show hidden files", false ) );

  filter->setHistoryItems( cg.readEntry("filter history", QStringList()), true );
  lastFilter = cg.readEntry( "last filter" );
  QString flt("");
  if (globalConfig.readEntry( "restore last filter", true ))
    flt = cg.readEntry("current filter");
  filter->lineEdit()->setText( flt );
  slotFilterChange( flt );
}

void KateFileBrowser::setupToolbar()
{
  // remove all actions from the toolbar (there should be none)
  m_toolbar->clear();

  // create action list
  QList<QAction*> actions;
  actions << m_dirOperator->actionCollection()->action("back");
  actions << m_dirOperator->actionCollection()->action("forward");

  // bookmarks action!
  KActionMenu *acmBookmarks = new KActionMenu( KIcon("bookmarks"), i18n("Bookmarks"), this );
  acmBookmarks->setDelayed( false );
  m_bookmarkHandler = new KateBookmarkHandler( this, acmBookmarks->menu() );
  acmBookmarks->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  actions << acmBookmarks;

  // action for synchronising the dir operator with the current document path
  KAction* syncFolder = new KAction(this);
  syncFolder->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  syncFolder->setText(i18n("Current Document Folder"));
  syncFolder->setIcon(KIcon("system-switch-user"));
  connect(syncFolder, SIGNAL(triggered()), this, SLOT(setActiveDocumentDir()));

  actions << syncFolder;

  // now add all actions to the toolbar
  foreach (QAction* ac, actions) {
    if (ac) {
      m_toolbar->addAction(ac);
    }
  }

  m_actionCollection->addAction("sync_dir", syncFolder);
  m_actionCollection->addAction("bookmarks", acmBookmarks);

  // section for settings menu
  KActionMenu *optionsMenu = new KActionMenu(KIcon("configure"), i18n("Options"), this);
  optionsMenu->setDelayed(false);
  optionsMenu->addAction(m_dirOperator->actionCollection()->action("short view"));
  optionsMenu->addAction(m_dirOperator->actionCollection()->action("detailed view"));
  optionsMenu->addAction(m_dirOperator->actionCollection()->action("tree view"));
  optionsMenu->addAction(m_dirOperator->actionCollection()->action("detailed tree view"));
  optionsMenu->addSeparator();
  optionsMenu->addAction(m_dirOperator->actionCollection()->action("show hidden"));

  m_toolbar->addSeparator();
  m_toolbar->addAction(optionsMenu);
}

void KateFileBrowser::writeSessionConfig(KConfigBase *config, const QString & name)
{
  KConfigGroup cgDir(config, name + ":dir");
  m_dirOperator->writeConfig(cgDir);

  KConfigGroup cg = KConfigGroup( config, name );
  cg.writePathEntry( "location", m_urlNavigator->url().url() );
  cg.writeEntry( "show hidden files", m_dirOperator->showHiddenFiles() );
}

//END Public Methods

//BEGIN Public Slots

void KateFileBrowser::slotFilterChange( const QString & nf )
{
  QString f = nf.trimmed();
  const bool empty = f.isEmpty() || f == "*";

  if ( empty )
  {
    m_dirOperator->clearFilter();
    filter->lineEdit()->setText( QString() );
    btnFilter->setToolTip( i18n("Apply last filter (\"%1\")", lastFilter) ) ;
  }
  else
  {
    m_dirOperator->setNameFilter( f );
    lastFilter = f;
    btnFilter->setToolTip( i18n("Clear filter") );
  }

  btnFilter->setChecked( !empty );
  m_dirOperator->updateDir();

  // this will be never true after the filter has been used;)
  btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );
}

bool kateFileSelectorIsReadable ( const KUrl& url )
{
  if ( !url.isLocalFile() )
    return true; // what else can we say?

  QDir dir(url.toLocalFile());
  return dir.exists ();
}

void KateFileBrowser::setDir( KUrl u )
{
  KUrl newurl;

  if ( !u.isValid() )
    newurl.setPath( QDir::homePath() );
  else
    newurl = u;

  QString pathstr = newurl.path( KUrl::AddTrailingSlash );
  newurl.setPath(pathstr);

  if ( !kateFileSelectorIsReadable ( newurl ) )
    newurl.cd(QString::fromLatin1(".."));

  if ( !kateFileSelectorIsReadable (newurl) )
    newurl.setPath( QDir::homePath() );

  m_dirOperator->setUrl(newurl, true);
}

//END Public Slots

//BEGIN Private Slots

void KateFileBrowser::fileSelected(const KFileItem & /*file*/)
{
  openSelectedFiles();
}

void KateFileBrowser::openSelectedFiles()
{
  const KFileItemList list = m_dirOperator->selectedItems();

  foreach (const KFileItem& item, list)
  {
    m_mainWindow->openUrl(item.url());
  }

  m_dirOperator->view()->selectionModel()->clear();
}




void KateFileBrowser::updateDirOperator( const KUrl& u )
{
  m_dirOperator->setUrl(u, true);
}

void KateFileBrowser::updateUrlNavigator( const KUrl& u )
{
  m_urlNavigator->setUrl( u );
}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void KateFileBrowser::filterButtonClicked()
{
  if ( !btnFilter->isChecked() )
  {
    slotFilterChange( QString() );
  }
  else
  {
    filter->lineEdit()->setText( lastFilter );
    slotFilterChange( lastFilter );
  }
}

void KateFileBrowser::setActiveDocumentDir()
{
//   kDebug(13001)<<"KateFileBrowser::setActiveDocumentDir()";
  KUrl u = activeDocumentUrl();
//   kDebug(13001)<<"URL: "<<u.prettyUrl();
  if (!u.isEmpty())
    setDir( u.upUrl() );
//   kDebug(13001)<<"... setActiveDocumentDir() DONE!";
}

void KateFileBrowser::selectorViewChanged( QAbstractItemView * newView )
{
  newView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

//END Private Slots

//BEGIN Protected

KUrl KateFileBrowser::activeDocumentUrl()
{
  KTextEditor::View *v = m_mainWindow->activeView();
  if ( v )
    return v->document()->url();
  return KUrl();
}

void KateFileBrowser::focusInEvent( QFocusEvent * )
{
  m_dirOperator->setFocus();
}
//END Protected


// kate: space-indent on; indent-width 2; replace-tabs on;
