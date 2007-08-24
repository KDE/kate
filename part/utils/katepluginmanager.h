/* This file is part of the KDE project
   Copyright 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright 2007 Dominik Haumann <dhaumann kde org>

   Taken from kdesdk/kate/app/katepluginmanager.cpp/h and adapted

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

#ifndef __KATEPART_PLUGINMANAGER_H__
#define __KATEPART_PLUGINMANAGER_H__

#include <kservice.h>
#include <kconfigbase.h>

#include <QObject>
#include <QList>

namespace KTextEditor {
  class Plugin;
  class Document;
  class View;
}

class KatePluginInfo
{
  public:
    mutable bool load;
    KService::Ptr service;
    KTextEditor::Plugin *plugin;
    QString saveName() const;
};

typedef QList<KatePluginInfo> KatePluginList;

class KatePluginManager : public QObject
{
    Q_OBJECT

  public:
    KatePluginManager();
    ~KatePluginManager();

    static KatePluginManager *self();

    void loadConfig ();
    void writeConfig ();

    void addDocument(KTextEditor::Document *doc);
    void removeDocument(KTextEditor::Document *doc);

    void addView(KTextEditor::View *view);
    void removeView(KTextEditor::View *view);

    void loadAllPlugins ();
    void unloadAllPlugins ();

    void loadPlugin (KatePluginInfo &item);
    void unloadPlugin (KatePluginInfo &item);

    void enablePlugin (KatePluginInfo &item);
    void disablePlugin (KatePluginInfo &item);

    inline KatePluginList & pluginList ()
    {
      return m_pluginList;
    }

    KTextEditor::Plugin *createPlugin(KService::Ptr service);

  private:
    void setupPluginList ();

    KConfig *m_config;
    KatePluginList m_pluginList;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

