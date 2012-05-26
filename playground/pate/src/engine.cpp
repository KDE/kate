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
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kate/application.h>

// config.h defines PATE_PYTHON_LIBRARY, the path to libpython.so
// on the build system
#include "config.h"

#include "engine.h"
#include "utilities.h"

#define PATE_MODULE_NAME "pate" 
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

static PyObject *pate_saveConfiguration(PyObject */*self*/) {
    if(Pate::Engine::self()->isInitialised())
        Pate::Engine::self()->saveConfiguration();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pateMethods[] = {
    {"saveConfiguration", (PyCFunction) pate_saveConfiguration, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

Pate::Engine* Pate::Engine::m_self = 0;

Pate::Engine::Engine(QObject *parent) :
    QStandardItemModel(parent)
{
    // Finish setting up the model. At the top level, we have pairs of icons
    // and directory names.
    setColumnCount(2);
    QStringList labels;
    labels << i18n("Name") << i18n("Status");
    setHorizontalHeaderLabels(labels);

    m_initialised = false;
    m_pythonLibrary = 0;
    m_pluginsLoaded = false;
    m_configuration = PyDict_New();
    reloadConfiguration();
}

Pate::Engine::~Engine() {
    // shut the interpreter down if it has been started
    
//     if(m_configuration) {
//         saveConfiguration();
//         Py_DECREF(m_configuration);
//         m_configuration = 0;
//     }
    if(m_initialised) {
        kDebug() << "Unloading m_pythonLibrary...";
        kDebug() << m_pythonLibrary->unload();
        delete m_pythonLibrary;
    }
}

Pate::Engine* Pate::Engine::self() {
    if(!m_self) {
        m_self = new Pate::Engine(qApp);
    }
    return m_self;
}

void Pate::Engine::del() {
    kDebug() << "delete called";
    if(!m_self)
        return;
    if(m_self->isInitialised()) {
        kDebug() << "initialised, acquiring state...";
#if THREADED
        PyEval_AcquireThread(m_self->m_pythonThreadState);
#endif
        kDebug() << "state acquired. Calling _pluginsUnloaded..";
        kDebug() << "Ok";
        kDebug() << "Finalising...";
        Py_Finalize();
        kDebug() << "Finalised.";
    }
    delete m_self;
    kDebug() << "deleted self";
    m_self = 0;
}

bool Pate::Engine::isInitialised() {
    return m_initialised;
}

bool Pate::Engine::init() {
    if(m_initialised)
        return true;
//     kDebug() << "Creating m_pythonLibrary";
    m_pythonLibrary = new QLibrary(PATE_PYTHON_LIBRARY, this);
    m_pythonLibrary->setLoadHints(QLibrary::ExportExternalSymbolsHint);
//     kDebug() << "Caling m_pythonLibrary->load()..";
    if(!m_pythonLibrary->load()) {
        kError() << "Could not load " << PATE_PYTHON_LIBRARY;
        return false;
    }
//     kDebug() << "success!";
//     kDebug() << "Calling Py_Initialize and PyEval_InitThreads...";
    Py_Initialize();
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
//     kDebug() << "success!";
    // initialise our built-in module and import it
//     kDebug() << "Initting built-in module and importting...";
    Py_InitModule(PATE_MODULE_NAME, pateMethods);
    PyObject *pate = PyImport_ImportModule(PATE_MODULE_NAME);
//     kDebug() << "success!";
    PyObject *pateModuleDictionary = PyModule_GetDict(pate);
    // host the configuration dictionary
    PyDict_SetItemString(pateModuleDictionary, "configuration", m_configuration);
    // load the kate module, but find it first, and verify it loads
    QString katePackageDirectory = KStandardDirs::locate("appdata", "plugins/pate/");
    PyObject *sysPath = PyDict_GetItemString(PyModule_GetDict(PyImport_ImportModule("sys")), "path");
    Py::appendStringToList(sysPath, katePackageDirectory);
//     kDebug() << "Importing Kate...";
    PyObject *katePackage = PyImport_ImportModule("kate");
    if(!katePackage) {
        Py::traceback("Could not import the kate module. Dieing.");
        del();
        return false;
    }
//     kDebug() << "success!";
    m_initialised = true;
    reloadConfiguration();
#if THREADED
    m_pythonThreadState = PyGILState_GetThisThreadState();
    PyEval_ReleaseThread(m_pythonThreadState);
#endif
    return true;
}

void Pate::Engine::saveConfiguration() {
    if(!m_configuration || !m_initialised)
        return;
    KConfig config("paterc", KConfig::SimpleConfig);
    Py::updateConfigurationFromDictionary(&config, m_configuration);
    config.sync();
}

void Pate::Engine::reloadConfiguration() 
{
    if (!m_initialised)
        return;
    PyDict_Clear(m_configuration);
    KConfig config("paterc", KConfig::SimpleConfig);
    Py::updateDictionaryFromConfiguration(m_configuration, &config);

    // Clear current state.
    QStandardItem *root = invisibleRootItem();
    root->removeRows(0, root->rowCount());
    QStringList usablePlugins;
    
    // Find all plugins.
    foreach(QString directoryPath, KGlobal::dirs()->findDirs("appdata", "pate")) {
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
}

// void Pate::Engine::die() {
//     PyEval_AcquireThread(m_pythonThreadState);
//     Py_Finalize();
//     // unload the library from memory
//     m_pythonLibrary->unload();
//     m_pythonLibrary = 0;
//     m_pluginsLoaded = false;
//     m_initialised = false;
//     kDebug() << "Pate::Engine::die() finished\n";
// }

void Pate::Engine::loadPlugins() 
{
    if (m_pluginsLoaded)
        return;
    init();

#if THREADED
    PyGILState_STATE state = PyGILState_Ensure();
#endif

    PyObject *pate = PyImport_ImportModule(PATE_MODULE_NAME);
    PyObject *pateModuleDictionary = PyModule_GetDict(pate);
    // find plugins and load them.
    loadPlugins(pateModuleDictionary);
    m_pluginsLoaded = true;
    PyObject *func = PyDict_GetItemString(moduleDictionary(), "_pluginsLoaded");
    if (!func) {
        kDebug() << "No " << PATE_MODULE_NAME << "._pluginsLoaded set";
    }
    // everything is loaded and started. Call the module's init callback
    else if (!Py::call(func)) {
        kError() << "Could not call " << PATE_MODULE_NAME << "._pluginsLoaded().";
    }
#if THREADED
    PyGILState_Release(state);
#endif
}

void Pate::Engine::unloadPlugins() {
    // We don't have the luxury of being able to unload Python easily while
    // Kate is running. If anyone can find a way, feel free to tell me and
    // I'll patch it in. Calling Py_Finalize crashes.
    // So, clean up the best that we can.
    if(!m_pluginsLoaded)
        return;
    kDebug() << "unloading";
#if THREADED
    PyGILState_STATE state = PyGILState_Ensure();
#endif
    PyObject *dict = moduleDictionary();
    PyObject *func = PyDict_GetItemString(dict, "_pluginsUnloaded");
    if(!func) {
        kDebug() << "No " << PATE_MODULE_NAME << "._pluginsUnloaded set";
    }
    else {
        // Remove each plugin from sys.modules
        PyObject *modules = PyImport_GetModuleDict();
        PyObject *plugins = PyDict_GetItemString(dict, "plugins");
        for(Py_ssize_t i = 0, j = PyList_Size(plugins); i < j; ++i) {
            PyObject *pluginName = PyDict_GetItemString(PyModule_GetDict(PyList_GetItem(plugins, i)), "__name__");
            if(pluginName && PyDict_Contains(modules, pluginName)) {
                PyDict_DelItem(modules, pluginName);
                kDebug() << "Deleted" << PyString_AsString(pluginName) << "from sys.modules";
            }
        }
        PyDict_DelItemString(dict, "plugins");
        Py_DECREF(plugins);
        m_pluginsLoaded = false;
        if(!Py::call(func))
            kError() << "Could not call " << PATE_MODULE_NAME << "._pluginsUnloaded()";
    }
#if THREADED
    PyGILState_Release(state);
#endif

}

void Pate::Engine::loadPlugins(PyObject *pateModuleDictionary)
{
    // Add two lists to the module: pluginDirectories and plugins.
    PyObject *pluginDirectories = PyList_New(0);
    Py_INCREF(pluginDirectories);
    PyDict_SetItemString(pateModuleDictionary, "pluginDirectories", pluginDirectories);
    PyObject *plugins = PyList_New(0);
    Py_INCREF(plugins);
    PyDict_SetItemString(pateModuleDictionary, "plugins", plugins);
    
    // Get a reference to sys.path, then add the pate directory to it.
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *pythonPath = PyDict_GetItemString(PyModule_GetDict(sys), "path");
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
            QString pluginName = pluginItem->text();
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
                    kError() << "Missing plugin directory" << path;
                    continue;
                }
            } else {
                path = directoryPath;
            }

            // Import and add to pate.plugins
            PyObject *plugin = PyImport_ImportModule(PQ(pluginName));
            if (plugin) {
                PyList_Append(plugins, plugin);
                Py_DECREF(plugin);
                directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(i18n("Loaded")));
            } else {
                directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(i18n("Not Loaded")));
                Py::traceback(QString("Could not load plugin %1").arg(pluginName));
            }
        }
    }
}

