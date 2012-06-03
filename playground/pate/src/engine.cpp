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

#include "Python.h"

#include <QApplication>
#include <QLibrary>
#include <QStack>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QStandardItem>

#include <kglobal.h>
#include <KIcon>
#include <KLocale>
#include <KConfig>
#include <KConfigGroup>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kate/application.h>

// config.h defines PATE_PYTHON_LIBRARY, the path to libpython.so
// on the build system
#include "config.h"

#include "engine.h"
#include "utilities.h"

const char *Pate::Engine::PATE_ENGINE = "pate";

/**
 * Name of the file where per-plugin configuration is stored.
 */
#define CONFIG_FILE "katepaterc"

#define THREADED 0

// We use a QStandardItemModel to store plugin information as follows:
//
//  - invisibleRoot
//      - directory
//          - pluginName
//
// A pluginName has an associated type to describe its loadability.

typedef enum
{
    Hidden = QStandardItem::UserType,
    UsableFile,
    UsableDirectory
} Loadability;

/**
 * A usable plugin.
 */
class UsablePlugin :
    public QStandardItem
{
public:
    UsablePlugin(const QString &text, bool isDirectory) :
        QStandardItem(KIcon("text-x-python"), text),
        m_isDirectory(isDirectory)
    {
        setCheckable(true);
    }

    virtual int type() const
    {
        return m_isDirectory ? UsableDirectory : UsableFile;
    }

private:
    bool m_isDirectory;
};

/**
 * A hidden plugin.
 */
class HiddenPlugin :
    public QStandardItem
{
public:
    HiddenPlugin(const QString &text) :
        QStandardItem(KIcon("script-error"), text)
    {
    }

    virtual int type() const
    {
        return Hidden;
    }
};

/**
 * Functions for the Python module called pate.
 */
