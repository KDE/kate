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

/// \note Namespace name written in uppercase intentionally!
/// It will appear in debug output from Python plugins...
namespace PATE {
PyObject* debug(PyObject* /*self*/, PyObject* args)
{
    const char* text;

    if (PyArg_ParseTuple(args, "s", &text))
        kDebug() << text;
    Py_INCREF(Py_None);
    return Py_None;
}
}                                                           // namespace PATE

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
  , {
        "kDebug"
      , &PATE::debug
      , METH_VARARGS
      , "True KDE way to show debug info"
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
{
}

Pate::Engine::~Engine()
{
    kDebug() << "Going to destroy the Python engine";

    // Notify Python that engine going to die
    {
        Python py = Python();
        py.functionCall("_pateUnloading");
    }
    unloadAllModules();

    // Clean internal configuration dicts
    // NOTE Do not need to save anything! It's already done!
    if (m_configuration)
        Py_DECREF(m_configuration);
    if (m_sessionConfiguration)
        Py_DECREF(m_sessionConfiguration);

    Python::libraryUnload();
    s_engine_instance = 0;
}

void Pate::Engine::unloadAllModules()
{
    // Unload all modules
    for (int i = 0; i < m_plugins.size(); ++i)
        if (m_plugins[i].isEnabled() && !m_plugins[i].isBroken())
            unloadModule(i);
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
        return i18nc("@info:tooltip ", "Cannot update Python paths");

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

    // Initialize our built-in module.
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

    // Initialize 'plugins' dict of module 'pate'
    PyObject* plugins = PyDict_New();
    py.itemStringSet("plugins", plugins);

    // Load the kate module, but find it first, and verify it loads.
    PyObject* katePackage = py.moduleImport("kate");
    if (!katePackage)
        return i18nc("@info:tooltip ", "Cannot load <icode>kate</icode> module");

    // Get plugins available
    scanPlugins();
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
                const bool checked = m_plugins[index.row()].isEnabled()
                  && !m_plugins[index.row()].isBroken()
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
    if (!m_plugins[index.row()].isBroken())
        result |= Qt::ItemIsEnabled;
    return static_cast<Qt::ItemFlag>(result);
}

bool Pate::Engine::setData(const QModelIndex& index, const QVariant& value, const int role)
{
    assert("Sanity check" && index.row() < m_plugins.size());

    if (role == Qt::CheckStateRole)
    {
        Q_ASSERT("Sanity check" && !m_plugins[index.row()].isBroken());

        const bool enabled = value.toBool();
        m_plugins[index.row()].m_enabled = enabled;
        if (enabled)
            loadModule(index.row());
        else
            unloadModule(index.row());
    }
    return true;
}

QStringList Pate::Engine::enabledPlugins() const
{
    /// \todo \c std::transform + lambda or even better to use
    /// filtered and transformed view from boost
    QStringList result;
    Q_FOREACH(const PluginState& plugin, m_plugins)
        if (plugin.isEnabled())
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

void Pate::Engine::scanPlugins()
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
        plugin.m_enabled = false;

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
}

void Pate::Engine::setEnabledPlugins(const QStringList& enabled_plugins)
{
    for (int i = 0; i < m_plugins.size(); ++i)
        m_plugins[i].m_enabled = enabled_plugins.indexOf(m_plugins[i].m_service->name()) != -1;
}

void Pate::Engine::tryLoadEnabledPlugins()
{
    for (int i = 0; i < m_plugins.size(); ++i)
        if (m_plugins[i].isEnabled() && ! m_plugins[i].isBroken())
            loadModule(i);
}

void Pate::Engine::loadModule(const int idx)
{
    Q_ASSERT("Plugin index is out of range!" && 0 <= idx && idx < m_plugins.size());
    PluginState& plugin = m_plugins[idx];
    Q_ASSERT(
        "Why to call loadModule() for disabled/broken plugin?"
      && plugin.isEnabled()
      && !plugin.isBroken()
      );

    QString module_name = plugin.pythonModuleName();
    kDebug() << "Loading module: " << module_name;

    Python py = Python();

    // Get 'plugins' key from 'pate' module dictionary.
    // Every entry has a module name as a key and 2 elements tuple as a value
    PyObject* plugins = py.itemString("plugins");
    Q_ASSERT("Smth damn wrong!" && plugins);

    PyObject* module = py.moduleImport(PQ(module_name));
    if (module)
    {
        // Move just loaded module to the dict
        const int r2 = PyDict_SetItemString(plugins, PQ(module_name), module);
        Q_ASSERT("Sanity check" && r2 == 0);
        /// \todo Handle error
        Py_DECREF(module);

        // Initialize the module from Python's side
        PyObject* const args = Py_BuildValue("(s)", PQ(module_name));
        py.functionCall("_pluginLoaded", Python::PATE_ENGINE, args);
        Py_DECREF(args);
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

void Pate::Engine::unloadModule(int idx)
{
    Q_ASSERT("Plugin index is out of range!" && 0 <= idx && idx < m_plugins.size());
    PluginState& plugin = m_plugins[idx];
    Q_ASSERT("Why to call unloadModule() for broken plugin?" && !plugin.isBroken());

    kDebug() << "Unloading module: " << plugin.pythonModuleName();

    Python py = Python();

    // Get 'plugins' key from 'pate' module dictionary
    PyObject* plugins = py.itemString("plugins");
    Q_ASSERT("Smth damn wrong! Code review needed!" && plugins);

    PyObject* const args = Py_BuildValue("(s)", PQ(plugin.pythonModuleName()));
    py.functionCall("_pluginUnloading", Python::PATE_ENGINE, args);
    Py_DECREF(args);

    PyDict_DelItemString(plugins, PQ(plugin.pythonModuleName()));
}

// kate: space-indent on; indent-width 4;
#undef PATE_INIT
