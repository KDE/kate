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

K_EXPORT_COMPONENT_FACTORY(katepateplugin, KGenericFactory<Pate::Plugin>("pate"))

//BEGIN Pate::Plugin
Pate::Plugin::Plugin(QObject* const app, const QStringList&)
  : Kate::Plugin(static_cast<Kate::Application*>(app))
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
    return new Pate::PluginView(window);
    // NOTE It is (still) useless to (try to) show any popup here...
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
        m_engine.tryLoadEnabledPlugins(group.readEntry("Enabled Plugins", QStringList()));

        Python py = Python();
        py.functionCall("_sessionCreated");
    }
    // NOTE It is (still) useless to (try to) show any popup here... ;-)
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
        // For the very first time, when plugin just enabled, there is only
        // an internal list of enabled plugins (and no configuration yet)
        // After "Ok"/"Apply" button gets pressed in configuration dialog,
        // this method will be called, and we must enable everything that was
        // checked... or has changed.
        if (!m_engine.isPluginsLoaded())
        {
            kDebug() << "Going to load enabled plugins";
            m_engine.reloadEnabledPlugins();
        }
        else if (m_engine.isRealoadNeeded())
        {
            /// \attention Reloading plugins at this point leads to UB:
            /// lot of errors in Python code and possible crash on application exit!
            /// (due some problems w/ removing a toolview if present)
            /// So popup is here...
            KPassivePopup::message(
                i18nc("@title:window", "Attention")
              , i18nc("@info", "<application>kate</application> must be restarted")
              , static_cast<QWidget*>(0)
              );
            /// \todo Need to change behaviour as C++ plugins work:
            /// if user enable/disable smth, a plugin must be loaded/unloaded
            /// at that time, cuz it may have configuration pages to be shown.
            /// But, nowadays kate's plugin interface doesn't have a way to
            /// dynamically change pages count... i.e. to indicate that fact
            /// when configuration dialog already shown...
        }
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
    Kate::PluginConfigPage* r = (Kate::PluginConfigPage*)py.objectUnwrap(result);

    /// \todo We leak this here reference.
    //Py_DECREF(result);
    return r;
}

QString Pate::Plugin::configPageName(uint number) const
{
    if (!number)
        return i18nc("@title:row", "Python Plugins");

    if (number > uint(m_moduleConfigPages.size()))
        return QString();

    number--;
    Python py = Python();
    PyObject* tuple = m_moduleConfigPages.at(number);
    PyObject* configPage = PyTuple_GetItem(tuple, 2);
    PyObject* name = PyTuple_GetItem(configPage, 0);
    return Python::unicode(name);
}

QString Pate::Plugin::configPageFullName(uint number) const
{
    if (!number)
        return i18nc("@title:tab", "Pâté host for Python plugins");

    if (number > (uint)m_moduleConfigPages.size())
        return QString();

    number--;
    Python py = Python();
    PyObject* tuple = m_moduleConfigPages.at(number);
    PyObject* configPage = PyTuple_GetItem(tuple, 2);
    PyObject* fullName = PyTuple_GetItem(configPage, 1);
    return Python::unicode(fullName);
}

