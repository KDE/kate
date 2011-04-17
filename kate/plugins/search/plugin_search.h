/*   Kate search plugin
 * 
 * Copyright (C) 2011 by Kåre Särs <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file called COPYING; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef _PLUGIN_SEARCH_H_
#define _PLUGIN_SEARCH_H_

#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kxmlguiclient.h>
#include <kaction.h>

#include "ui_search.h"

#include "search_open_files.h"
#include "search_folder.h"


class KatePluginSearch : public Kate::Plugin
{
    Q_OBJECT

public:
    explicit KatePluginSearch(QObject* parent = 0, const QList<QVariant>& = QList<QVariant>());
    virtual ~KatePluginSearch();

    Kate::PluginView *createView(Kate::MainWindow *mainWindow);
};



class KatePluginSearchView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

public:
    KatePluginSearchView(Kate::MainWindow *mainWindow, Kate::Application* application);
    ~KatePluginSearchView();

    virtual void readSessionConfig(KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig(KConfigBase* config, const QString& groupPrefix);

private Q_SLOTS:
    void toggleSearchView();
    void startSearch();
    void toggleOptions(bool show);

    void searchPlaceChanged();
    void searchPatternChanged();
    
    void matchFound(const QString &fileName, int line, const QString &lineContent);
    void searchDone();

    void itemSelected(QTreeWidgetItem *item);
    
private:
    Ui::SearchDialog   m_ui;
    QWidget           *m_toolView;
    Kate::Application *m_kateApp;
    SearchOpenFiles    m_searchOpenFiles;
    SearchFolder       m_searchFolder;
    KAction           *m_matchCase;
    KAction           *m_useRegExp;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;

