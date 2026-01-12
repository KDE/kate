/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#include "katepluginmanager.h"

#include "kateapp.h"
#include "katemainwindow.h"

#include <KConfig>
#include <KConfigBase>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KTextEditor/Plugin>
#include <KTextEditor/SessionConfigInterface>

#include "kate_timings_debug.h"
#include <QElapsedTimer>
#include <QFileInfo>
#include <memory_resource>
#include <unordered_set>

QString KatePluginInfo::saveName() const
{
    return QFileInfo(metaData.fileName()).baseName();
}

bool KatePluginInfo::operator<(const KatePluginInfo &other) const
{
    if (sortOrder != other.sortOrder) {
        return sortOrder < other.sortOrder;
    }

    return saveName() < other.saveName();
}

KatePluginManager::KatePluginManager()
{
    setupPluginList();
}

KatePluginManager::~KatePluginManager()
{
    unloadAllPlugins();
}

void KatePluginManager::setupPluginList()
{
    // no plugins for KWrite mode
    if (KateApp::isKWrite()) {
        Q_ASSERT(m_pluginList.empty());
        return;
    }

    // activate a hand-picked list of plugins per default, give them a hand-picked sort order for loading
    struct PluginNameAndSortOrder {
        const char *name;
        int sortOrder;
    };

    constexpr PluginNameAndSortOrder defaultPlugins[] = {
        {.name = "katefiletreeplugin", .sortOrder = -1000},
        {.name = "katesearchplugin", .sortOrder = -900},
        {.name = "kateprojectplugin", .sortOrder = -800},
        {.name = "tabswitcherplugin", .sortOrder = -100},
        {.name = "textfilterplugin", .sortOrder = -100},
        {.name = "externaltoolsplugin", .sortOrder = -100},
        {.name = "lspclientplugin", .sortOrder = -100},
        {.name = "katekonsoleplugin", .sortOrder = -100},
    };

    // handle all install KTextEditor plugins
    m_pluginList.clear();

    std::byte buffer[65 * 1000];
    std::pmr::monotonic_buffer_resource memory(buffer, sizeof(buffer));
    std::pmr::unordered_set<QString> unique(&memory);

    const QList<KPluginMetaData> plugins = KPluginMetaData::findPlugins(QStringLiteral("kf6/ktexteditor"));
    m_pluginList.reserve(plugins.size());

    for (const auto &pluginMetaData : plugins) {
        KatePluginInfo info;
        info.metaData = pluginMetaData;
        const QString saveName = info.saveName();

        // only load plugins once, even if found multiple times!unique
        if (unique.contains(saveName)) {
            continue;
        }

        for (auto dp : defaultPlugins) {
            if (QLatin1String(dp.name) == saveName) {
                // ensure during unit tests, the plugins don't interfere
                // the lsp stuff for example might want to start lsp servers
                info.defaultLoad = !QStandardPaths::isTestModeEnabled();
                info.sortOrder = dp.sortOrder;
                break;
            }
        }

        info.load = info.defaultLoad; // keep this in load, too, to avoid new sessions kill that info on writeConfig
        info.plugin = nullptr;
        m_pluginList.push_back(info);
        unique.insert(saveName);
    }

    // sort to ensure some deterministic plugin load order, this is important for tool-view creation order
    std::sort(m_pluginList.begin(), m_pluginList.end());
}

void KatePluginManager::loadConfig(KConfig *config)
{
    // first: unload the plugins
    unloadAllPlugins();

    /**
     * ask config object
     */
    if (config) {
        KConfigGroup cg = KConfigGroup(config, QStringLiteral("Kate Plugins"));

        // disable all plugin if no config, beside the ones marked as default load
        for (auto &pluginInfo : m_pluginList) {
            pluginInfo.load = cg.readEntry(pluginInfo.saveName(), pluginInfo.defaultLoad);
        }
    }

    /**
     * load plugins
     */
    for (auto &pluginInfo : m_pluginList) {
        if (pluginInfo.load) {
            /**
             * load plugin + trigger update of GUI for already existing main windows
             */
            loadPlugin(&pluginInfo);
            enablePluginGUI(&pluginInfo);

            // restore config
            if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *>(pluginInfo.plugin)) {
                KConfigGroup group(config, QStringLiteral("Plugin:%1:").arg(pluginInfo.saveName()));
                interface->readSessionConfig(group);
            }
        }
    }
}

