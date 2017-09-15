/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <KTextEditor/Plugin>
#include <KTextEditor/ConfigPage>
#include <KTextEditor/Editor>
#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPageDialog>

class QCheckBox;
class QSpinBox;
class KateMainWindow;
class KPluralHandlingSpinBox;

namespace Ui {
    class SessionConfigWidget;
}

struct PluginPageListItem {
    KTextEditor::Plugin *plugin;
    uint idInPlugin;
    KTextEditor::ConfigPage *pluginPage;
    QWidget *pageParent;
    KPageWidgetItem *pageWidgetItem;
};

class KateConfigDialog : public KPageDialog
{
    Q_OBJECT

public:
    KateConfigDialog(KateMainWindow *parent, KTextEditor::View *view);
    ~KateConfigDialog() override;

public: // static
    /**
     * Reads the value from the given open config. If not present in config yet then
     * the default value 10 is used.
     */
    static int recentFilesMaxCount();

public:
    void addPluginPage(KTextEditor::Plugin *plugin);
    void removePluginPage(KTextEditor::Plugin *plugin);
    void showAppPluginPage(KTextEditor::Plugin *plugin, uint id);
protected Q_SLOTS:
    void slotApply();
    void slotChanged();
    void slotHelp();

    void slotCurrentPageChanged(KPageWidgetItem *current, KPageWidgetItem *before);

private:
    KateMainWindow *m_mainWindow;

    KTextEditor::View *m_view;
    bool m_dataChanged;

    QCheckBox *m_modNotifications;
    QCheckBox *m_saveMetaInfos;
    KPluralHandlingSpinBox *m_daysMetaInfos;

    // Sessions Page
    Ui::SessionConfigWidget *sessionConfigUi;

    QHash<KPageWidgetItem *, PluginPageListItem *> m_pluginPages;
    QList<KTextEditor::ConfigPage *> m_editorPages;
    KPageWidgetItem *m_applicationPage;
    KPageWidgetItem *m_editorPage;

    void addEditorPages();
};

#endif

