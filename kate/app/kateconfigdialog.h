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

#ifndef __kate_configdialog_h__
#define __kate_configdialog_h__

#include "katemain.h"

#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>

#include <KTextEditor/Document>
#include <KTextEditor/EditorChooser>

#include <KPageDialog>
#include <QList>

class QCheckBox;
class QSpinBox;
class Q3ButtonGroup;
class KateMainWindow;

struct PluginPageListItem
{
  Kate::Plugin *plugin;
  Kate::PluginConfigPage *page;
};

class KateConfigDialog : public KPageDialog
{
    Q_OBJECT

  public:
    KateConfigDialog (KateMainWindow *parent, KTextEditor::View *view);
    ~KateConfigDialog ();

  public:
    void addPluginPage (Kate::Plugin *plugin, KPageWidgetItem *parent = 0);
    void removePluginPage (Kate::Plugin *plugin);

  protected Q_SLOTS:
    void slotOk();
    void slotApply();
    void slotChanged();

  private:
    KateMainWindow *mainWindow;

    KTextEditor::View* v;
    bool dataChanged;

    QCheckBox *cb_modNotifications;
    QCheckBox *cb_saveMetaInfos;
    QSpinBox *sb_daysMetaInfos;
    QCheckBox* cb_restoreVC;
    Q3ButtonGroup *sessions_start;
    Q3ButtonGroup *sessions_exit;
    QList<PluginPageListItem *> pluginPages;
    QList<KTextEditor::ConfigPage *> editorPages;
    KTextEditor::EditorChooser *m_editorChooser;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

