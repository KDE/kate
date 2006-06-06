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

// used for qSort
bool operator<( const KateCmdBinding& a, const KateCmdBinding& b )
{
  return QString::localeAwareCompare(a.name, b.name) == -1;
}

//BEGIN KateCmdActionItem
class KateCmdActionItem : public QTreeWidgetItem
{
  public:
    KateCmdActionItem( QTreeWidget* parent, const QString& text )
      : QTreeWidgetItem( parent, QStringList() << text ), m_action( 0 ) {}
    KateCmdActionItem( QTreeWidgetItem* parent, const QStringList& strings, KateCmdBinding* action )
      : QTreeWidgetItem( parent, strings ), m_action( action ) {}
    virtual ~KateCmdActionItem() {}

  public:
    KateCmdBinding* m_action;
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
  // FIXME: do I have to delete it myself?
//   delete m_actionCollection;
}

void KateCmdActionMenu::reload()
{
  // 1. clear actions / action collection
  // 2. iterate actions from action manager
  // 3. add new actions, along with a category
  // 4. add (sorted) submenus
//   kDebug() << "___ reload KateCmdActionMenu" << endl;
//   kMenu()->clear();
  m_actionCollection->clear();

  QMap<QString, KActionMenu*> menumap;
  const QVector<KateCmdBinding>& actions = KateCmdBindingManager::self()->actions();

  for( QVector<KateCmdBinding>::const_iterator it = actions.begin();
       it != actions.end();
       ++it )
  {
    const KateCmdBinding& cmd = (*it);
    if( !menumap.contains( cmd.category ) )
      menumap[cmd.category] = new KActionMenu( cmd.category, m_actionCollection, cmd.category );
    KActionMenu* submenu = menumap[cmd.category];

    KateCmdAction* a = new KateCmdAction( cmd.name, m_actionCollection,
                                          cmd.command, m_view, &cmd );
    a->setShortcut( cmd.shortcut );
    a->setStatusTip( cmd.description );
    a->setData( QVariant::fromValue( cmd.command ) );

    submenu->addAction( a );
  }

  // now insert the submenus, as they are automatically sorted by QMap
  QMap<QString, KActionMenu*>::const_iterator it = menumap.constBegin();
  while( it != menumap.constEnd() )
  {
    addAction( it.value() );
    ++it;
  }
}
//END KateCmdActionMenu

//BEGIN KateCmdAction
KateCmdAction::KateCmdAction( const QString& text, KActionCollection* parent,
                              const QString& name, KTextEditor::View* view,
                              const KateCmdBinding* binding )
  : KAction( text, parent, name), m_view( view ), m_binding( binding) 
{
  connect( this, SIGNAL( triggered( bool ) ), this, SLOT( slotRun() ) );
}

KateCmdAction::~KateCmdAction() {}

void KateCmdAction::slotRun()
{
  KTextEditor::Command* command = KateGlobal::self()->queryCommand( m_binding->command );
  if( command )
  {
    QString returnMessage;
    command->exec( m_view, m_binding->command, returnMessage );
    kDebug() << "executed command, return code: " << returnMessage << endl;
  }
}
//END KateCmdAction

//BEGIN KateCmdBindingManager
KateCmdBindingManager::KateCmdBindingManager()
{
}

KateCmdBindingManager::~KateCmdBindingManager()
{
}

KateCmdBindingManager *KateCmdBindingManager::self()
{
  return KateGlobal::self()->cmdBindingManager ();
}

const QVector<KateCmdBinding>& KateCmdBindingManager::actions() const
{
  return m_actions;
}

void KateCmdBindingManager::setActions( const QVector<KateCmdBinding>& actions )
{
  // 1. assign new actions
  // 2. update all KateViews to reflect the changes
  m_actions = actions;
  updateViews();
}

