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

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeView>
#include <QVBoxLayout>

#define CONFIG_SECTION "Pate"

//
// The Pate plugin
//

K_EXPORT_COMPONENT_FACTORY(pateplugin, KGenericFactory<Pate::Plugin>("pate"))

Pate::Plugin::Plugin(QObject *parent, const QStringList &) :
    Kate::Plugin((Kate::Application *)parent)
{
    if (!Pate::Engine::self()) {
        kError() << "Could not initialise Pate. Ouch!";
    }
}

Pate::Plugin::~Plugin() {
    Pate::Engine::del();
}

Kate::PluginView *Pate::Plugin::createView(Kate::MainWindow *window)
{
    Pate::Engine::self()->reloadConfiguration();
    return new Pate::PluginView(window);
}

/**
 * The configuration system uses one dictionary which is wrapped in Python to
 * make it appear as though it is module-specific.
 * Atomic Python types are stored by writing their representation to the config file
 * on save and evaluating them back to a Python type on load.
 * XX should probably pickle.
 */
void Pate::Plugin::readConfig(Pate::ConfigPage *page)
{
    KConfigGroup config(KGlobal::config(), CONFIG_SECTION);
    page->m_manager.autoReload->setChecked(config.readEntry("AutoReload", false));

    Pate::Engine::self()->moduleCallFunction("_sessionCreated");
//     PyGILState_STATE state = PyGILState_Ensure();
//
//     PyObject *d = Pate::Engine::self()->moduleDictionary();
//     kDebug() << "setting configuration";
//     PyDict_SetItemString(d, "sessionConfiguration", Pate::Engine::self()->wrap((void *) config, "PyKDE4.kdecore.KConfigBase"));
//     if(!config->hasGroup("Pate")) {
//         PyGILState_Release(state);
//
//         return;
//     }
//     // relatively safe evaluation environment for Pythonizing the serialised types:
//     PyObject *evaluationLocals = PyDict_New();
//     PyObject *evaluationGlobals = PyDict_New();
//     PyObject *evaluationBuiltins = PyDict_New();
//     PyDict_SetItemString(evaluationGlobals, "__builtin__", evaluationBuiltins);
//     // read config values for our group, shoving the Python evaluated value into a dict
//     KConfigGroup group = config->group("Pate");
//     foreach(QString key, group.keyList()) {
//         QString valueString = group.readEntry(key);
//         PyObject *value = PyRun_String(PQ(group.readEntry(key)), Py_eval_input, evaluationLocals, evaluationGlobals);
//         if(value) {
//             PyObject *c = Pate::Engine::self()->configuration();
//             PyDict_SetItemString(c, PQ(key), value);
//         }
//         else {
//             Py::traceback(QString("Bad config value: %1").arg(valueString));
//         }
//     }
//     Py_DECREF(evaluationBuiltins);
//     Py_DECREF(evaluationGlobals);
//     Py_DECREF(evaluationLocals);
//     PyGILState_Release(state);

}

void Pate::Plugin::writeConfig(Pate::ConfigPage *page)
{
    KConfigGroup config(KGlobal::config(), CONFIG_SECTION);
    config.writeEntry("AutoReload", page->m_manager.autoReload->isChecked());
    config.sync();
//     // write session config data
//     kDebug() << "write session config\n";
//     KConfigGroup group(config, "Pate");
//     PyGILState_STATE state = PyGILState_Ensure();
//
//     PyObject *key, *value;
//     Py_ssize_t position = 0;
//     while(PyDict_Next(Pate::Engine::self()->configuration(), &position, &key, &value)) {
//         // ho ho
//         QString keyString = PyString_AsString(key);
//         PyObject *pyRepresentation = PyObject_Repr(value);
//         if(!pyRepresentation) {
//             Py::traceback(QString("Could not get the representation of the value for '%1'").arg(keyString));
//             continue;
//         }
//         QString valueString = PyString_AsString(pyRepresentation);
//         group.writeEntry(keyString, valueString);
//         Py_DECREF(pyRepresentation);
//     }
//     PyGILState_Release(state);
}

uint Pate::Plugin::configPages() const
{
    // The Manager page is always present.
    uint pages = 1;

    // Count the number of plugins which need their own custom page.
    QStandardItem *root = Pate::Engine::self()->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        QStandardItem *directoryItem = root->child(i);

        // Walk the plugins in this directory.
        for (int j = 0; j < directoryItem->rowCount(); j++) {
            // TODO: Query the engine for this information, and then extend
            // our sibling functions to get the necessary information from
            // the plugins who want to play.

            //QString pluginName = directoryItem->child(j)->text();
            //pages++;
        }
    }
    return pages;
}

Kate::PluginConfigPage *Pate::Plugin::configPage(uint number, QWidget *parent, const char *name)
{
    Q_UNUSED(name);

    if (number != 0)
        return 0;
    return new Pate::ConfigPage(parent, this);
}

QString Pate::Plugin::configPageName (uint number) const
{
    if (number != 0)
        return QString();
    return i18n("Pâté");
}

QString Pate::Plugin::configPageFullName (uint number) const
{
    if (number != 0)
        return QString();
    return i18n("Python Scripting");
}

KIcon Pate::Plugin::configPageIcon (uint number) const
{
    if (number != 0)
        return KIcon();
    return KIcon("applications-development");
}

