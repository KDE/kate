/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_PLUGINMANAGER_H__
#define __KATE_PLUGINMANAGER_H__

#include <KTextEditor/Plugin>

#include <KPluginMetaData>
#include <KConfigBase>

#include <QObject>
#include <QList>
#include <QMap>

class KConfig;
class KateMainWindow;

class KatePluginInfo
{
public:
    KatePluginInfo()
        : load(false)
        , defaultLoad(false)
        , plugin(nullptr)
    {}
    bool load;
    bool defaultLoad;
    KPluginMetaData metaData;
    KTextEditor::Plugin *plugin;
    QString saveName() const;
};

typedef QList<KatePluginInfo> KatePluginList;

class KatePluginManager : public QObject
{
    Q_OBJECT

public:
    KatePluginManager(QObject *parent);
    ~KatePluginManager() override;

    void unloadAllPlugins();

    void enableAllPluginsGUI(KateMainWindow *win, KConfigBase *config = nullptr);
    void disableAllPluginsGUI(KateMainWindow *win);

    void loadConfig(KConfig *);
    void writeConfig(KConfig *);

    bool loadPlugin(KatePluginInfo *item);
    void unloadPlugin(KatePluginInfo *item);

    void enablePluginGUI(KatePluginInfo *item, KateMainWindow *win, KConfigBase *config = nullptr);
    void enablePluginGUI(KatePluginInfo *item);

    void disablePluginGUI(KatePluginInfo *item, KateMainWindow *win);
    void disablePluginGUI(KatePluginInfo *item);

    inline KatePluginList &pluginList() {
        return m_pluginList;
    }

    KTextEditor::Plugin *plugin(const QString &name);
    bool pluginAvailable(const QString &name);

    KTextEditor::Plugin *loadPlugin(const QString &name, bool permanent = true);
    void unloadPlugin(const QString &name, bool permanent = true);

private:
    void setupPluginList();

    /**
     * all known plugins
     */
    KatePluginList m_pluginList;

    /**
     * fast access map from name => plugin info
     * uses the info stored in the plugin list
     */
    QMap<QString, KatePluginInfo *> m_name2Plugin;
};

#endif