static PyObject *pateSaveConfiguration(PyObject */*self*/, PyObject */*unused*/)
{
    if (Pate::Engine::self()) {
        Pate::Engine::self()->saveConfiguration();
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pateMethods[] =
{
    { "saveConfiguration", pateSaveConfiguration, METH_NOARGS,
    "Save the configuration of the plugin into " CONFIG_FILE },
    { NULL, NULL, 0, NULL }
};

Pate::Engine *Pate::Engine::m_self = 0;

Pate::Engine::Engine(QObject *parent) :
    QStandardItemModel(parent),
    m_pythonLibrary(0),
    m_pythonThreadState(0),
    m_configuration(0),
    m_pluginsLoaded(false)
{
}

Pate::Engine::~Engine()
{
    kDebug() << "Destroy the Python engine";
    if (m_configuration) {
        saveConfiguration();
        Py_DECREF(m_configuration);
    }
    // Shut the interpreter down if it has been started.
    if (Py_IsInitialized()) {
#if THREADED
        PyEval_AcquireThread(m_pythonThreadState);
        Py_Finalize();
#endif
    }
    if (m_pythonLibrary->isLoaded()) {
        m_pythonLibrary->unload();
    }
    delete m_pythonLibrary;
}

Pate::Engine *Pate::Engine::self()
{
    if (!m_self) {
        m_self = new Pate::Engine(qApp);
        if (!m_self->init()) {
            del();
        }
    }
    return m_self;
}

void Pate::Engine::del()
{
    delete m_self;
    m_self = 0;
}

bool Pate::Engine::init()
{
    kDebug() << "Construct the Python engine";
    // Finish setting up the model. At the top level, we have pairs of icons
    // and directory names.
    setColumnCount(2);
    QStringList labels;
    labels << i18n("Name") << i18n("Comment");
    setHorizontalHeaderLabels(labels);

    //     kDebug() << "Creating m_pythonLibrary";
    m_pythonLibrary = new QLibrary(PATE_PYTHON_LIBRARY, this);
    if (!m_pythonLibrary) {
        kError() << "Could not create" << PATE_PYTHON_LIBRARY;
        return false;
    }
    m_pythonLibrary->setLoadHints(QLibrary::ExportExternalSymbolsHint);
    if (!m_pythonLibrary->load()) {
        kError() << "Could not load" << PATE_PYTHON_LIBRARY;
        return false;
    }
    Py_Initialize();
    if (!Py_IsInitialized()) {
        kError() << "Could not initialise" << PATE_PYTHON_LIBRARY;
        return false;
    }
#if THREADED
    PyEval_InitThreads();
#endif
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
    Py_InitModule3(PATE_ENGINE, pateMethods, "The pate module");
    m_configuration = PyDict_New();

    // Host the configuration dictionary.
    moduleSetItemString("configuration", m_configuration);

    // Load the kate module, but find it first, and verify it loads.
    PyObject *katePackage = 0;
    QString katePackageDirectory = KStandardDirs::locate("appdata", "plugins/pate/");
    PyObject *sysPath = moduleGetItemString("path", "sys");
    if (sysPath) {
        Py::appendStringToList(sysPath, katePackageDirectory);
        katePackage = moduleImport("kate");
    }
#if THREADED
    m_pythonThreadState = PyGILState_GetThisThreadState();
    PyEval_ReleaseThread(m_pythonThreadState);
#endif
    if (!katePackage) {
        Py::traceback("Could not import the kate module. Dieing.");
        return false;
    }
    return true;
}

void Pate::Engine::readConfiguration(const QString &groupPrefix)
{
    m_pateConfigGroup = groupPrefix + "load";
    reloadConfiguration();
}

void Pate::Engine::saveConfiguration()
{
    // Now, walk the directories.
    QStandardItem *root = invisibleRootItem();
    KConfigGroup group(KGlobal::config(), m_pateConfigGroup);
    for (int i = 0; i < root->rowCount(); i++) {
        QStandardItem *directoryItem = root->child(i);

        // Walk the plugins in this directory.
        for (int j = 0; j < directoryItem->rowCount(); j++) {
            UsablePlugin *pluginItem = dynamic_cast<UsablePlugin *>(directoryItem->child(j));
            if (!pluginItem) {
                // Don't even try.
                continue;
            }

            // Were we asked to load this plugin?
            QString pluginName = pluginItem->text();
            group.writeEntry(pluginName, pluginItem->checkState() == Qt::Checked);
        }
    }
    KGlobal::config()->sync();
    KConfig config(CONFIG_FILE, KConfig::SimpleConfig);
    Py::updateConfigurationFromDictionary(&config, m_configuration);
    config.sync();
}

void Pate::Engine::reloadConfiguration()
{
    unloadPlugins();
    PyDict_Clear(m_configuration);
    KConfig config(CONFIG_FILE, KConfig::SimpleConfig);
    Py::updateDictionaryFromConfiguration(m_configuration, &config);

    // Clear current state.
    QStandardItem *root = invisibleRootItem();
    root->removeRows(0, root->rowCount());
    QStringList usablePlugins;

    // Find all plugins.
    foreach(QString directoryPath, KGlobal::dirs()->findDirs("appdata", PATE_ENGINE)) {
        QStandardItem *directoryRow = new QStandardItem(KIcon("inode-directory"), directoryPath);
        root->appendRow(directoryRow);
        QDir directory(directoryPath);

        // Traverse the directory.
        QFileInfoList infoList = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
        foreach(QFileInfo info, infoList) {
            QString path = info.absoluteFilePath();
            QString pluginName = path.section('/', -1).section('.', 0, 0);

            // A directory foo must contain foo.py.
            if (info.isDir()) {
                QString tmp = path + '/' + pluginName + ".py";
                QFile f(tmp);
                if (f.exists()) {
                    path = tmp;
                }
            }

            if (path.endsWith(".py")) {
                QList<QStandardItem *> pluginRow;
                // We will only load the first plugin with a give name. The
                // rest will be "hidden".
                if (!usablePlugins.contains(pluginName)) {
                    usablePlugins.append(pluginName);
                    pluginRow.append(new UsablePlugin(pluginName, info.isDir()));
                } else {
                    pluginRow.append(new HiddenPlugin(pluginName));
                    pluginRow.append(new QStandardItem(i18n("Hidden")));
                }
                directoryRow->appendRow(pluginRow);
            } else {
                kDebug() << "Not a valid plugin" << path;
            }
        }
    }
    loadPlugins();
}

void Pate::Engine::loadPlugins()
{
    if (m_pluginsLoaded) {
        return;
    }
    kDebug() << "loading";
    // find plugins and load them.
#if THREADED
    PyGILState_STATE state = PyGILState_Ensure();
#endif
    // Add two lists to the module: pluginDirectories and plugins.
    PyObject *pluginDirectories = PyList_New(0);
    Py_INCREF(pluginDirectories);
    moduleSetItemString("pluginDirectories", pluginDirectories);
    PyObject *plugins = PyList_New(0);
    Py_INCREF(plugins);
    moduleSetItemString("plugins", plugins);

    // Get a reference to sys.path, then add the pate directory to it.
    PyObject *pythonPath = moduleGetItemString("path", "sys");
    QStack<QDir> directories;

    // Now, walk the directories.
    QStandardItem *root = invisibleRootItem();
    KConfigGroup group(KGlobal::config(), m_pateConfigGroup);
    for (int i = 0; i < root->rowCount(); i++) {
        QStandardItem *directoryItem = root->child(i);
        QString directoryPath = directoryItem->text();

        // Add to pate.pluginDirectories and to sys.path.
        Py::appendStringToList(pluginDirectories, directoryPath);
        PyObject *d = Py::unicode(directoryPath);
        PyList_Insert(pythonPath, 0, d);
        Py_DECREF(d);

        // Walk the plugins in this directory.
        for (int j = 0; j < directoryItem->rowCount(); j++) {
            UsablePlugin *pluginItem = dynamic_cast<UsablePlugin *>(directoryItem->child(j));
            if (!pluginItem) {
                // Don't even try.
                continue;
            }

            // Read the plugin config.
            QString pluginName = pluginItem->text();
            pluginItem->setCheckState(group.readEntry(pluginName, false) ? Qt::Checked : Qt::Unchecked);

            // Find the path to the .py file.
            QString path;
            if (pluginItem->type() == UsableDirectory) {
                // This is a directory plugin. The .py is in a subdirectory,
                // add the subdirectory to the path.
                path = directoryPath + pluginName;
                QFile f(path);
                if (f.exists()) {
                    PyObject *d = Py::unicode(path);
                    PyList_Insert(pythonPath, 0, d);
                    Py_DECREF(d);
                } else {
                    directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(i18n("Missing plugin file %1", path)));
                    continue;
                }
            } else {
                path = directoryPath;
            }

            // Were we asked to load this plugin?
            if (pluginItem->checkState() == Qt::Checked) {
                // Import and add to pate.plugins
                PyObject *plugin = moduleImport(PQ(pluginName));
                if (plugin) {
                    PyList_Append(plugins, plugin);
                    Py_DECREF(plugin);
                    directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(i18n("Loaded")));
                } else {
                    directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(i18n("Not Loaded: %1").arg(Py::lastTraceback())));
                }
            }
        }
    }
    m_pluginsLoaded = true;

    // everything is loaded and started. Call the module's init callback
    moduleCallFunction("_pluginsLoaded");