void KatePluginManager::writeConfig(KConfig *config)
{
    Q_ASSERT(config);

    KConfigGroup cg = KConfigGroup(config, QStringLiteral("Kate Plugins"));
    for (const KatePluginInfo &plugin : std::as_const(m_pluginList)) {
        QString saveName = plugin.saveName();

        cg.writeEntry(saveName, plugin.load);

        // save config
        if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *>(plugin.plugin)) {
            KConfigGroup group(config, QStringLiteral("Plugin:%1:").arg(saveName));
            interface->writeSessionConfig(group);
        }
    }
}

void KatePluginManager::unloadAllPlugins()
{
    for (auto &pluginInfo : m_pluginList) {
        if (pluginInfo.plugin) {
            unloadPlugin(&pluginInfo);
        }
    }
}

void KatePluginManager::enableAllPluginsGUI(KateMainWindow *win, KConfigBase *config)
{
    QElapsedTimer t;
    t.start();
    for (auto &pluginInfo : m_pluginList) {
        if (pluginInfo.plugin) {
            t.restart();
            enablePluginGUI(&pluginInfo, win, config);
            qCDebug(LibKateTime, "-> %s load in %lld ms", qPrintable(pluginInfo.saveName()), t.elapsed());
        }
    }
}

void KatePluginManager::disableAllPluginsGUI(KateMainWindow *win)
{
    for (auto &pluginInfo : m_pluginList) {
        if (pluginInfo.plugin) {
            disablePluginGUI(&pluginInfo, win);
        }
    }
}

bool KatePluginManager::loadPlugin(KatePluginInfo *item)
{
    /**
     * try to load the plugin
     */
    item->plugin = KPluginFactory::instantiatePlugin<KTextEditor::Plugin>(item->metaData, KateApp::self(), QVariantList() << item->saveName()).plugin;
    item->load = item->plugin != nullptr;

    /**
     * tell the world about the success
     */
    if (item->plugin) {
        Q_EMIT KateApp::self()->wrapper()->pluginCreated(item->saveName(), item->plugin);
    }

    return item->plugin != nullptr;
}

void KatePluginManager::unloadPlugin(KatePluginInfo *item)
{
    disablePluginGUI(item);
    delete item->plugin;
    KTextEditor::Plugin *plugin = item->plugin;
    item->plugin = nullptr;
    item->load = false;
    Q_EMIT KateApp::self()->wrapper()->pluginDeleted(item->saveName(), plugin);
}

void KatePluginManager::enablePluginGUI(KatePluginInfo *item, KateMainWindow *win, KConfigBase *config)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // lookup if there is already a view for it..
    QObject *createdView = nullptr;
    if (!win->pluginViews().contains(item->plugin)) {
        // create the view + try to correctly load shortcuts, if it's a GUI Client
        createdView = item->plugin->createView(win->wrapper());
        if (createdView) {
            win->pluginViews().insert(item->plugin, createdView);
        }
    }

    // load session config if needed
    if (config && win->pluginViews().contains(item->plugin)) {
        if (auto interface = qobject_cast<KTextEditor::SessionConfigInterface *>(win->pluginViews().value(item->plugin))) {
            KConfigGroup group(config, QStringLiteral("Plugin:%1:MainWindow:0").arg(item->saveName()));
            interface->readSessionConfig(group);
        }
    }

    if (createdView) {
        Q_EMIT win->wrapper()->pluginViewCreated(item->saveName(), createdView);
    }
}

void KatePluginManager::enablePluginGUI(KatePluginInfo *item)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // enable the gui for all mainwindows...
    for (int i = 0; i < KateApp::self()->mainWindowsCount(); i++) {
        enablePluginGUI(item, KateApp::self()->mainWindow(i), nullptr);
    }
}

void KatePluginManager::disablePluginGUI(KatePluginInfo *item, KateMainWindow *win)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // lookup if there is a view for it..
    if (!win->pluginViews().contains(item->plugin)) {
        return;
    }

    // really delete the view of this plugin
    QObject *deletedView = win->pluginViews().value(item->plugin);
    delete deletedView;
    win->pluginViews().remove(item->plugin);
    Q_EMIT win->wrapper()->pluginViewDeleted(item->saveName(), deletedView);
}

void KatePluginManager::disablePluginGUI(KatePluginInfo *item)
{
    // plugin around at all?
    if (!item->plugin) {
        return;
    }

    // disable the gui for all mainwindows...
    for (int i = 0; i < KateApp::self()->mainWindowsCount(); i++) {
        disablePluginGUI(item, KateApp::self()->mainWindow(i));
    }
}

KTextEditor::Plugin *KatePluginManager::plugin(const QString &name)
{
    const auto it = std::find_if(m_pluginList.cbegin(), m_pluginList.cend(), [name](const KatePluginInfo &pi) {
        return pi.saveName() == name;
    });
    return (it == m_pluginList.cend()) ? nullptr : it->plugin;
}
