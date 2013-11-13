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

#include "engine.h"

// config.h defines PATE_PYTHON_LIBRARY, the path to libpython.so
// on the build system
#include "config.h"
#include "utilities.h"

#include <Python.h>

#include <QLibrary>
#include <QFileInfo>

#include <KConfig>
#include <KDebug>
#include <KGlobal>
#include <KLocalizedString>
#include <KServiceTypeTrader>
#include <KStandardDirs>

#include <kate/application.h>

/// Name of the file where per-plugin configuration is stored.
#define CONFIG_FILE "katepaterc"

#if PY_MAJOR_VERSION < 3
# define PATE_INIT initpate
#else
# define PATE_INIT PyInit_pate
#endif

PyMODINIT_FUNC PATE_INIT();                                 // fwd decl

namespace {
PyObject* s_pate;
/**
 * \attention Kate has embedded Python, so init function \b never will be called
 * automatically! We can use this fact to initialize a pointer to an instance
 * of the \c Engine class (which is a part of the \c Plugin), so exported
 * functions will know it (yep, from Python's side they should be static).
 */
Pate::Engine* s_engine_instance = 0;

/**
 * Wrapper function, called exolicitly from \c Engine::Engine
 * to initialize pointer to the only (by design) instance of the engine,
 * so exported (to Python) functions get know it... Then invoke
 * a real initialization sequence...
 */
void pythonInitwrapper(Pate::Engine* const engine)
{
    assert("Sanity check" && !s_engine_instance);
    s_engine_instance = engine;
    // Call initialize explicitly to initialize embedded interpreter.
    PATE_INIT();
}

/**
 * Functions for the Python module called pate.
 * \todo Does it \b REALLY needed? Configuration data will be flushed
 * on exit anyway! Why to write it (and even allow to plugins to call this)
 * \b before kate really going to exit? It would be better to \b deprecate
 * this (harmful) function!
 */
PyObject* pateSaveConfiguration(PyObject* /*self*/, PyObject* /*unused*/)
{
    if (s_engine_instance)
        s_engine_instance->saveGlobalPluginsConfiguration();
    Py_INCREF(Py_None);
    return Py_None;
}

PyMethodDef pateMethods[] =
{
    {
        "saveConfiguration"
      , &pateSaveConfiguration
      , METH_NOARGS
      , "Save the configuration of the plugin into " CONFIG_FILE
    }
  , { 0, 0, 0, 0 }
};

}                                                           // anonymous namespace

//BEGIN Python module registration
PyMODINIT_FUNC PATE_INIT()
{
#if PY_MAJOR_VERSION < 3
    s_pate = Py_InitModule3("pate", pateMethods, "The pate module");
    PyModule_AddStringConstant(s_pate, "__file__", __FILE__);
#else
    static struct PyModuleDef moduledef =
    {
        PyModuleDef_HEAD_INIT
      , "pate"
      , "The pate module"
      , -1
      , pateMethods
      , 0
      , 0
      , 0
      , 0
    };
    s_pate = PyModule_Create(&moduledef);
    PyModule_AddStringConstant(s_pate, "__file__", __FILE__);
    return s_pate;
#endif
}
//END Python module registration


//BEGIN Pate::Engine::PluginState
Pate::Engine::PluginState::PluginState()
  : m_enabled(false)
  , m_broken(false)
  , m_isDir(false)
{
}
//END Pate::Engine::PluginState


/**
 * Just initialize some members. The second (most important) part
 * is to call \c Engine::tryInitializeGetFailureReason()!
 * W/o that call instance is invalid and using it lead to UB!
 */
Pate::Engine::Engine()
  : m_configuration(0)
  , m_sessionConfiguration(0)
  , m_engineIsUsable(false)
  , m_pluginsLoaded(false)
{
}

Pate::Engine::~Engine()
{
    kDebug() << "Destroy the Python engine";
    if (m_configuration)
    {
        /// \todo Do we \b really need to write smth??
        /// Isn't it already written??
        saveGlobalPluginsConfiguration();
        Py_DECREF(m_configuration);
    }
    if (m_sessionConfiguration)
        Py_DECREF(m_sessionConfiguration);

    Python::libraryUnload();
    s_engine_instance = 0;
}

/**
 * \todo Make sure noone tries to use uninitialized engine!
 * (Or enable exceptions for this module, so this case wouldn't even araise?)
 */