#if THREADED
    PyGILState_Release(state);
#endif
}

void Pate::Engine::unloadPlugins()
{
    // We don't have the luxury of being able to unload Python easily while
    // Kate is running. If anyone can find a way, feel free to tell me and
    // I'll patch it in. Calling Py_Finalize crashes.
    // So, clean up the best that we can.
    if (!m_pluginsLoaded) {
        return;
    }
    kDebug() << "unloading";
#if THREADED
    PyGILState_STATE state = PyGILState_Ensure();
#endif
    // Remove each plugin from sys.modules
    PyObject *modules = PyImport_GetModuleDict();
    PyObject *plugins = moduleGetItemString("plugins");
    for(Py_ssize_t i = 0, j = PyList_Size(plugins); i < j; ++i) {
        PyObject *pluginName = moduleGetItemString("__name__", PyModule_GetDict(PyList_GetItem(plugins, i)));
        if(pluginName && PyDict_Contains(modules, pluginName)) {
            PyDict_DelItem(modules, pluginName);
            kDebug() << "Deleted" << PyString_AsString(pluginName) << "from sys.modules";
        }
    }
    moduleDelItemString("plugins");
    Py_DECREF(plugins);
    m_pluginsLoaded = false;
    moduleCallFunction("_pluginsUnloaded");
#if THREADED
    PyGILState_Release(state);
#endif
}

