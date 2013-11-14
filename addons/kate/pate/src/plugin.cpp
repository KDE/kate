// This file is part of Pate, Kate' Python scripting plugin.
//
// Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2012 Shaheed Haque <srhaque@theiet.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "plugin.h"
#include "engine.h"
#include "utilities.h"

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <KAboutData>
#include <KAction>
#include <KActionCollection>
#include <KDialog>
#include <KLocale>
#include <KGenericFactory>
#include <KConfigBase>
#include <KConfigGroup>
#include <KTextEdit>
#include <KPassivePopup>

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QTreeView>
#include <QVBoxLayout>

#define CONFIG_SECTION "global"

//
// The Pate plugin
//

K_PLUGIN_FACTORY(PatePluginFactory, registerPlugin<Pate::Plugin>();)
K_EXPORT_PLUGIN(
    PatePluginFactory(
        KAboutData(
            "katepateplugin"
          , "katepateplugin"
          , ki18n("Pâté Plugin")
          , "1.0"
          , ki18n("Pâté host for Python plugins")
          , KAboutData::License_LGPL_V3
          )
      )
  )

//BEGIN Pate::Plugin
Pate::Plugin::Plugin(QObject* const app, const QList<QVariant>&)
  : Kate::Plugin(static_cast<Kate::Application*>(app), "katepateplugin")
  , m_engineFailureReason(m_engine.tryInitializeGetFailureReason())
  , m_autoReload(false)
{
    // NOTE It is useless to show any popup here (in case of engine failure)...
    // Need to wait untill some document's view changed...
}

Pate::Plugin::~Plugin()
{
    m_moduleConfigPages.clear();
}

Kate::PluginView* Pate::Plugin::createView(Kate::MainWindow* window)
{
    return new Pate::PluginView(window, this);
}

void Pate::Plugin::readSessionConfig(KConfigBase* const config, const QString& groupPrefix)
{
    KConfigGroup group = config->group(groupPrefix + CONFIG_SECTION);
    m_autoReload = group.readEntry("AutoReload", false);
    if (m_engine)
    {
        m_engine.readGlobalPluginsConfiguration();
        kDebug() << "Reading session config from:" << getSessionPrivateStorageFilename(config);
        KConfig session_config(getSessionPrivateStorageFilename(config), KConfig::SimpleConfig);
        m_engine.readSessionPluginsConfiguration(&session_config);
        m_engine.setEnabledPlugins(group.readEntry("Enabled Plugins", QStringList()));
    }
}

void Pate::Plugin::writeSessionConfig(KConfigBase* const config, const QString& groupPrefix)
{
    KConfigGroup group = config->group(groupPrefix + CONFIG_SECTION);
    group.writeEntry("AutoReload", m_autoReload);
    if (m_engine)
    {
        group.writeEntry("Enabled Plugins", m_engine.enabledPlugins());
        kDebug() << "Writing session config to:" << getSessionPrivateStorageFilename(config);
        m_engine.saveGlobalPluginsConfiguration();
        KConfig session_config(getSessionPrivateStorageFilename(config), KConfig::SimpleConfig);
        m_engine.writeSessionPluginsConfiguration(&session_config);
        session_config.sync();
    }
    group.sync();
}

uint Pate::Plugin::configPages() const
{
    const uint pages = 1;                                   // The Manager page is always present.
    reloadModuleConfigPages();
    return pages + m_moduleConfigPages.size();
}

/**
 * The only purpose of this method is to reuse the same popup
 * appeared (over and over) in case of broken engine...
 */
bool Pate::Plugin::checkEngineShowPopup() const
{
    if (!m_engine)
    {
        KPassivePopup::message(
            i18nc("@title:window", "Pate engine could not be initialised")
          , m_engineFailureReason
          , static_cast<QWidget*>(0)
          );
        return false;
    }
    return true;
}

void Pate::Plugin::reloadModuleConfigPages() const
{
    // Count the number of plugins which need their own custom page.
    m_moduleConfigPages.clear();

    if (!checkEngineShowPopup())
        return;

    Python py = Python();
    Q_FOREACH(const Engine::PluginState& plugin, m_engine.plugins())
    {
        // Do not even try to load not enabled or broken plugins!
        if (!plugin.isEnabled() || plugin.isBroken())
            continue;

        PyObject* configPages = py.moduleConfigPages(PQ(plugin.pythonModuleName()));
        if (configPages)
        {
            for (Py_ssize_t k = 0, l = PyList_Size(configPages); k < l; ++k)
            {
                // Add an action for this plugin.
                PyObject* tuple = PyList_GetItem(configPages, k);
                m_moduleConfigPages.append(tuple);
            }
        }
    }
}

Kate::PluginConfigPage* Pate::Plugin::configPage(
    uint number
  , QWidget* const parent
  , const char* const name
  )
{
    Q_UNUSED(name);

    if (!number)
        return new Pate::ConfigPage(parent, this);

    if (number > uint(m_moduleConfigPages.size()))
        return 0;

    number--;
    Python py = Python();
    PyObject* tuple = m_moduleConfigPages.at(number);
    PyObject* func = PyTuple_GetItem(tuple, 1);
    PyObject* w = py.objectWrap(parent, "PyQt4.QtGui.QWidget");
    PyObject* arguments = Py_BuildValue("(Oz)", w, name);
    Py_DECREF(w);
    Py_INCREF(func);
    PyObject* result = PyObject_CallObject(func, arguments);
    Py_DECREF(arguments);
    if (!result)
    {
        // Return a page descrbing the error rather than crashing.
        py.traceback("failed to call plugin page");
        return new Pate::ErrorConfigPage(parent, py.lastTraceback());
    }
    Kate::PluginConfigPage* r = reinterpret_cast<Kate::PluginConfigPage*>(py.objectUnwrap(result));

    /// \todo We leak this here reference.
    //Py_DECREF(result);
    return r;
}

