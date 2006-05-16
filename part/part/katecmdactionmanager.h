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

#ifndef KATE_COMMAND_ACTION_MANAGER
#define KATE_COMMAND_ACTION_MANAGER

#include <QString>
#include <QKeySequence>
#include <QList>
#include <QVector>

#include <kactionmenu.h>
#include <kdialog.h>

#include "katedialogs.h"

class KActionCollection;
class KConfig;

namespace KTextEditor
{
  class View;
}

namespace Ui
{
  class CmdBindingConfigWidget;
  class CmdBindingEditWidget;
}

/**
 * A Command binding has the following infos:
 * - name
 * - description
 * - command line string
 * - category
 * - key sequence/shortcut
 */
class KateCmdBinding
{
  public:
    QString name;
    QString description;
    QString command;
    QString category;
    QKeySequence shortcut;
};

/**
 * Action Menu, that will be hooked into the Tools menu. Its name is "Commands".
 */
class KateCmdActionMenu : public KActionMenu
{
  Q_OBJECT
  public:
    KateCmdActionMenu( KTextEditor::View* view,
                       const QString &text,
                       KActionCollection *ac,
                       const QString& ac_name );
    ~KateCmdActionMenu();

    /** Reload action menu. */
    void reload();

    /** return own action collection */
    KActionCollection *actionCollection() { return m_actionCollection; }

  private:
    KActionCollection *m_actionCollection;
    KTextEditor::View *m_view;
};

/**
 * A KAction, which contains a pointer to the corresponding KateCmdBinding.
 * If triggered, it will run the associated command.
 */
class KateCmdAction : public KAction
{
  Q_OBJECT
  public:
    KateCmdAction( const QString& text, KActionCollection* parent,
                   const QString& name, KTextEditor::View* view,
                   const KateCmdBinding* binding );
    ~KateCmdAction();

  private Q_SLOTS:
    void slotRun();

  public:
    KTextEditor::View* m_view;
    const KateCmdBinding* m_binding;
};

/**
 * The Command Action Manager contains a list of all bindings.
 * Additionally, it loads and saves the list.
 */
class KateCmdBindingManager
{
  public:
    KateCmdBindingManager();
    ~KateCmdBindingManager();

    static KateCmdBindingManager *self();

    void readConfig( KConfig* config );
    void writeConfig( KConfig* config );

  public:
    const QVector<KateCmdBinding>& actions() const;
    void setActions( const QVector<KateCmdBinding>& actions );

  protected:
    /** iterate all KateViews and reload the action menu*/
    void updateViews();

  private:
    QVector<KateCmdBinding> m_actions;
};

/**
 * Config page for the Command Action Manager.
 * It provides functions to add/remove/edit bindings. If the changes are applied
 * KateCmdBindingManager itself will call KateCmdBindingManager::updateViews();
 */
class KateCmdBindingConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    KateCmdBindingConfigPage( QWidget *parent );
    ~KateCmdBindingConfigPage();

  protected Q_SLOTS:
    void addEntry();
    void editEntry();
    void removeEntry();
    void currentItemChanged( class QTreeWidgetItem* current,
                             class QTreeWidgetItem* previous );

  protected:
    Ui::CmdBindingConfigWidget* ui;
    QList<KateCmdBinding*> m_actions;

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset ();
    void defaults ();
};

/**
 * Dialog to modify a single KateCmdBinding.
 */
class KateCmdBindingEditDialog : public KDialog
{
  Q_OBJECT
  public:
    KateCmdBindingEditDialog( QWidget *parent );
    virtual ~KateCmdBindingEditDialog();

  public:
    Ui::CmdBindingEditWidget* ui;

  protected Q_SLOTS:
    void commandChanged( const QString& text );
    void slotOk();
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