//
// Plugin view, instances of which are created once for each session.
//

Pate::PluginView::PluginView(Kate::MainWindow *window) :
    Kate::PluginView(window)
{
    kDebug() << "create PluginView";
}

//
// Plugin configuration view.
//

#define BUILT_IN 0
#define PLUGIN 1
Pate::ConfigPage::ConfigPage(QWidget *parent, Plugin *plugin) :
    Kate::PluginConfigPage(parent),
    m_plugin(plugin),
    m_pluginActions(0)
{
    kDebug() << "create ConfigPage";

    // Create a page with just the main manager tab.
    m_manager.setupUi(parent);
    m_manager.tree->setModel(Pate::Engine::self());
    reset();
    connect(m_manager.autoReload, SIGNAL(stateChanged(int)), SLOT(apply()));
    connect(m_manager.reload, SIGNAL(clicked(bool)), SLOT(reloadConfiguration()));

    // Add a tab for reference information.
    QWidget *infoWidget = new QWidget(m_manager.tabWidget);
    m_info.setupUi(infoWidget);
    m_manager.tabWidget->addTab(infoWidget, i18n("Packages"));
    connect(m_info.topics, SIGNAL(currentIndexChanged(int)), SLOT(infoTopicChanged(int)));
    reloadConfiguration();
}

void Pate::ConfigPage::reloadConfiguration()
{
    Pate::Engine::self()->reloadConfiguration();
    m_manager.tree->resizeColumnToContents(0);
    m_manager.tree->expandAll();
    QString topic;

    // Add a topic for each built-in packages, using stacked page 0.
    m_info.topics->clear();
    topic = QLatin1String("kate");
    m_info.topics->addItem(KIcon("applications-development"), topic, QVariant(BUILT_IN));
    topic = QLatin1String("kate.gui");
    m_info.topics->addItem(KIcon("applications-development"), topic, QVariant(BUILT_IN));

    // Add a topic for each plugin. using stacked page 1.
    PyObject *plugins = Pate::Engine::self()->moduleGetItemString("plugins");
    for(Py_ssize_t i = 0, j = PyList_Size(plugins); i < j; ++i) {
        PyObject *module = PyList_GetItem(plugins, i);

        // Add a topic for this plugin, using stacked page 1.
        topic = QLatin1String(PyModule_GetName(module));
        m_info.topics->addItem(KIcon("text-x-python"), topic, QVariant(PLUGIN));
    }
    infoTopicChanged(0);
}

void Pate::ConfigPage::infoTopicChanged(int topicIndex)
{
    QString topic = m_info.topics->itemText(topicIndex);
    int optionalSection = m_info.topics->itemData(topicIndex).toInt();
    m_info.help->setHtml(Pate::Engine::self()->help(topic));
    m_info.optionalSection->setCurrentIndex(optionalSection);
    switch (optionalSection) {
    case BUILT_IN:
        m_info.optionalSection->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        Py_XDECREF(m_pluginActions);
        m_pluginActions = 0;
        break;
    case PLUGIN:
        m_info.optionalSection->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        // Populate the plugin-specific action information.
        Py_XDECREF(m_pluginActions);
        m_pluginActions = Pate::Engine::self()->pluginActions(topic);
        m_info.actions->clear();
        for(Py_ssize_t i = 0, j = PyList_Size(m_pluginActions); i < j; ++i) {
            PyObject *tuple = PyList_GetItem(m_pluginActions, i);
            PyObject *functionName = PyTuple_GetItem(tuple, 0);

            // Add an action for this plugin.
            m_info.actions->addItem(PyString_AsString(functionName));
        }
        infoPluginActionsChanged(0);
        break;
    }
}

void Pate::ConfigPage::infoPluginActionsChanged(int actionIndex)
{
    if (!m_pluginActions) {
        // This is a bit wierd.
        return;
    }
    PyObject *tuple = PyList_GetItem(m_pluginActions, actionIndex);
    if (!tuple) {
        // This is a bit wierd: a plugin with no executable actions?
        return;
    }

    PyObject *action = PyTuple_GetItem(tuple, 1);
    PyObject *text = PyTuple_GetItem(action, 0);
    PyObject *icon = PyTuple_GetItem(action, 1);
    PyObject *shortcut = PyTuple_GetItem(action, 2);
    PyObject *menu = PyTuple_GetItem(action, 3);

    // Add a topic for this plugin, using stacked page 0.
    // TODO: Proper handling of Unicode
    // TODO: handling of actual QPixmaps and so on for icon.
    kError() << PyString_Check(text) << PyString_Check(icon) << PyString_Check(shortcut) << PyString_Check(menu);
    kError() << PyUnicode_Check(text) << PyUnicode_Check(icon) << PyUnicode_Check(shortcut) << PyUnicode_Check(menu);
    m_info.text->setText(PyString_AsString(text));
    m_info.icon->setText(PyString_AsString(icon));
    m_info.icon->setText(PyString_AsString(icon));
    m_info.shortcut->setText(PyString_AsString(shortcut));
    m_info.menu->setText(PyString_AsString(menu));
}

void Pate::ConfigPage::apply()
{
    m_plugin->writeConfig(this);
}

void Pate::ConfigPage::reset()
{
    m_plugin->readConfig(this);
}

#include "plugin.moc"