PyObject *Pate::Engine::wrap(void *o, QString fullClassName) {
    QString classModuleName = fullClassName.section('.', 0, -2);
    QString className = fullClassName.section('.', -1);
    PyObject *classObject = moduleGetItemString(PQ(className), PQ(classModuleName));
    if (!classObject) {
        return 0;
    }
    PyObject *wrapInstance = moduleGetItemString("wrapinstance", "sip");
    if (!wrapInstance) {
        return 0;
    }
    PyObject *arguments = Py_BuildValue("NO", PyLong_FromVoidPtr(o), classObject);
    PyObject *result = PyObject_CallObject(wrapInstance, arguments);
    if(!result) {
        Py::traceback("failed to wrap instance");
        return 0;
    }
    return result;
}

QString Pate::Engine::moduleGetHelp(const char *moduleName) const
{
    Py::Object module = PyImport_ImportModule(moduleName);
    Py::Object func = moduleGetItemString("moduleGetHelp", "kate");
    if (!func) {
        Py::traceback("failed to resolve moduleGetHelp");
        return QString();
    }
    Py::Object arguments = Py_BuildValue("(O)", (PyObject *)module);
    Py::Object result = PyObject_CallObject(++func, arguments);
    if (!result) {
        Py::traceback("failed to call moduleGetHelp");
        return QString();
    }
    return QString(PyString_AsString(result));
}

bool Pate::Engine::moduleCallFunction(const char *functionName, const char *moduleName) const
{
    bool result;
    PyObject *func = moduleGetItemString(functionName, moduleName);
    if (!func) {
        return false;
    }
#if THREADED
    PyGILState_STATE state = PyGILState_Ensure();
#endif
    result = Py::call(func);
#if THREADED
    PyGILState_Release(state);
#endif
    if (!result) {
        Py::traceback(QString("Could not call %1").arg(functionName));
        return false;
    }
    return true;
}

PyObject *Pate::Engine::moduleGetDict(const char *moduleName) const
{
    PyObject *module = moduleImport(moduleName);
    if (!module) {
        return 0;
    }
    PyObject *dictionary = PyModule_GetDict(module);
    if (dictionary) {
        return dictionary;
    }
    Py::traceback(QString("Could not get dict %1").arg(moduleName));
    return 0;
}

bool Pate::Engine::moduleDelItemString(const char *item, const char *moduleName) const
{
    PyObject *dict = moduleGetDict(moduleName);
    if (!dict) {
        return false;
    }
    if (!PyDict_DelItemString(dict, item)) {
        return true;
    }
    Py::traceback(QString("Could not delete item string %1.%2").arg(moduleName).arg(item));
    return false;
}

PyObject *Pate::Engine::moduleGetItemString(const char *item, const char *moduleName) const
{
    PyObject *value = moduleGetItemString(item, moduleGetDict(moduleName));
    if (value) {
        return value;
    }
    Py::traceback(QString("Could not get item string %1.%2").arg(moduleName).arg(item));
    return 0;
}

PyObject *Pate::Engine::moduleGetItemString(const char *item, PyObject *dict) const
{
    if (!dict) {
        return 0;
    }
    PyObject *value = PyDict_GetItemString(dict, item);
    if (value) {
        return value;
    }
    Py::traceback(QString("Could not get item string %1").arg(item));
    return 0;
}

bool Pate::Engine::moduleSetItemString(const char *item, PyObject *value, const char *moduleName) const
{
    PyObject *dict = moduleGetDict(moduleName);
    if (!dict) {
        return false;
    }
    if (PyDict_SetItemString(dict, item, value)) {
        Py::traceback(QString("Could not set item string %1.%2").arg(moduleName).arg(item));
        return false;
    }
    return true;
}

PyObject *Pate::Engine::moduleImport(const char *moduleName) const
{
    PyObject *module = PyImport_ImportModule(moduleName);
    if (module) {
        return module;
    }
    Py::traceback(QString("Could not import %1").arg(moduleName));
    return 0;
}

PyObject *Pate::Engine::moduleGetActions(const char *moduleName) const
{
    Py::Object module = PyImport_ImportModule(moduleName);
    Py::Object func = moduleGetItemString("moduleGetActions", "kate");
    if (!func) {
        Py::traceback("failed to resolve moduleGetActions");
        return 0;
    }
    Py::Object arguments = Py_BuildValue("(O)", (PyObject *)module);
    Py::Object result = PyObject_CallObject(++func, arguments);
    if (!result) {
        Py::traceback("failed to call moduleGetActions");
        return 0;
    }
    return ++result;
}

#include "engine.moc"

// kate: space-indent on;
