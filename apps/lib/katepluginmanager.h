/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KPluginMetaData>

class KConfig;
class KateMainWindow;
class KConfigBase;
namespace KTextEditor
{
class Plugin;
}

class KatePluginInfo
{
public:
    bool load = false;
    bool defaultLoad = false;
    KPluginMetaData metaData;
    KTextEditor::Plugin *plugin = nullptr;
    int sortOrder = 0;
    QString saveName() const;
    bool operator<(const KatePluginInfo &other) const;
};

typedef std::vector<KatePluginInfo> KatePluginList;

class KatePluginManager
{
public:
    explicit KatePluginManager();
    ~KatePluginManager();

    void unloadAllPlugins();

    void enableAllPluginsGUI(KateMainWindow *win, KConfigBase *config = nullptr);
    void disableAllPluginsGUI(KateMainWindow *win);

    void loadConfig(KConfig *);
    void writeConfig(KConfig *);

    bool loadPlugin(KatePluginInfo *item);
    void unloadPlugin(KatePluginInfo *item);

    static void enablePluginGUI(KatePluginInfo *item, KateMainWindow *win, KConfigBase *config = nullptr);
    static void enablePluginGUI(KatePluginInfo *item);

    static void disablePluginGUI(KatePluginInfo *item, KateMainWindow *win);
    static void disablePluginGUI(KatePluginInfo *item);

    inline KatePluginList &pluginList()
    {
        return m_pluginList;
    }

    KTextEditor::Plugin *plugin(const QString &name);

private:
    void setupPluginList();

    /**
     * all known plugins
     */
    KatePluginList m_pluginList;
};