QString Pate::Engine::tryInitializeGetFailureReason()
{
    kDebug() << "Construct the Python engine for Python" << PY_MAJOR_VERSION << PY_MINOR_VERSION;
    if (0 != PyImport_AppendInittab(Python::PATE_ENGINE, PATE_INIT))
        return i18nc("@info:tooltip ", "Cannot load built-in <icode>pate</icode> module");

    Python::libraryLoad();
    Python py = Python();

    // Update PYTHONPATH
    // 0) custom plugin directories (prefer local dir over systems')
    // 1) shipped kate module's dir
    // 2) w/ site_packages/ dir of the Python
    QStringList pluginDirectories = KGlobal::dirs()->findDirs("appdata", "pate/");
    pluginDirectories
      << KStandardDirs::locate("appdata", "plugins/pate/")
      << QLatin1String(PATE_PYTHON_SITE_PACKAGES_INSTALL_DIR)
      ;
    kDebug() << "Plugin Directories: " << pluginDirectories;
    if (!py.prependPythonPaths(pluginDirectories))
        return i18nc("@info:tooltip ", "Can not update Python paths");

    // Show some SPAM
    /// \todo Add <em>"About Python"</em> to Help menu or <em>System Info</em> tab
    /// to the plugin configuration, so users (and plugin authors) can get a path
    /// list (and probably other (interestring) system details) w/o reading a lot
    /// of SPAM from terminal or \c ~/.xsession_errors file.
    if (PyObject* sysPath = py.itemString("path", "sys"))
    {
        Py_ssize_t len = PyList_Size(sysPath);
        for (Py_ssize_t i = 0; i < len; i++)
        {
            PyObject* path = PyList_GetItem(sysPath, i);
            kDebug() << "sys.path" << i << Python::unicode(path);
        }
    }

    PyRun_SimpleString(
        "import sip\n"
        "sip.setapi('QDate', 2)\n"
        "sip.setapi('QTime', 2)\n"
        "sip.setapi('QDateTime', 2)\n"
        "sip.setapi('QUrl', 2)\n"
        "sip.setapi('QTextStream', 2)\n"
        "sip.setapi('QString', 2)\n"
        "sip.setapi('QVariant', 2)\n"
    );

    // Initialise our built-in module.
    pythonInitwrapper(this);
    if (!s_pate)
        return i18nc("@info:tooltip ", "No <icode>pate</icode> built-in module");

    // Setup global configuration
    m_configuration = PyDict_New();
    /// \todo Check \c m_configuration ?
    // Host the configuration dictionary.
    py.itemStringSet("configuration", m_configuration);

    // Setup per session configuration
    m_sessionConfiguration = PyDict_New();
    py.itemStringSet("sessionConfiguration", m_sessionConfiguration);

    // Load the kate module, but find it first, and verify it loads.
    PyObject* katePackage = py.moduleImport("kate");
    if (!katePackage)
        return i18nc("@info:tooltip ", "Cannot load <icode>kate</icode> module");

    // NOTE Empty failure reson string indicates success!
    m_engineIsUsable = true;
    return QString();
}

int Pate::Engine::columnCount(const QModelIndex&) const
{
    return Column::LAST__;
}

int Pate::Engine::rowCount(const QModelIndex&) const
{
    return m_plugins.size();
}

QModelIndex Pate::Engine::index(const int row, const int column, const QModelIndex& parent) const
{
    if (!parent.isValid() && row < m_plugins.size() && column < Column::LAST__)
        return createIndex(row, column, 0);
    return QModelIndex();
}

QModelIndex Pate::Engine::parent(const QModelIndex&) const
{
    return QModelIndex();
}

QVariant Pate::Engine::headerData(
    const int section
  , const Qt::Orientation orientation
  , const int role
  ) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case Column::NAME:
                return i18nc("@title:column", "Name");
            case Column::COMMENT:
                return i18nc("@title:column", "Comment");
            default:
                break;
        }
    }
    return QVariant();
}

QVariant Pate::Engine::data(const QModelIndex& index, const int role) const
{
    assert("Sanity check" && index.row() < m_plugins.size());
    assert("Sanity check" && index.column() < Column::LAST__);
    switch (role)
    {
        case Qt::DisplayRole:
            switch (index.column())
            {
                case Column::NAME:
                    return m_plugins[index.row()].m_service->name();
                case Column::COMMENT:
                    return m_plugins[index.row()].m_service->comment();
                default:
                    break;
            }
            break;
        case Qt::CheckStateRole:
        {
            if (index.column() == Column::NAME)
            {
                const bool checked = m_plugins[index.row()].m_enabled
                  && !m_plugins[index.row()].m_broken
                  ;
                return checked ? Qt::Checked : Qt::Unchecked;
            }
            break;
        }
        case Qt::ToolTipRole:
            if (!m_plugins[index.row()].m_errorReason.isEmpty())
                return m_plugins[index.row()].m_errorReason;
            break;
        default:
            break;
    }
    return QVariant();
}