void KateCmdBindingManager::readConfig( KConfig* config )
{
  if( !config ) return;
  m_actions.clear();

  const QString oldGroup = config->group();
  const int size = config->readEntry( "Commands", 0 );

  // read every entry from its own group
  for( int i = 0; i < size; ++i )
  {
    config->setGroup( QString("Kate Command Binding %1").arg( i ) );

    m_actions.append( KateCmdBinding() );
    KateCmdBinding& a = m_actions.last();

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

void KateCmdBindingManager::writeConfig( KConfig* config )
{
  if( !config ) return;
  const QString oldGroup = config->group();

  config->writeEntry( "Commands", m_actions.size() );

  int i = 0;
  QVector<KateCmdBinding>::const_iterator it = m_actions.begin();
  for( ; it != m_actions.end(); ++it )
  {
    config->setGroup( QString("Kate Command Binding %1").arg( i ) );
    const KateCmdBinding& a = *it;

    config->writeEntry( "Name", a.name );
    config->writeEntry( "Description", a.description );
    config->writeEntry( "Command", a.command );
    config->writeEntry( "Category", a.category );
    config->writeEntry( "Shortcut", a.shortcut.toString() );
    ++i;
  }
  config->setGroup( oldGroup );
}

void KateCmdBindingManager::updateViews()
{
  QList<KateView*>& views = KateGlobal::self()->views();

  // force every view to reload the command bindings
  for( QList<KateView*>::iterator it = views.begin();
       it != views.end(); ++it )
  {
    KateCmdActionMenu* menu =
      qobject_cast<KateCmdActionMenu*>( (*it)->action( "tools_commands" ) );

    if( menu )
      menu->reload();
  }
}
//END KateCmdBindingManager

//BEGIN KateCmdBindingConfigPage
KateCmdBindingConfigPage::KateCmdBindingConfigPage( QWidget *parent )
  : KateConfigPage( parent )
{
  setObjectName( "kate-command-binding-config-page" );
  ui = new Ui::CmdBindingConfigWidget();
  ui->setupUi( this );

  ui->lblIcon->setPixmap( BarIcon("idea", 22 ) );

  const QVector<KateCmdBinding>& actions = KateCmdBindingManager::self()->actions();

  // populate tree widget
  QHash<QString, KateCmdActionItem*> hashmap; // contains toplevel items
  for( QVector<KateCmdBinding>::const_iterator it = actions.begin();
       it != actions.end(); ++it )
  {
    KateCmdBinding* a = new KateCmdBinding();
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

KateCmdBindingConfigPage::~KateCmdBindingConfigPage()
{
  qDeleteAll( m_actions );
  m_actions.clear();
}

void KateCmdBindingConfigPage::currentItemChanged( QTreeWidgetItem* current,
                                                     QTreeWidgetItem* previous )
{
  // if selection changed, enable/disable buttons accordingly
  Q_UNUSED( previous );
  KateCmdActionItem* item = static_cast<KateCmdActionItem*>( current );
  ui->btnEditEntry->setEnabled( item->m_action != 0 );
  ui->btnRemoveEntry->setEnabled( item->m_action != 0 );
}


void KateCmdBindingConfigPage::apply()
{
  // nothing changed, no need to apply stuff
  if (!hasChanged())
    return;
  m_changed = false;

  // convert QList into QVector
  QVector<KateCmdBinding> actions;
  for( QList<KateCmdBinding*>::iterator it = m_actions.begin();
       it != m_actions.end(); ++it )
  {
    actions.append( **it );
  }

  // finally set the new actions
  KateCmdBindingManager::self()->setActions( actions );
}

void KateCmdBindingConfigPage::reload()
{
}

void KateCmdBindingConfigPage::reset()
{
}

void KateCmdBindingConfigPage::defaults()
{
}

void KateCmdBindingConfigPage::addEntry()
{
  // collect autocompletion infos
  QStringList categories;
  const QVector<KateCmdBinding>& actions = KateCmdBindingManager::self()->actions();
  for( int i = 0; i < actions.size(); ++i )
    categories.append( actions[i].category );
  categories.sort(); // duplicate entries will be removed by the combo box
  QStringList commands = KateCmd::self()->commandList();
  commands.sort();

  KateCmdBindingEditDialog dlg( this );
  dlg.ui->cmbCategory->addItems( categories );
  dlg.ui->cmbCategory->clearEditText();
  dlg.ui->cmbCommand->addItems( commands );
  dlg.ui->cmbCommand->clearEditText();

  if( dlg.exec() == KDialog::Accepted)
  {
    const QString category = dlg.ui->cmbCategory->currentText();

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
    KateCmdBinding* a = new KateCmdBinding();
    a->name = dlg.ui->edtName->text();
    a->description = dlg.ui->edtDescription->text();
    a->command = dlg.ui->cmbCommand->currentText();
    a->category = category;
    new KateCmdActionItem( top, QStringList() << a->name << a->command << a->description, a );
    m_actions.append( a );

    slotChanged();
  }
}

void KateCmdBindingConfigPage::editEntry()
{
  KateCmdActionItem* item =
      static_cast<KateCmdActionItem*>( ui->treeWidget->currentItem() );
  if( !item || !item->m_action ) return;

  // collect autocompletion infos
  QStringList categories;
  const QVector<KateCmdBinding>& actions = KateCmdBindingManager::self()->actions();
  for( int i = 0; i < actions.size(); ++i )
    categories.append( actions[i].category );
  categories.sort(); // duplicate entries will be removed by the combo box
  QStringList commands = KateCmd::self()->commandList();
  commands.sort();

  KateCmdBindingEditDialog dlg( this );
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

    const QString category = dlg.ui->cmbCategory->currentText();

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
    item->setText( 1, item->m_action->description );
    item->setText( 2, item->m_action->command );

    if( oldParent->childCount() == 0 )
      delete oldParent;

    slotChanged();
  }
}

void KateCmdBindingConfigPage::removeEntry()
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
//END KateCmdBindingConfigPage

//BEGIN KateCmdBindingEditDialog
KateCmdBindingEditDialog::KateCmdBindingEditDialog( QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n("Edit Entry") );
  setButtons( Ok | Cancel );
  enableButtonSeparator( true );

  QWidget *w = new QWidget( this );
  ui = new Ui::CmdBindingEditWidget();
  ui->setupUi( w );
  setMainWidget( w );

  connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
  connect( ui->cmbCommand, SIGNAL( activated( const QString& ) ),
           this, SLOT( commandChanged( const QString& ) ) );
}

KateCmdBindingEditDialog::~KateCmdBindingEditDialog()
{
}

void KateCmdBindingEditDialog::commandChanged( const QString& text )
{
  KTextEditor::Command* cmd = KateCmd::self()->queryCommand( text );
  if( cmd )
  {
    if( !cmd->name( text ).isEmpty() )
      ui->edtName->setText( cmd->name( text ) );
    if( !cmd->description( text ).isEmpty() )
      ui->edtDescription->setText( cmd->description( text ) );
    if( !cmd->category( text ).isEmpty() )
      ui->cmbCategory->setEditText( cmd->category( text ) );
  }
}

void KateCmdBindingEditDialog::slotOk()
{
  // prevent empty category
  if( ui->cmbCategory->currentText().isEmpty() )
    ui->cmbCategory->setEditText( i18n("Other") );
}
//END KateCmdBindingEditDialog


// kate: space-indent on; indent-width 2; replace-tabs on;
