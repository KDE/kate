/* This file is part of the KDE libraries
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>

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

#include "katecmdactionmanager.h"
#include "katecmdactionmanager.moc"

#include "ui_commandmenuconfigwidget.h"
#include "ui_commandmenueditwidget.h"

#include "kateglobal.h"
#include "katecmd.h"
#include "kateview.h"

#include <kactionmenu.h>
#include <kactioncollection.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmenu.h>
#include <ktexteditor/commandinterface.h>

#include <QtAlgorithms>
#include <QAccessible>

#include <QTime>

// bool cmpKateCmdActions(const KateCmdAction& a, const KateCmdAction& b)
// {
//   if( (a.category.isEmpty() && b.category.isEmpty())
//     ||(!a.category.isEmpty() && !b.category.isEmpty()) ) {
//     return a.name < b.name;
//   } else if( a.category.isEmpty() || b.category.isEmpty() ) {
//     return a.category.isEmpty();
//   }
// }

// used for qSort
bool operator<(const KateCmdAction& a, const KateCmdAction& b)
{
  return QString::localeAwareCompare(a.name, b.name) == -1;
}

//BEGIN KateCmdActionItem
class KateCmdActionItem : public QTreeWidgetItem
{
  public:
    KateCmdActionItem( QTreeWidget* parent, const QString& text )
      : QTreeWidgetItem( parent, QStringList() << text ), m_action( 0 ) {}
    KateCmdActionItem( QTreeWidgetItem* parent, const QStringList& strings, KateCmdAction* action )
      : QTreeWidgetItem( parent, strings ), m_action( action ) {}
    virtual ~KateCmdActionItem() {}

  public:
    KateCmdAction* m_action;
};
//END KateCmdActionItem

//BEGIN KateCmdActionMenu
KateCmdActionMenu::KateCmdActionMenu( KTextEditor::View* view,
                                      const QString &text,
                                      KActionCollection *ac,
                                      const QString& ac_name )
  : KActionMenu(text, ac, ac_name)
{
  m_view = view;

  m_actionCollection = new KActionCollection( ac );

  reload();
}

KateCmdActionMenu::~KateCmdActionMenu()
{
  delete m_actionCollection;
}

void KateCmdActionMenu::reload()
{
  kDebug() << "___ reload KateCmdActionMenu" << endl;
  kMenu()->clear();
  m_actionCollection->clear();

  QHash<QString, KActionMenu*> hashmap;
  const QVector<KateCmdAction>& actions = KateCmdActionManager::self()->actions();

  for( QVector<KateCmdAction>::const_iterator it = actions.begin();
       it != actions.end();
       ++it )
  {
    // TODO: insert into action collection?
    const KateCmdAction& cmd = (*it);
    if( cmd.category.isEmpty() )
    {
      QAction* a = new QAction( cmd.name, this );
      a->setShortcut( cmd.shortcut );
      a->setStatusTip( cmd.description );
      a->setData( QVariant::fromValue( cmd.command ) );

      addAction( a );
    }
    else
    {
      if( !hashmap.contains( cmd.category ) )
        hashmap[cmd.category] = new KActionMenu( cmd.category, m_actionCollection, cmd.category );
      KActionMenu* submenu = hashmap[cmd.category];
      addAction( submenu );

      QAction* a = new QAction( cmd.name, submenu );
      a->setShortcut( cmd.shortcut );
      a->setStatusTip( cmd.description );
      a->setData( QVariant::fromValue( cmd.command ) );

      submenu->addAction( a );
    }
  }
}
//END KateCmdActionMenu

//BEGIN KateCmdActionManager
KateCmdActionManager::KateCmdActionManager()
{
//   QList<KTextEditor::Command*> cmds = KateCmd::self()->commands();
//   for( QList<KTextEditor::Command*>::iterator it = cmds.begin();
//        it != cmds.end();
//        ++it )
//   {
//     KTextEditor::Command* cmd = *it;
//     QStringList l = cmd->cmds();
//     
//     for( int i = 0; i < l.size(); ++i )
//     {
//       m_actions.append( KateCmdAction() );
//       KateCmdAction& a = m_actions.last();
// 
//       a.name = cmd->name( l[i] );
//       a.description = cmd->description( l[i] );
//       a.command = l[i];
//       a.category = "Test";
//     }
//   }
}

KateCmdActionManager::~KateCmdActionManager()
{
}

KateCmdActionManager *KateCmdActionManager::self()
{
  return KateGlobal::self()->cmdActionManager ();
}

const QVector<KateCmdAction>& KateCmdActionManager::actions() const
{
  return m_actions;
}

void KateCmdActionManager::setActions( const QVector<KateCmdAction>& actions )
{
  m_actions = actions;
  updateViews();
}

void KateCmdActionManager::readConfig( KConfig* config )
{
  if( !config ) return;
  m_actions.clear();

  const QString oldGroup = config->group();
  const int size = config->readEntry( "Commands", 0 );

  // read every entry from its own group
  for( int i = 0; i < size; ++i )
  {
    config->setGroup( QString("Kate Command Binding %1").arg( i ) );

    m_actions.append( KateCmdAction() );
    KateCmdAction& a = m_actions.last();

    a.name = config->readEntry( "Name", i18n("Unnamed") );
    a.description = config->readEntry( "Description", i18n("No description available.") );
    a.command = config->readEntry( "Command", QString() );
    a.category = config->readEntry( "Category", QString() );
    a.shortcut = QKeySequence::fromString(
        config->readEntry( "Shortcut", QString() ) );
  }
  config->setGroup( oldGroup );

  // sort, so that the menus are populated in the right order
  qSort( m_actions.begin(), m_actions.end() );

  updateViews();
}

void KateCmdActionManager::writeConfig( KConfig* config )
{
  if( !config ) return;
  const QString oldGroup = config->group();

  config->writeEntry( "Commands", m_actions.size() );

  int i = 0;
  QVector<KateCmdAction>::const_iterator it = m_actions.begin();
  for( ; it != m_actions.end(); ++it )
  {
    config->setGroup( QString("Kate Command Binding %1").arg( i ) );
    const KateCmdAction& a = *it;

    config->writeEntry( "Name", a.name );
    config->writeEntry( "Description", a.description );
    config->writeEntry( "Command", a.command );
    config->writeEntry( "Category", a.category );
    config->writeEntry( "Shortcut", a.shortcut.toString() );
    ++i;
  }
  config->setGroup( oldGroup );
}

void KateCmdActionManager::updateViews()
{
  QList<KateView*>& views = KateGlobal::self()->views();

  for( QList<KateView*>::iterator it = views.begin();
       it != views.end(); ++it )
  {
    KateCmdActionMenu* menu =
      qobject_cast<KateCmdActionMenu*>( (*it)->action( "tools_commands" ) );

    if( menu )
      menu->reload();
  }
}
//END KateCmdActionManager

//BEGIN KateCommandMenuConfigPage
KateCommandMenuConfigPage::KateCommandMenuConfigPage( QWidget *parent )
  : KateConfigPage( parent )
{
  setObjectName("kate-command-menu-config-page");
  ui = new Ui::CommandMenuConfigWidget();
  ui->setupUi( this );

  ui->lblIcon->setPixmap( BarIcon("idea", 22 ) );

  const QVector<KateCmdAction>& actions = KateCmdActionManager::self()->actions();

  // populate tree widget
  QHash<QString, KateCmdActionItem*> hashmap; // contains toplevel items
  for( QVector<KateCmdAction>::const_iterator it = actions.begin();
       it != actions.end(); ++it )
  {
    KateCmdAction* a = new KateCmdAction();
    *a = *it;
    m_actions.append( a );
    KateCmdActionItem* top = 0;
    if( a->category.isEmpty() ) a->category = i18n("Other");
    if( !hashmap.contains( a->category ) )
      hashmap[a->category] = new KateCmdActionItem( ui->treeWidget, a->category );
    top = hashmap[a->category];
    ui->treeWidget->expandItem( top );

    new KateCmdActionItem( top, QStringList() << a->name << a->command << a->description, a );
  }
  
  connect( ui->treeWidget, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ),
           this, SLOT( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
  connect( ui->btnAddEntry, SIGNAL( clicked() ), this, SLOT( addEntry() ) );
  connect( ui->btnEditEntry, SIGNAL( clicked() ), this, SLOT( editEntry() ) );
  connect( ui->btnRemoveEntry, SIGNAL( clicked() ), this, SLOT( removeEntry() ) );

  m_changed = false;
}

KateCommandMenuConfigPage::~KateCommandMenuConfigPage()
{
  qDeleteAll( m_actions );
  m_actions.clear();
}

void KateCommandMenuConfigPage::currentItemChanged( QTreeWidgetItem* current,
                                                    QTreeWidgetItem* previous )
{
  Q_UNUSED( previous );
  KateCmdActionItem* item = static_cast<KateCmdActionItem*>( current );
  ui->btnEditEntry->setEnabled( item->m_action != 0 );
  ui->btnRemoveEntry->setEnabled( item->m_action != 0 );
}


void KateCommandMenuConfigPage::apply()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  QVector<KateCmdAction> actions;
  for( QList<KateCmdAction*>::iterator it = m_actions.begin();
       it != m_actions.end(); ++it )
  {
    actions.append( **it );
  }

  KateCmdActionManager::self()->setActions( actions );
}

void KateCommandMenuConfigPage::reload()
{
}

void KateCommandMenuConfigPage::reset()
{
}

void KateCommandMenuConfigPage::defaults()
{
}

void KateCommandMenuConfigPage::addEntry()
{
  // collect autocompletion infos
  QStringList categories;
  const QVector<KateCmdAction>& actions = KateCmdActionManager::self()->actions();
  for( int i = 0; i < actions.size(); ++i )
    categories.append( actions[i].category );
  categories.sort(); // duplicate entries will be removed by the combo box
  QStringList commands = KateCmd::self()->commandList();
  commands.sort();

  KateCmdMenuEditDialog dlg( this );
  dlg.ui->cmbCategory->addItems( categories );
  dlg.ui->cmbCategory->clearEditText();
  dlg.ui->cmbCommand->addItems( commands );
  dlg.ui->cmbCommand->clearEditText();

  if( dlg.exec() == KDialog::Accepted)
  {
    const QString category = dlg.ui->cmbCategory->currentText().isEmpty() ?
      i18n("Other") : dlg.ui->cmbCategory->currentText();

    // find new category, maybe it already exists
    KateCmdActionItem* top = 0;
    for( int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i )
    {
      top = static_cast<KateCmdActionItem*>( ui->treeWidget->topLevelItem( i ) );
      if( top->text( 0 ) == category )
        break;
      top = 0;
    }
    if( !top ) top = new KateCmdActionItem( ui->treeWidget, category );
    ui->treeWidget->expandItem( top );
    KateCmdAction* a = new KateCmdAction();
    a->name = dlg.ui->edtName->text();
    a->description = dlg.ui->edtDescription->text();
    a->command = dlg.ui->cmbCommand->currentText();
    a->category = category;
    new KateCmdActionItem( top, QStringList() << a->name << a->command << a->description, a );
    m_actions.append( a );

    slotChanged();
  }
}

void KateCommandMenuConfigPage::editEntry()
{
  KateCmdActionItem* item =
      static_cast<KateCmdActionItem*>( ui->treeWidget->currentItem() );
  if( !item || !item->m_action ) return;

  // collect autocompletion infos
  QStringList categories;
  const QVector<KateCmdAction>& actions = KateCmdActionManager::self()->actions();
  for( int i = 0; i < actions.size(); ++i )
    categories.append( actions[i].category );
  categories.sort(); // duplicate entries will be removed by the combo box
  QStringList commands = KateCmd::self()->commandList();
  commands.sort();

  KateCmdMenuEditDialog dlg( this );
  dlg.ui->cmbCategory->addItems( categories );
  dlg.ui->cmbCommand->addItems( commands );

  dlg.ui->edtName->setText( item->m_action->name );
  dlg.ui->edtDescription->setText( item->m_action->description );
  dlg.ui->cmbCommand->setEditText( item->m_action->command );
  dlg.ui->cmbCategory->setEditText( item->m_action->category );

  if( dlg.exec() == KDialog::Accepted)
  {
    QTreeWidgetItem* oldParent = item->parent();
    oldParent->takeChild( oldParent->indexOfChild( item ) );

    const QString category = dlg.ui->cmbCategory->currentText().isEmpty() ?
        i18n("Other") : dlg.ui->cmbCategory->currentText();

    // find new category, maybe it already exists
    KateCmdActionItem* top = 0;
    for( int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i )
    {
      top = static_cast<KateCmdActionItem*>( ui->treeWidget->topLevelItem( i ) );
      if( top->text( 0 ) == category )
        break;
      top = 0;
    }
    if( !top ) top = new KateCmdActionItem( ui->treeWidget, category );
    top->addChild( item );
    ui->treeWidget->expandItem( top );

    item->m_action->name = dlg.ui->edtName->text();
    item->m_action->description = dlg.ui->edtDescription->text();
    item->m_action->command = dlg.ui->cmbCommand->currentText();
    item->m_action->category = dlg.ui->cmbCategory->currentText();
    item->setText( 0, item->m_action->name );
    item->setText( 0, item->m_action->description );
    item->setText( 0, item->m_action->command );

    if( oldParent->childCount() == 0 )
      delete oldParent;

    slotChanged();
  }
}

void KateCommandMenuConfigPage::removeEntry()
{
  KateCmdActionItem* item =
    static_cast<KateCmdActionItem*>( ui->treeWidget->currentItem() );
  if( !item || !item->m_action ) return;

  QTreeWidgetItem* top = item->parent();
  delete m_actions.takeAt( m_actions.indexOf( item->m_action ) );
  delete item;
  if( top && top->childCount() == 0 ) delete top;
  slotChanged();
}
//END KateCommandMenuConfigPage

//BEGIN CommandMenuEditWidget
KateCmdMenuEditDialog::KateCmdMenuEditDialog( QWidget *parent )
  : KDialog( parent, i18n("Edit Entry"), Ok | Cancel )
{
  enableButtonSeparator( true );

  QWidget *w = new QWidget( this );
  ui = new Ui::CommandMenuEditWidget();
  ui->setupUi( w );
  setMainWidget( w );

  connect( ui->cmbCommand, SIGNAL( activated( const QString& ) ),
           this, SLOT( commandChanged( const QString& ) ) );
}

KateCmdMenuEditDialog::~KateCmdMenuEditDialog()
{
}

void KateCmdMenuEditDialog::commandChanged( const QString& text )
{
  KTextEditor::Command* cmd = KateCmd::self()->queryCommand( text );
  if( cmd )
  {
    ui->edtName->setText( cmd->name( text ) );
    ui->edtDescription->setText( cmd->description( text ) );
  }
}
//END CommandMenuEditWidget


// kate: space-indent on; indent-width 2; replace-tabs on;
