/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_PLUGINMANAGER_H__
#define __KATE_PLUGINMANAGER_H__

#include "katemain.h"

#include <kate/plugin.h>
#include <kate/pluginmanager.h>

#include <KService>
#include <KConfigBase>

#include <QObject>
#include <QLinkedList>

class KateMainWindow;

class KatePluginInfo
{
  public:
    mutable bool load;
    KService::Ptr service;
    Kate::Plugin *plugin;
    QString saveName() const;
};

typedef QLinkedList<KatePluginInfo> KatePluginList;

class KatePluginManager : public QObject
{
    Q_OBJECT

  public:
    KatePluginManager(QObject *parent);
    ~KatePluginManager();

    static KatePluginManager *self();

    Kate::PluginManager *pluginManager () const
    {
      return m_pluginManager;
    };

    void unloadAllPlugins ();

    void enableAllPluginsGUI (KateMainWindow *win, KConfigBase *config = 0);
    void disableAllPluginsGUI (KateMainWindow *win);

    void loadConfig (KConfig*);
    void writeConfig (KConfig*);

    void loadPlugin (KatePluginInfo *item);
    void unloadPlugin (KatePluginInfo *item);

    void enablePluginGUI (KatePluginInfo *item, KateMainWindow *win, KConfigBase *config = 0);
    void enablePluginGUI (KatePluginInfo *item);

    void disablePluginGUI (KatePluginInfo *item, KateMainWindow *win);
    void disablePluginGUI (KatePluginInfo *item);

    inline KatePluginList & pluginList ()
    {
      return m_pluginList;
    };

    Kate::Plugin *plugin (const QString &name);
    bool pluginAvailable (const QString &name);

    Kate::Plugin *loadPlugin (const QString &name, bool permanent = true);
    void unloadPlugin (const QString &name, bool permanent = true);

  private:
    Kate::PluginManager *m_pluginManager;

    void setupPluginList ();

    KatePluginList m_pluginList;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

