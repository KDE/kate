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
  class CommandMenuConfigWidget;
  class CommandMenuEditWidget;
}

/**
 * A Command Action has the following infos:
 * - name
 * - description
 * - command line string
 * - category
 * - global flag: if true, all apps using kate part will show the command
 * - key sequence/shortcut
 */
class KateCmdAction
{
  public:
    QString name;
    QString description;
    QString command;
    QString category;
    bool global;
    QKeySequence shortcut;
};

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

    KActionCollection *actionCollection() { return m_actionCollection; }

  private:
    KActionCollection *m_actionCollection;
    KTextEditor::View *m_view;
};

class KateCmdActionManager
{
  public:
    KateCmdActionManager();
    ~KateCmdActionManager();

    static KateCmdActionManager *self();

    void readConfig( KConfig* config );
    void writeConfig( KConfig* config );

  public:
    const QVector<KateCmdAction>& actions() const;
    void setActions( const QVector<KateCmdAction>& actions );

  protected:
    void updateViews();

  private:
    QVector<KateCmdAction> m_actions;
};

class KateCommandMenuConfigPage : public KateConfigPage
{
  Q_OBJECT

  public:
    KateCommandMenuConfigPage( QWidget *parent );
    ~KateCommandMenuConfigPage();

  protected Q_SLOTS:
    void addEntry();
    void editEntry();
    void removeEntry();
    void currentItemChanged( class QTreeWidgetItem* current,
                             class QTreeWidgetItem* previous );

  protected:
    Ui::CommandMenuConfigWidget* ui;
    QList<KateCmdAction*> m_actions;

  public Q_SLOTS:
    void apply ();
    void reload ();
    void reset ();
    void defaults ();
};

class KateCmdMenuEditDialog : public KDialog
{
  Q_OBJECT
  public:
    KateCmdMenuEditDialog( QWidget *parent );
    virtual ~KateCmdMenuEditDialog();

  public:
    Ui::CommandMenuEditWidget* ui;

  protected Q_SLOTS:
    void commandChanged( const QString& text );
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
