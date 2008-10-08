/* This file is part of the KDE project
   Copyright (C) 2004 Stephan Möres <Erdling@gmx.net>
   Copyright (C) 2008 Jakob Petsovits <jpetso@gmx.at>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef _PLUGIN_KATESNIPPETS_H_
#define _PLUGIN_KATESNIPPETS_H_

#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kxmlguiclient.h>
#include <kconfig.h>

class KateSnippetsWidget;


class KatePluginSnippets : public Kate::Plugin
{
  Q_OBJECT

  public:
    explicit KatePluginSnippets( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KatePluginSnippets();

    Kate::PluginView *createView( Kate::MainWindow *mainWindow );
};


class KatePluginSnippetsView : public Kate::PluginView, public KXMLGUIClient
{
  Q_OBJECT

  public:
    explicit KatePluginSnippetsView( Kate::MainWindow *w );
    virtual ~KatePluginSnippetsView();

  protected:
    KateSnippetsWidget *kateSnippetsWidget() const { return m_snippetsWidget; }

  protected Q_SLOTS:
    void readConfig();
    void writeConfig();

  private:
    KConfig *m_config;
    QWidget *m_dock;
    KateSnippetsWidget *m_snippetsWidget;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
