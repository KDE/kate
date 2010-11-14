/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
 *  Copyright (C) 2007 Dominik Haumann <dhaumann kde org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATEPARTPLUGINMANAGER_H
#define KATEPARTPLUGINMANAGER_H

#include <kplugininfo.h>
#include <kservice.h>

#include <QObject>
#include <QList>

namespace KTextEditor {
  class Plugin;
  class Document;
  class View;
}

class KatePartPluginInfo
{
  public:
    KatePartPluginInfo(KService::Ptr service);
    mutable bool load;
    KTextEditor::Plugin *plugin;
    QString saveName() const;
    KService::Ptr service() const { return m_pluginInfo.service(); }
    QStringList dependencies() const { return m_pluginInfo.dependencies(); }
    bool isEnabledByDefault() const { return m_pluginInfo.isPluginEnabledByDefault(); }
  private:
    KPluginInfo m_pluginInfo;
    QString m_saveName;
};

typedef QList<KatePartPluginInfo> KatePartPluginList;

class KatePartPluginManager : public QObject
{
    Q_OBJECT

  public:
    KatePartPluginManager();
    ~KatePartPluginManager();

    static KatePartPluginManager *self();

    void loadConfig ();
    void writeConfig ();

    void addDocument(KTextEditor::Document *doc);
    void removeDocument(KTextEditor::Document *doc);

    void addView(KTextEditor::View *view);
    void removeView(KTextEditor::View *view);

    void loadAllPlugins ();
    void unloadAllPlugins ();

    void loadPlugin (KatePartPluginInfo &item);
    void unloadPlugin (KatePartPluginInfo &item);

    void enablePlugin (KatePartPluginInfo &item);
    void disablePlugin (KatePartPluginInfo &item);

    inline KatePartPluginList & pluginList ()
    {
      return m_pluginList;
    }

  private:
    void setupPluginList ();

    KConfig *m_config;
    KatePartPluginList m_pluginList;
};

#endif // KATEPARTPLUGINMANAGER_H

// kate: space-indent on; indent-width 2; replace-tabs on;
