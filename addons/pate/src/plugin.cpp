// This file is part of Pate, Kate' Python scripting plugin.
//
// Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2012, 2013 Shaheed Haque <srhaque@theiet.org>
// Copyright (C) 2013 Alex Turbov <i.zaufi@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) version 3, or any
// later version accepted by the membership of KDE e.V. (or its
// successor approved by the membership of KDE e.V.), which shall
// act as a proxy defined in Section 6 of version 3 of the license.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library.  If not, see <http://www.gnu.org/licenses/>.
//

#include "plugin.h"
#include "engine.h"
#include "utilities.h"

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KAction>
#include <KActionCollection>
#include <KConfigBase>
#include <KConfigGroup>
#include <KDialog>
#include <KGenericFactory>
#include <KLocale>
#include <KPassivePopup>
#include <KTextEdit>

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

namespace {
const KAboutData& getAboutData()
{
    static KAboutData about = KAboutData(
        "katepateplugin"
      , "pate"
      , ki18n("Pâté host for Python plugins")
      , "2.0"
      , ki18n("Python interpreter settings")
      , KAboutData::License_LGPL_V3
      );
    return about;
}
}                                                           // anonymous namespace

K_PLUGIN_FACTORY(PatePluginFactory, registerPlugin<Pate::Plugin>();)
K_EXPORT_PLUGIN(PatePluginFactory(getAboutData()))

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
        qDebug() << "Reading session config from:" << getSessionPrivateStorageFilename(config);
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
        qDebug() << "Writing session config to:" << getSessionPrivateStorageFilename(config);
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
    else
    {
        // Check if some modules are not available and show warning
        unsigned broken_modules_count = 0;
        Q_FOREACH(const Engine::PluginState& plugin, m_engine.plugins())
            broken_modules_count += unsigned(plugin.isEnabled() && plugin.isBroken());

        if (broken_modules_count)
            KPassivePopup::message(
                i18nc("@title:window", "Warning")
              , i18ncp(
                    "@info:tooltip %1 is a number of failed plugins"
                  , "%1 plugin module couldn't be loaded. Check the Python plugins config page for details."
                  , "%1 plugin modules couldn't be loaded. Check the Python plugins config page for details."
                  , broken_modules_count
                  )
              , static_cast<QWidget*>(0)
              );
    }
    return true;
}

void Pate::Plugin::reloadModuleConfigPages() const
{
    // Count the number of plugins which need their own custom page.
    m_moduleConfigPages.clear();

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

KTextEditor::ConfigPage* Pate::Plugin::configPage(
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
    KTextEditor::ConfigPage* r = reinterpret_cast<KTextEditor::ConfigPage*>(py.objectUnwrap(result));

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

void Pate::Plugin::setFailureReason(QString reason)
{
    m_engineFailureReason.swap(reason);
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
    about->setText(i18n("About Pate"));
    about->setIcon(KIcon("python"));
    connect(about, SIGNAL(triggered(bool)), this, SLOT(aboutPate()));

    // Try to import the `kate` module
    Python py = Python();
    PyObject* katePackage = py.moduleImport("kate");
    if (katePackage)
    {
        // Ok, load others...
        plugin->engine().tryLoadEnabledPlugins();
        py.functionCall("_pateLoaded");
    }
    else
    {
        m_plugin->setFailureReason(xi18nc("@info:tooltip ", "Cannot load <icode>kate</icode> module"));
        m_plugin->engine().setBroken();
    }
    m_plugin->checkEngineShowPopup();
    // Inject collected actions into GUI
    mainWindow()->guiFactory()->addClient(this);
}
//END Pate::PluginView

Pate::PluginView::~PluginView()
{
    mainWindow()->guiFactory()->removeClient(this);
}

void Pate::PluginView::aboutPate()
{
    KAboutData about = getAboutData();
    // Set other text to show some info about Python used
    // NOTE Separate scope around Python() instance
    QStringList pythonPaths;
    Python py = Python();
    if (PyObject* sysPath = py.itemString("path", "sys"))
    {
        Py_ssize_t len = PyList_Size(sysPath);
        for (Py_ssize_t i = 0; i < len; i++)
        {
            PyObject* path = PyList_GetItem(sysPath, i);
            pythonPaths += Python::unicode(path);
        }
    }
    /// \todo Show info about loaded modules? Problems?

    /// \attention It seems about dialog is not customizable much...
    /// Particularly it would be nice to add a custom tab...
    /// So \b dirty hack is here...
    about.setOtherText(ki18nc("Python variables, no translation needed",
                                "sys.version = %1<br/>sys.path = %2").
                        subs(PY_VERSION).subs(pythonPaths.join(",\n&nbsp;&nbsp;&nbsp;&nbsp;")));

    /// \todo Add logo, authors and everything...
    about.setProgramIconName("python");
    about.addAuthor(ki18n("Paul Giannaros"), ki18n("Out-of-tree original"), "paul@giannaros.org");
    about.addAuthor(ki18n("Shaheed Haque"), ki18n("Rewritten and brought in-tree, V1.0"), "srhaque@theiet.org");
    about.addAuthor(ki18n("Alex Turbov"), ki18n("Streamlined and updated, V2.0"), "i.zaufi@gmail.com");
    KAboutApplicationDialog ad(&about, KAboutApplicationDialog::HideKdeVersion);
    ad.exec();
}

//
// Plugin configuration view.
//

//BEGIN Pate::ConfigPage
Pate::ConfigPage::ConfigPage(QWidget* const parent, Plugin* const plugin)
  : KTextEditor::ConfigPage(parent)
  , m_plugin(plugin)
{
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
  : KTextEditor::ConfigPage(parent)
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
