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

    void setBroken(bool broken)
    {
        if (broken) {
            setIcon(KIcon("script-error"));
        } else {
            setIcon(KIcon("text-x-python"));
        }
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

static  PyObject *s_pate;

#if PY_MAJOR_VERSION < 3
#define PATE_INIT initpate
#else
#define PATE_INIT PyInit_pate
#endif

PyMODINIT_FUNC PATE_INIT(void)
{
#if PY_MAJOR_VERSION < 3
    s_pate = Py_InitModule3("pate", pateMethods, "The pate module");
    PyModule_AddStringConstant(s_pate, "__file__", __FILE__);
#else
    static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT, "pate", "The pate module",
        -1, pateMethods, 0, 0, 0, 0 };
    s_pate = PyModule_Create(&moduledef);
    PyModule_AddStringConstant(s_pate, "__file__", __FILE__);
    return s_pate;
#endif
}

Pate::Engine *Pate::Engine::s_self = 0;

Pate::Engine::Engine(QObject *parent) :
    QStandardItemModel(parent),
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
    Python::libraryUnload();
}

Pate::Engine *Pate::Engine::self()
{
    if (!s_self) {
        s_self = new Pate::Engine(qApp);
        if (!s_self->init()) {
            del();
        }
    }
    return s_self;
}

void Pate::Engine::del()
{
    delete s_self;
    s_self = 0;
}

bool Pate::Engine::init()
{
    kDebug() << "Construct the Python engine for Python" << PY_MAJOR_VERSION << PY_MINOR_VERSION;
    if (0 != PyImport_AppendInittab(Python::PATE_ENGINE, PATE_INIT)) {
        kError() << "Cannot extend inittab";
        return false;
    }
    Python::libraryLoad();
    Python py = Python();
    // Finish setting up the model. At the top level, we have pairs of icons
    // and directory names.
    setColumnCount(2);
    QStringList labels;
    labels << i18n("Name") << i18n("Comment");
    setHorizontalHeaderLabels(labels);
 
    // Move the custom directories to the front, so they get picked up instead
    // of stale distribution ones.
    QString katePackageDirectory = KStandardDirs::locate("appdata", "plugins/pate/");
    QString sitePackageDirectory = QLatin1String(PATE_PYTHON_SITE_PACKAGES_INSTALL_DIR);
    PyObject *sysPath = py.itemString("path", "sys");
    if (!sysPath) {
        kError() << "Cannot get sys.path";
        return false;
    }
    if (!py.prependStringToList(sysPath, sitePackageDirectory)) {
        return false;
    }
    if (!py.prependStringToList(sysPath, katePackageDirectory)) {
        return false;
    }
    Py_ssize_t len = PyList_Size(sysPath);
    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *path = PyList_GetItem(sysPath, i);
        kDebug() << "sys.path" << i << Python::unicode(path);
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
    PATE_INIT();
    if (!s_pate) {
        kError() << "No pate built-in module";
        return false;
    }
    m_configuration = PyDict_New();

    // Host the configuration dictionary.
    py.itemStringSet("configuration", m_configuration);

    // Load the kate module, but find it first, and verify it loads.
    PyObject *katePackage = py.moduleImport("kate");
    if (!katePackage) {
        return false;
    }
    return true;
}

void Pate::Engine::readConfiguration(const QString &groupPrefix)
{
    m_pateConfigGroup = groupPrefix + "load";
    reloadConfiguration();
}