Qt::ItemFlags Pate::Engine::flags(const QModelIndex& index) const
{
    assert("Sanity check" && index.row() < m_plugins.size());
    assert("Sanity check" && index.column() < Column::LAST__);

    int result = Qt::ItemIsSelectable;
    if (index.column() == Column::NAME)
        result |= Qt::ItemIsUserCheckable;
    // Disable to select/check broken modules
    if (!m_plugins[index.row()].m_broken)
        result |= Qt::ItemIsEnabled;
    return static_cast<Qt::ItemFlag>(result);
}

bool Pate::Engine::setData(const QModelIndex& index, const QVariant& value, const int role)
{
    assert("Sanity check" && index.row() < m_plugins.size());

    if (role == Qt::CheckStateRole)
    {
        kDebug() << "value.toBool()=" << value.toBool();
        m_plugins[index.row()].m_enabled = value.toBool();
    }
    return true;
}

QStringList Pate::Engine::enabledPlugins() const
{
    QStringList result;
    Q_FOREACH(const PluginState& plugin, m_plugins)
        if (plugin.m_enabled)
            result.append(plugin.m_service->name());
    return result;
}

void Pate::Engine::readGlobalPluginsConfiguration()
{
    Python py = Python();
    PyDict_Clear(m_configuration);
    KConfig config(CONFIG_FILE, KConfig::SimpleConfig);
    py.updateDictionaryFromConfiguration(m_configuration, &config);
}

void Pate::Engine::saveGlobalPluginsConfiguration()
{
    Python py = Python();
    KGlobal::config()->sync();
    KConfig config(CONFIG_FILE, KConfig::SimpleConfig);
    py.updateConfigurationFromDictionary(&config, m_configuration);
    config.sync();
}

void Pate::Engine::readSessionPluginsConfiguration(KConfigBase* const config)
{
    PyDict_Clear(m_sessionConfiguration);
    Python().updateDictionaryFromConfiguration(m_sessionConfiguration, config);
}

void Pate::Engine::writeSessionPluginsConfiguration(KConfigBase* const config)
{
    Python().updateConfigurationFromDictionary(config, m_sessionConfiguration);
}

void Pate::Engine::tryLoadEnabledPlugins(const QStringList& enabled_plugins)
{
    m_plugins.clear();                                      // Clear current state.

    // Collect plugins available
    KService::List services;
    KServiceTypeTrader* trader = KServiceTypeTrader::self();

    kDebug() << "Seeking for installed plugins...";
    services = trader->query("Kate/PythonPlugin");
    Q_FOREACH(KService::Ptr service, services)
    {
        kDebug() << "Got Kate/PythonPlugin: " << service->name()
          << ", module-path=" << service->library()
          ;
        // Make sure mandatory properties are here
        if (service->name().isEmpty())
        {
            kDebug() << "Ignore desktop file w/o a name";
            continue;
        }
        if (service->library().isEmpty())
        {
            kDebug() << "Ignore desktop file w/o a module to import";
            continue;
        }

        // Check Python compatibility
        const bool is_compatible = service->property(
#if PY_MAJOR_VERSION < 3
            "X-Python-2-Compatible"
#else
            "X-Python-3-Compatible"
#endif
          , QVariant::Bool
          ).toBool();

        if (!is_compatible)
        {
            kDebug() << service->name() << "is incompatible w/ embedded Python version";
            continue;
        }

        // Make a new state
        PluginState plugin;
        plugin.m_service = service;
        plugin.m_enabled = enabled_plugins.indexOf(service->name()) != -1;

        // Find the module:
        // 0) try to locate directory based plugin first
        KUrl rel_path = QString(Python::PATE_ENGINE);
        rel_path.addPath(plugin.moduleFilePathPart());
        rel_path.addPath("__init__.py");
        QString module_path = KGlobal::dirs()->findResource("appdata", rel_path.toLocalFile());
        if (module_path.isEmpty())
        {
            // 1) Nothing found, then try file based plugin
            rel_path = QString(Python::PATE_ENGINE);
            rel_path.addPath(plugin.moduleFilePathPart() + ".py");
            module_path = KGlobal::dirs()->findResource("appdata", rel_path.toLocalFile());
        }
        else plugin.m_isDir = true;

        // Is anything found at all?
        if (module_path.isEmpty())
        {
            plugin.m_broken = true;
            plugin.m_errorReason = i18nc(
                "@info:tooltip"
              , "Unable to find the module specified <application>%1</application>"
              , service->library()
              );
        }
        else kDebug() << "Found module path:" << module_path;

        // Try to check dependencies. To do it
        // just try to import a module... when unload it ;)
        /// \todo Full featured dependencies checker can be implemented
        /// as a Python module (special named or listed as a property in
        /// a \c .desktop file ;-)
        const QStringList dependencies = service->property(
            "X-Python-Dependencies"
          , QVariant::StringList
          ).toStringList();
        Python py = Python();
        Q_FOREACH(const QString& dep, dependencies)
        {
            kDebug() << "Try import dependency module/package:" << dep;
            PyObject* module = py.moduleImport(PQ(dep));
            if (module)
                Py_DECREF(module);
            else
            {
                plugin.m_broken = true;
                plugin.m_errorReason = i18nc(
                    "@info:tooltip"
                  , "Failure on checking dependency <application>%1</application>:<nl/>%2"
                  , dep
                  , py.lastTraceback()
                  );
            }
        }

        m_plugins.append(plugin);
    }

    reloadEnabledPlugins();
}