KIcon Pate::Plugin::configPageIcon(uint number) const
{
    if (!number)
        return KIcon("preferences-plugin");

    if (number > (uint)m_moduleConfigPages.size())
        return KIcon();

    number--;
    Python py = Python();
    PyObject* tuple = m_moduleConfigPages.at(number);
    PyObject* configPage = PyTuple_GetItem(tuple, 2);
    PyObject* icon = PyTuple_GetItem(configPage, 2);
    return *(KIcon*)py.objectUnwrap(icon);
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
Pate::PluginView::PluginView(Kate::MainWindow* const window)
  : Kate::PluginView(window)
{
}
//END Pate::PluginView


//
// Plugin configuration view.
//

//BEGIN Pate::ConfigPage
Pate::ConfigPage::ConfigPage(QWidget* const parent, Plugin* const plugin)
  : Kate::PluginConfigPage(parent)
  , m_plugin(plugin)
  , m_pluginActions(0)
  , m_pluginConfigPages(0)
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
    reset();
    connect(m_manager.reload, SIGNAL(clicked(bool)), SLOT(reloadPage(bool)));

    // Add a tab for reference information.
    QWidget* const infoWidget = new QWidget(m_manager.tabWidget);
    m_info.setupUi(infoWidget);
    m_manager.tabWidget->addTab(infoWidget, i18nc("@title:tab", "Documentation"));
    connect(m_info.topics, SIGNAL(currentIndexChanged(int)), SLOT(infoTopicChanged(int)));
    connect(m_info.actions, SIGNAL(currentIndexChanged(int)), SLOT(infoPluginActionsChanged(int)));
    connect(m_info.configPages, SIGNAL(currentIndexChanged(int)), SLOT(infoPluginConfigPagesChanged(int)));
    reloadPage(true);

    const bool is_enabled = bool(m_plugin->engine());
    const bool is_visible = !is_enabled;
    m_manager.errorLabel->setVisible(is_visible);
    m_manager.tabWidget->setEnabled(is_enabled);
    m_manager.reload->setEnabled(is_enabled);
}

Pate::ConfigPage::~ConfigPage()
{
    Python py = Python();
    Py_XDECREF(m_pluginActions);
    Py_XDECREF(m_pluginConfigPages);
}

void Pate::ConfigPage::reloadPage(const bool init)
{
    if (!init)
    {
        if (m_plugin->engine())
        {
            m_plugin->engine().saveGlobalPluginsConfiguration();
            m_plugin->engine().reloadEnabledPlugins();
        }
        else
        {
            /// \todo Show error
        }
    }
    m_plugin->reloadModuleConfigPages();

    m_manager.pluginsList->resizeColumnToContents(0);
    m_manager.pluginsList->sortByColumn(0, Qt::AscendingOrder);

    // Add a topic for each built-in packages, using stacked page 0. Note that
    // pate itself is not exposed as it is just part of the plumbing required
    // by kate.
    m_info.topics->clear();
    m_info.topics->addItem(KIcon("applications-development"), QLatin1String("kate"));
    m_info.topics->addItem(KIcon("applications-development"), QLatin1String("kate.gui"));

    // Add a topic for each plugin. using stacked page 1.
    Python py = Python();
    PyObject* plugins = py.itemString("plugins");
    if (plugins)
    {
        for (Py_ssize_t i = 0, j = PyList_Size(plugins); i < j; ++i)
        {
            PyObject* module = PyList_GetItem(plugins, i);
            // Add a topic for this plugin, using stacked page 1.
            m_info.topics->addItem(
                KIcon("text-x-python")
              , QLatin1String(PyModule_GetName(module))
              );
        }
    }
    infoTopicChanged(0);
}

void Pate::ConfigPage::infoTopicChanged(const int topicIndex)
{
    Python py = Python();
    if (-1 == topicIndex)
    {
        // We are being reset.
        Py_XDECREF(m_pluginActions);
        m_pluginActions = 0;
        Py_XDECREF(m_pluginConfigPages);
        m_pluginConfigPages = 0;
        return;
    }

    // Display the information for the selected module/plugin.
    const QString topic = m_info.topics->itemText(topicIndex);

    // Reference tab.
    m_info.help->setHtml(py.moduleHelp(PQ(topic)));

    // Action tab.
    m_info.actions->clear();
    Py_XDECREF(m_pluginActions);
    m_pluginActions = py.moduleActions(PQ(topic));
    if (m_pluginActions)
    {
        for (Py_ssize_t i = 0, j = PyList_Size(m_pluginActions); i < j; ++i)
        {
            PyObject* tuple = PyList_GetItem(m_pluginActions, i);
            PyObject* functionName = PyTuple_GetItem(tuple, 0);

            // Add an action for this plugin.
            m_info.actions->addItem(Python::unicode(functionName));
        }
    }
    infoPluginActionsChanged(0);

    // Config pages tab.
    m_info.configPages->clear();
    Py_XDECREF(m_pluginConfigPages);
    m_pluginConfigPages = py.moduleConfigPages(PQ(topic));
    if (m_pluginConfigPages)
    {
        for (Py_ssize_t i = 0, j = PyList_Size(m_pluginConfigPages); i < j; ++i)
        {
            PyObject* tuple = PyList_GetItem(m_pluginConfigPages, i);
            PyObject* functionName = PyTuple_GetItem(tuple, 0);

            // Add a config page for this plugin.
            m_info.configPages->addItem(Python::unicode(functionName));
        }
    }
    infoPluginConfigPagesChanged(0);
}

