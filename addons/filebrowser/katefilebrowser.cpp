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

#include "katebookmarkhandler.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <KActionCollection>
#include <KActionMenu>
#include <KConfigGroup>
#include <KDirOperator>
#include <KFilePlacesModel>
#include <KHistoryComboBox>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KToolBar>
#include <KUrlNavigator>

#include <QAbstractItemView>
#include <QAction>
#include <QDir>
#include <QLineEdit>
#include <QVBoxLayout>

//END Includes

KateFileBrowser::KateFileBrowser(KTextEditor::MainWindow *mainWindow,
                                 QWidget * parent)
  : QWidget (parent)
  , m_mainWindow(mainWindow)
{

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);

  m_toolbar = new KToolBar(this);
  m_toolbar->setMovable(false);
  m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  m_toolbar->setContextMenuPolicy(Qt::NoContextMenu);
  mainLayout->addWidget(m_toolbar);

  // includes some actions, but not hooked into the shortcut dialog atm
  m_actionCollection = new KActionCollection(this);
  m_actionCollection->addAssociatedWidget(this);

  KFilePlacesModel* model = new KFilePlacesModel(this);
  m_urlNavigator = new KUrlNavigator(model, QUrl::fromLocalFile(QDir::homePath()), this);
  connect(m_urlNavigator, &KUrlNavigator::urlChanged, this, &KateFileBrowser::updateDirOperator);
  mainLayout->addWidget(m_urlNavigator);

  m_dirOperator = new KDirOperator(QUrl(), this);
  // Default to a view with only one column since columns are auto-sized
  m_dirOperator->setView(KFile::Tree);
  m_dirOperator->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_dirOperator->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  mainLayout->addWidget(m_dirOperator);

  // Mime filter for the KDirOperator
  QStringList filter;
  filter << QStringLiteral("text/plain") << QStringLiteral("text/html") << QStringLiteral("inode/directory");
  m_dirOperator->setNewFileMenuSupportedMimeTypes(filter);

  setFocusProxy(m_dirOperator);
  connect(m_dirOperator, &KDirOperator::viewChanged, this, &KateFileBrowser::selectorViewChanged);
  connect(m_urlNavigator, &KUrlNavigator::returnPressed,
          m_dirOperator, static_cast<void (KDirOperator::*)()>(&KDirOperator::setFocus));

  // now all actions exist in dir operator and we can use them in the toolbar
  setupActions();
  setupToolbar();

  m_filter = new KHistoryComboBox(true, this);
  m_filter->setMaxCount(10);
  m_filter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  m_filter->lineEdit()->setPlaceholderText(i18n("Search"));
  mainLayout->addWidget(m_filter);

  connect(m_filter, &KHistoryComboBox::editTextChanged, this, &KateFileBrowser::slotFilterChange);
  connect(m_filter, static_cast<void (KHistoryComboBox::*)(const QString &)>(&KHistoryComboBox::returnPressed),
          m_filter, &KHistoryComboBox::addToHistory);
  connect(m_filter, static_cast<void (KHistoryComboBox::*)(const QString &)>(&KHistoryComboBox::returnPressed),
          m_dirOperator, static_cast< void (KDirOperator::*)()>(&KDirOperator::setFocus));
  connect(m_dirOperator, &KDirOperator::urlEntered, this, &KateFileBrowser::updateUrlNavigator);

  // Connect the bookmark handler
  connect(m_bookmarkHandler, &KateBookmarkHandler::openUrl,
          this, static_cast<void (KateFileBrowser::*)(const QString&)>(&KateFileBrowser::setDir));

  m_filter->setWhatsThis(i18n("Enter a name filter to limit which files are displayed."));

  connect(m_dirOperator, &KDirOperator::fileSelected, this, &KateFileBrowser::fileSelected);
  connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KateFileBrowser::autoSyncFolder);
}

KateFileBrowser::~KateFileBrowser()
{
}
//END Constroctor/Destrctor

//BEGIN Public Methods
void KateFileBrowser::setupToolbar()
{
  KConfigGroup config(KSharedConfig::openConfig(), "filebrowser");
  QStringList actions = config.readEntry( "toolbar actions", QStringList() );
  if ( actions.isEmpty() ) // default toolbar
    actions << QStringLiteral("back") << QStringLiteral("forward") << QStringLiteral("bookmarks") << QStringLiteral("sync_dir") << QStringLiteral("configure");

  // remove all actions from the toolbar (there should be none)
  m_toolbar->clear();

  // now add all actions to the toolbar
  foreach (const QString& it, actions)
  {
    QAction *ac = nullptr;
    if (it.isEmpty()) continue;
    if (it == QStringLiteral("bookmarks") || it == QStringLiteral("sync_dir") || it == QStringLiteral("configure"))
      ac = actionCollection()->action(it);
    else
      ac = m_dirOperator->actionCollection()->action(it);

    if (ac)
      m_toolbar->addAction(ac);
  }
}

void KateFileBrowser::readSessionConfig (const KConfigGroup& cg)
{
  m_dirOperator->readConfig(cg);
  m_dirOperator->setView(KFile::Default);

  m_urlNavigator->setLocationUrl(cg.readEntry("location", QUrl::fromLocalFile (QDir::homePath())));
  setDir(cg.readEntry("location", QUrl::fromLocalFile (QDir::homePath())));
  m_autoSyncFolder->setChecked(cg.readEntry("auto sync folder", false));
  m_filter->setHistoryItems(cg.readEntry("filter history", QStringList()), true);
}

void KateFileBrowser::writeSessionConfig (KConfigGroup& cg)
{
  m_dirOperator->writeConfig(cg);

  cg.writeEntry("location", m_urlNavigator->locationUrl().url());
  cg.writeEntry("auto sync folder", m_autoSyncFolder->isChecked());
  cg.writeEntry("filter history", m_filter->historyItems());
}