void Pate::Engine::reloadEnabledPlugins()
{
    unloadModules();
    loadModules();
}

/**
 * Walk over the model, loading all usable plugins into a PyObject module
 * dictionary.
 */
void Pate::Engine::loadModules()
{
    if (m_pluginsLoaded)
        return;

    kDebug() << "Loading enabled python modules";

    // Add two lists to the module: pluginDirectories and plugins.
    Python py = Python();
    PyObject* pluginDirectories = PyList_New(0);
    Py_INCREF(pluginDirectories);
    py.itemStringSet("pluginDirectories", pluginDirectories);
    PyObject* plugins = PyList_New(0);
    Py_INCREF(plugins);
    py.itemStringSet("plugins", plugins);

    for (
        QList<PluginState>::iterator it = m_plugins.begin()
      , last = m_plugins.end()
      ; it != last
      ; ++it
      )
    {
        PluginState& plugin = *it;
        QString module_name = plugin.pythonModuleName();
        kDebug() << "Loading module: " << module_name;

        // Were we asked to load this plugin?
        if (plugin.m_enabled)
        {
            // Import and add to pate.plugins
            PyObject* module = py.moduleImport(PQ(module_name));
            if (module)
            {
                PyList_Append(plugins, module);
                Py_DECREF(module);
            }
            else
            {
                plugin.m_broken = true;
                plugin.m_errorReason = i18nc(
                    "@info:tooltip"
                  , "Module not loaded:<nl/>%1"
                  , py.lastTraceback()
                  );
            }
        }
    }

    m_pluginsLoaded = true;

    // everything is loaded and started. Call the module's init callback
    py.functionCall("_pluginsLoaded");
}

void Pate::Engine::unloadModules()
{
    // We don't have the luxury of being able to unload Python easily while
    // Kate is running. If anyone can find a way, feel free to tell me and
    // I'll patch it in. Calling Py_Finalize crashes.
    // So, clean up the best that we can.
    if (!m_pluginsLoaded)
        return;

    kDebug() << "Unloading python modules";

    // Remove each plugin from sys.modules
    Python py = Python();
    PyObject* modules = PyImport_GetModuleDict();
    PyObject* plugins = py.itemString("plugins");
    if (plugins)
    {
        for (Py_ssize_t i = 0, j = PyList_Size(plugins); i < j; ++i)
        {
            PyObject* pluginName = py.itemString("__name__", PyModule_GetDict(PyList_GetItem(plugins, i)));
            if (pluginName && PyDict_Contains(modules, pluginName))
            {
                PyDict_DelItem(modules, pluginName);
                kDebug() << "Deleted" << Python::unicode(pluginName) << "from sys.modules";
            }
        }
        py.itemStringDel("plugins");
        Py_DECREF(plugins);
    }
    m_pluginsLoaded = false;
    py.functionCall("_pluginsUnloaded");
}

// kate: space-indent on; indent-width 4;
#undef PATE_INIT