void Pate::ConfigPage::infoPluginActionsChanged(const int actionIndex)
{
    if (!m_pluginActions)
        // This is a bit weird. ORLY? :-)
        return;

    Python py = Python();
    PyObject* tuple = PyList_GetItem(m_pluginActions, actionIndex);
    if (!tuple)
    {
        // A plugin with no executable actions? It must be a module...
        m_info.text->setText(QString::null);
        m_info.actionIcon->setIcon(QIcon());
        m_info.actionIcon->setText(QString::null);
        m_info.shortcut->setText(QString::null);
        m_info.menu->setText(QString::null);
        m_info.description->setText(QString::null);
        return;
    }

    PyObject* action = PyTuple_GetItem(tuple, 1);
    PyObject* text = PyTuple_GetItem(action, 0);
    PyObject* icon = PyTuple_GetItem(action, 1);
    PyObject* shortcut = PyTuple_GetItem(action, 2);
    PyObject* menu = PyTuple_GetItem(action, 3);
    PyObject* __doc__ = PyTuple_GetItem(tuple, 2);

    // Add a topic for this plugin, using stacked page 0.
    m_info.text->setText(Python::unicode(text));
    if (Py_None == icon)
        m_info.actionIcon->setIcon(QIcon());
    else if (Python::isUnicode(icon))
        m_info.actionIcon->setIcon(KIcon(Python::unicode(icon)));
    else
    {
#if PY_MAJOR_VERSION < 3
        m_info.actionIcon->setIcon(*(QPixmap*)PyCObject_AsVoidPtr(icon));
#else
        m_info.actionIcon->setIcon(*(QPixmap*)PyCapsule_GetPointer(icon, "icon"));
#endif
    }
    m_info.shortcut->setText(Python::unicode(shortcut));
    m_info.menu->setText(Python::unicode(menu));
    m_info.description->setText(Python::unicode(__doc__));
}

void Pate::ConfigPage::infoPluginConfigPagesChanged(const int pageIndex)
{
    if (!m_pluginConfigPages)
        // This is a bit weird.
        return;

    Python py = Python();
    PyObject* tuple = PyList_GetItem(m_pluginConfigPages, pageIndex);
    if (!tuple)
    {
        // No config pages.
        m_info.name->setText(QString::null);
        m_info.fullName->setText(QString::null);
        m_info.configPageIcon->setIcon(QIcon());
        m_info.configPageIcon->setText(QString::null);
        return;
    }

    PyObject* configPage = PyTuple_GetItem(tuple, 2);
    PyObject* name = PyTuple_GetItem(configPage, 0);
    PyObject* fullName = PyTuple_GetItem(configPage, 1);
    PyObject* icon = PyTuple_GetItem(configPage, 2);

    // Add a topic for this plugin, using stacked page 0.
    /// \todo Proper handling of Unicode
    m_info.name->setText(Python::unicode(name));
    m_info.fullName->setText(Python::unicode(fullName));
    if (Py_None == icon)
        m_info.configPageIcon->setIcon(QIcon());
    else if (Python::isUnicode(icon))
        m_info.configPageIcon->setIcon(KIcon(Python::unicode(icon)));
    else
        m_info.configPageIcon->setIcon(*(KIcon*)py.objectUnwrap(icon));
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