PyObject *Pate::Engine::configuration() {
    return m_configuration;
}
PyObject *Pate::Engine::moduleDictionary() {
    PyObject *pate = PyImport_ImportModule(PATE_MODULE_NAME);
    return PyModule_GetDict(pate);
}
PyObject *Pate::Engine::wrap(void *o, QString fullClassName) {
    PyObject *sip = PyImport_ImportModule("sip");
    if(!sip) {
        Py::traceback("Could not import the sip module.");
        return 0;
    }
    QString classModuleName = fullClassName.section('.', 0, -2);
    QString className = fullClassName.section('.', -1);
    PyObject *classModule = PyImport_ImportModule(classModuleName.toAscii().data());
    if(!classModule) {
        Py::traceback(QString("Could not import %1.").arg(classModuleName));
        return 0;
    }
    PyObject *classObject = PyDict_GetItemString(PyModule_GetDict(classModule), className.toAscii().data());
    if(!classObject) {
        Py::traceback(QString("Could not get item %1 from module %2").arg(className).arg(classModuleName));
        return 0;
    }
    PyObject *wrapInstance = PyDict_GetItemString(PyModule_GetDict(sip), "wrapinstance");
    PyObject *arguments = Py_BuildValue("NO", PyLong_FromVoidPtr(o), classObject);
    PyObject *result = PyObject_CallObject(wrapInstance, arguments);
    if(!result) {
        Py::traceback("failed to wrap instance");
        return 0;
    }
    return result;
}

void Pate::Engine::callModuleFunction(const QString &name) {
#if THREADED
    PyGILState_STATE state = PyGILState_Ensure();
#endif
    PyObject *dict = moduleDictionary();
    PyObject *func = PyDict_GetItemString(dict, PQ(name));
    if(!Py::call(func))
        kDebug() << "Could not call " << PATE_MODULE_NAME << "." << name << "().";    
#if THREADED
    PyGILState_Release(state);
#endif
}

#include "engine.moc"

// kate: space-indent on;
