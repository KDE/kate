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
    Py_InitModule3(Py::PATE_ENGINE, pateMethods, "The pate module");
    m_configuration = PyDict_New();

    // Host the configuration dictionary.
    Py::itemStringSet("configuration", m_configuration);

    // Load the kate module, but find it first, and verify it loads.
    PyObject *katePackage = 0;
    QString katePackageDirectory = KStandardDirs::locate("appdata", "plugins/pate/");
    PyObject *sysPath = Py::itemString("path", "sys");
    if (sysPath) {
        Py::appendStringToList(sysPath, katePackageDirectory);
        katePackage = Py::moduleImport("kate");
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
    KConfigGroup group(KGlobal::config(), m_pateConfigGroup);

    PyDict_Clear(m_configuration);
    KConfig config(CONFIG_FILE, KConfig::SimpleConfig);
    Py::updateDictionaryFromConfiguration(m_configuration, &config);

    // Clear current state.
    QStandardItem *root = invisibleRootItem();
    root->removeRows(0, root->rowCount());
    QStringList usablePlugins;

    // Find all plugins.
    foreach(QString directoryPath, KGlobal::dirs()->findDirs("appdata", Py::PATE_ENGINE)) {
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
                QStandardItem *pluginItem;
                if (!usablePlugins.contains(pluginName)) {
                    usablePlugins.append(pluginName);
                    pluginItem = new UsablePlugin(pluginName, info.isDir());
                    pluginRow.append(pluginItem);
                } else {
                    pluginItem = new HiddenPlugin(pluginName);
                    pluginRow.append(pluginItem);
                    pluginRow.append(new QStandardItem(i18n("Hidden")));
                }

                // Has the user enabled this item or not?
                pluginItem->setCheckState(group.readEntry(pluginName, false) ? Qt::Checked : Qt::Unchecked);
                directoryRow->appendRow(pluginRow);
            } else {
                kDebug() << "Not a valid plugin" << path;
            }
        }
    }
    reloadModules();
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

void Pate::Engine::reloadModules()
{
    unloadModules();
    loadModules();
}

void Pate::Engine::loadModules()
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
    Py::itemStringSet("pluginDirectories", pluginDirectories);
    PyObject *plugins = PyList_New(0);
    Py_INCREF(plugins);
    Py::itemStringSet("plugins", plugins);

    // Get a reference to sys.path, then add the pate directory to it.
    PyObject *pythonPath = Py::itemString("path", "sys");
    QStack<QDir> directories;

    // Now, walk the directories.
    QStandardItem *root = invisibleRootItem();
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

            // Find the path to the .py file.
            QString path;
            QString pluginName = pluginItem->text();
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
                PyObject *plugin = Py::moduleImport(PQ(pluginName));
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
    Py::functionCall("_pluginsLoaded");
#if THREADED
    PyGILState_Release(state);
#endif
}

void Pate::Engine::unloadModules()
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
    PyObject *plugins = Py::itemString("plugins");
    for(Py_ssize_t i = 0, j = PyList_Size(plugins); i < j; ++i) {
        PyObject *pluginName = Py::itemString("__name__", PyModule_GetDict(PyList_GetItem(plugins, i)));
        if(pluginName && PyDict_Contains(modules, pluginName)) {
            PyDict_DelItem(modules, pluginName);
            kDebug() << "Deleted" << PyString_AsString(pluginName) << "from sys.modules";
        }
    }
    Py::itemStringDel("plugins");
    Py_DECREF(plugins);
    m_pluginsLoaded = false;
    Py::functionCall("_pluginsUnloaded");
#if THREADED
    PyGILState_Release(state);
#endif
}

#include "engine.moc"

// kate: space-indent on;