//END Public Methods

//BEGIN Public Slots

void KateFileBrowser::slotFilterChange(const QString & nf)
{
  QString f = nf.trimmed();
  const bool empty = f.isEmpty() || f == QStringLiteral("*");

  if (empty) {
    m_dirOperator->clearFilter();
  } else {
    m_dirOperator->setNameFilter(f);
  }

  m_dirOperator->updateDir();
}

bool kateFileSelectorIsReadable (const QUrl& url)
{
  if (!url.isLocalFile())
    return true; // what else can we say?

  QDir dir(url.toLocalFile());
  return dir.exists ();
}

void KateFileBrowser::setDir(QUrl u)
{
  QUrl newurl;

  if (!u.isValid())
    newurl = QUrl::fromLocalFile(QDir::homePath());
  else
    newurl = u;

  QString path(newurl.path());
  if (!path.endsWith(QLatin1Char('/')))
    path += QLatin1Char('/');
  newurl.setPath(path);

  if (!kateFileSelectorIsReadable(newurl)) {
    newurl.setPath(newurl.path() + QStringLiteral("../"));
    newurl = newurl.adjusted(QUrl::NormalizePathSegments);
  }

  if (!kateFileSelectorIsReadable(newurl)) {
    newurl = QUrl::fromLocalFile(QDir::homePath());
  }

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

  if (list.count()>20) {
    if (KMessageBox::questionYesNo(this,i18np("You are trying to open 1 file, are you sure?", "You are trying to open %1 files, are you sure?", list.count()))
      == KMessageBox::No) return;
  }
  
  foreach (const KFileItem& item, list)
  {
    m_mainWindow->openUrl(item.url());
  }

  m_dirOperator->view()->selectionModel()->clear();
}




void KateFileBrowser::updateDirOperator(const QUrl &u)
{
  m_dirOperator->setUrl(u, true);
}

void KateFileBrowser::updateUrlNavigator(const QUrl &u)
{
  m_urlNavigator->setLocationUrl(u);
}

void KateFileBrowser::setActiveDocumentDir()
{
  QUrl u = activeDocumentUrl();
  if (!u.isEmpty())
    setDir(KIO::upUrl(u));
}

void KateFileBrowser::autoSyncFolder()
{
  if (m_autoSyncFolder->isChecked()) {
    setActiveDocumentDir();
  }
}


void KateFileBrowser::selectorViewChanged(QAbstractItemView * newView)
{
  newView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

//END Private Slots

//BEGIN Protected

QUrl KateFileBrowser::activeDocumentUrl()
{
  KTextEditor::View *v = m_mainWindow->activeView();
  if (v)
    return v->document()->url();
  return QUrl();
}

void KateFileBrowser::setupActions()
{
  // bookmarks action!
  KActionMenu *acmBookmarks = new KActionMenu(QIcon::fromTheme(QStringLiteral("bookmarks")), i18n("Bookmarks"), this);
  acmBookmarks->setDelayed(false);
  m_bookmarkHandler = new KateBookmarkHandler(this, acmBookmarks->menu());
  acmBookmarks->setShortcutContext(Qt::WidgetWithChildrenShortcut);

  // action for synchronizing the dir operator with the current document path
  QAction* syncFolder = new QAction(this);
  syncFolder->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  syncFolder->setText(i18n("Current Document Folder"));
  syncFolder->setIcon(QIcon::fromTheme(QStringLiteral("system-switch-user")));
  connect(syncFolder, &QAction::triggered, this, &KateFileBrowser::setActiveDocumentDir);

  m_actionCollection->addAction(QStringLiteral("sync_dir"), syncFolder);
  m_actionCollection->addAction(QStringLiteral("bookmarks"), acmBookmarks);

  // section for settings menu
  KActionMenu *optionsMenu = new KActionMenu(QIcon::fromTheme(QStringLiteral("configure")), i18n("Options"), this);
  optionsMenu->setDelayed(false);
  optionsMenu->addAction(m_dirOperator->actionCollection()->action(QStringLiteral("short view")));
  optionsMenu->addAction(m_dirOperator->actionCollection()->action(QStringLiteral("detailed view")));
  optionsMenu->addAction(m_dirOperator->actionCollection()->action(QStringLiteral("tree view")));
  optionsMenu->addAction(m_dirOperator->actionCollection()->action(QStringLiteral("detailed tree view")));
  optionsMenu->addSeparator();
  optionsMenu->addAction(m_dirOperator->actionCollection()->action(QStringLiteral("show hidden")));

  // action for synchronising the dir operator with the current document path
  m_autoSyncFolder = new QAction(this);
  m_autoSyncFolder->setCheckable(true);
  m_autoSyncFolder->setText(i18n("Automatically synchronize with current document"));
  m_autoSyncFolder->setIcon(QIcon::fromTheme(QStringLiteral("system-switch-user")));
  connect(m_autoSyncFolder, &QAction::triggered, this, &KateFileBrowser::autoSyncFolder);
  optionsMenu->addAction(m_autoSyncFolder);

  m_actionCollection->addAction(QStringLiteral("configure"), optionsMenu);
  
  //
  // Remove all shortcuts due to shortcut clashes (e.g. F5: reload, Ctrl+B: bookmark)
  // BUGS: #188954, #236368
  //
  foreach (QAction* a, m_actionCollection->actions()) {
    a->setShortcut(QKeySequence());
  }
  foreach (QAction* a, m_dirOperator->actionCollection()->actions()) {
    a->setShortcut(QKeySequence());
  }
}
//END Protected

// kate: space-indent on; indent-width 2; replace-tabs on;