void Pate::Engine::reloadConfiguration()
{
    Python py = Python();

    KConfigGroup group(KGlobal::config(), m_pateConfigGroup);

    PyDict_Clear(m_configuration);
    KConfig config(CONFIG_FILE, KConfig::SimpleConfig);
    py.updateDictionaryFromConfiguration(m_configuration, &config);

    // Clear current state.
    QStandardItem *root = invisibleRootItem();
    root->removeRows(0, root->rowCount());
    QStringList usablePlugins;

    // Find all plugins.
    foreach(QString directoryPath, KGlobal::dirs()->findDirs("appdata", py.PATE_ENGINE)) {
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
                bool usable = false;
                // We will only load the first plugin with a give name. The
                // rest will be "hidden".
                QStandardItem *pluginItem;
                if (!usablePlugins.contains(pluginName)) {
                    usablePlugins.append(pluginName);
                    usable = true;
                    pluginItem = new UsablePlugin(pluginName, info.isDir());
                    pluginRow.append(pluginItem);
                } else {
                    pluginItem = new HiddenPlugin(pluginName);
                    pluginRow.append(pluginItem);
                    pluginRow.append(new QStandardItem(i18n("Hidden")));
                }

                // Has the user enabled this item or not?
                pluginItem->setCheckState(usable && group.readEntry(pluginName, false) ? Qt::Checked : Qt::Unchecked);
                directoryRow->appendRow(pluginRow);
            } else {
                kDebug() << "Not a valid plugin" << path;
            }
        }
    }
    unloadModules();
    loadModules();
}

void Pate::Engine::saveConfiguration()
{
    Python py = Python();

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
    py.updateConfigurationFromDictionary(&config, m_configuration);
    config.sync();
}

void Pate::Engine::loadModules()
{
    if (m_pluginsLoaded) {
        return;
    }
    kDebug() << "loading";
    // find plugins and load them.
    // Add two lists to the module: pluginDirectories and plugins.
    Python py = Python();
    PyObject *pluginDirectories = PyList_New(0);
    Py_INCREF(pluginDirectories);
    py.itemStringSet("pluginDirectories", pluginDirectories);
    PyObject *plugins = PyList_New(0);
    Py_INCREF(plugins);
    py.itemStringSet("plugins", plugins);

    // Get a reference to sys.path, then add the pate directory to it.
    PyObject *pythonPath = py.itemString("path", "sys");
    QStack<QDir> directories;

    // Now, walk the directories.
    QStandardItem *root = invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        QStandardItem *directoryItem = root->child(i);
        QString directoryPath = directoryItem->text();

        // Add to pate.pluginDirectories and to sys.path.
        py.prependStringToList(pluginDirectories, directoryPath);
        PyObject *d = Python::unicode(directoryPath);
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
                    PyObject *d = Python::unicode(path);
                    PyList_Insert(pythonPath, 0, d);
                    Py_DECREF(d);
                } else {
                    pluginItem->setBroken(true);
                    directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(i18n("Missing plugin file %1", path)));
                    continue;
                }
            } else {
                path = directoryPath;
            }

            // Were we asked to load this plugin?
            if (pluginItem->checkState() == Qt::Checked) {
                // Import and add to pate.plugins
                PyObject *plugin = py.moduleImport(PQ(pluginName));
                if (plugin) {
                    PyList_Append(plugins, plugin);
                    Py_DECREF(plugin);
                    pluginItem->setBroken(false);

                    // Get a description of the plugin if we can.
                    PyObject *doc = py.itemString("__doc__", PQ(pluginName));
                    QString comment = Python::isUnicode(doc) ? Python::unicode(doc) : i18n("Loaded");

                    directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(comment.split("\n")[0]));
                } else {
                    pluginItem->setBroken(true);
                    directoryItem->setChild(pluginItem->row(), 1, new QStandardItem(i18n("Not Loaded: %1").arg(py.lastTraceback())));
                }
            } else {
                // Remove any previously set status.
                delete directoryItem->takeChild(pluginItem->row(), 1);
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
    if (!m_pluginsLoaded) {
        return;
    }
    kDebug() << "unloading";

    // Remove each plugin from sys.modules
    Python py = Python();
    PyObject *modules = PyImport_GetModuleDict();
    PyObject *plugins = py.itemString("plugins");
    if (plugins) {
        for (Py_ssize_t i = 0, j = PyList_Size(plugins); i < j; ++i) {
            PyObject *pluginName = py.itemString("__name__", PyModule_GetDict(PyList_GetItem(plugins, i)));
            if(pluginName && PyDict_Contains(modules, pluginName)) {
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

#include "engine.moc"

// kate: space-indent on;