QString Pate::Plugin::configPageName(const uint number) const
{
    if (!number)
        return i18nc("@title:row", "Python Plugins");

    if (number > uint(m_moduleConfigPages.size()))
        return QString();

    Python py = Python();
    PyObject* tuple = m_moduleConfigPages.at(number - 1);
    PyObject* configPage = PyTuple_GetItem(tuple, 2);
    PyObject* name = PyTuple_GetItem(configPage, 0);
    return Python::unicode(name);
}

QString Pate::Plugin::configPageFullName(const uint number) const
{
    if (!number)
        return i18nc("@title:tab", "Pâté host for Python plugins");

    if (number > uint(m_moduleConfigPages.size()))
        return QString();

    Python py = Python();
    PyObject* tuple = m_moduleConfigPages.at(number - 1);
    PyObject* configPage = PyTuple_GetItem(tuple, 2);
    PyObject* fullName = PyTuple_GetItem(configPage, 1);
    return Python::unicode(fullName);
}

KIcon Pate::Plugin::configPageIcon(const uint number) const
{
    if (!number)
        return KIcon("preferences-plugin");

    if (number > (uint)m_moduleConfigPages.size())
        return KIcon();

    Python py = Python();
    PyObject* tuple = m_moduleConfigPages.at(number - 1);
    PyObject* configPage = PyTuple_GetItem(tuple, 2);
    PyObject* icon = PyTuple_GetItem(configPage, 2);
    return *reinterpret_cast<KIcon*>(py.objectUnwrap(icon));
}

inline const Pate::Engine& Pate::Plugin::engine() const
{
    return m_engine;
}

inline Pate::Engine& Pate::Plugin::engine()
{
    return m_engine;
}

QString Pate::Plugin::getSessionPrivateStorageFilename(KConfigBase* const config)
{
    KConfig* real_config = dynamic_cast<KConfig*>(config);
    Q_ASSERT("WOW! KDE API now uses smth else than KConfig?" && real_config);
    /// \note In case of new or default session, a "global" config file
    /// will be used, so switch to a separate (private) file then...
    if (real_config->name() == "katerc")
        return "katepaterc";
    return real_config->name().replace(".katesession", ".katepate");
}

//
// Plugin view, instances of which are created once for each session.
//
//BEGIN Pate::PluginView
Pate::PluginView::PluginView(Kate::MainWindow* const window, Plugin* const plugin)
  : Kate::PluginView(window)
  , Kate::XMLGUIClient(PatePluginFactory::componentData())
  , m_plugin(plugin)
{
    KAction* about = actionCollection()->addAction("about_pate");
    about->setText("About Pate");
    //
    plugin->engine().tryLoadEnabledPlugins();
    Python py = Python();
    py.functionCall("_pateLoaded");
    //
    mainWindow()->guiFactory()->addClient(this);
}
//END Pate::PluginView

Pate::PluginView::~PluginView()
{
    mainWindow()->guiFactory()->removeClient(this);
}

//
// Plugin configuration view.
//

//BEGIN Pate::ConfigPage
Pate::ConfigPage::ConfigPage(QWidget* const parent, Plugin* const plugin)
  : Kate::PluginConfigPage(parent)
  , m_plugin(plugin)
{
    kDebug() << "create ConfigPage";

    if (!m_plugin->checkEngineShowPopup())
    {
        /// \todo HIDE THE MANAGER AND SHOW ERROR INSTEAD!
        //return;
    }

    // Create a page with just the main manager tab.
    m_manager.setupUi(this);
    QSortFilterProxyModel* const proxy_model = new QSortFilterProxyModel(this);
    proxy_model->setSourceModel(&m_plugin->engine());
    m_manager.pluginsList->setModel(proxy_model);
    m_manager.pluginsList->resizeColumnToContents(0);
    m_manager.pluginsList->sortByColumn(0, Qt::AscendingOrder);
    reset();

    const bool is_enabled = bool(m_plugin->engine());
    const bool is_visible = !is_enabled;
    m_manager.errorLabel->setVisible(is_visible);
    m_manager.pluginsList->setEnabled(is_enabled);
}

Pate::ConfigPage::~ConfigPage()
{
}

void Pate::ConfigPage::apply()
{
    // Retrieve the settings from the UI and reflect them in the plugin.
}

void Pate::ConfigPage::reset()
{
    // Retrieve the settings from the plugin and reflect them in the UI.
}

void Pate::ConfigPage::defaults()
{
    // Set the UI to have default settings.
    Q_EMIT(changed());
}
//END Pate::ConfigPage


//BEGIN Pate::ErrorConfigPage
Pate::ErrorConfigPage::ErrorConfigPage(QWidget* const parent, const QString& traceback)
  : Kate::PluginConfigPage(parent)
{
    KTextEdit* const widget = new KTextEdit(parent);
    widget->setText(traceback);
    widget->setReadOnly(true);
    widget->setEnabled(true);
    QLayout* lo = parent->layout();
    lo->addWidget(widget);
}
//END Pate::ErrorConfigPage

// kate: indent-width 4;
