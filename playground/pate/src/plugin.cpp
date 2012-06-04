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

#define CONFIG_SECTION "global"

//
// The Pate plugin
//

K_EXPORT_COMPONENT_FACTORY(pateplugin, KGenericFactory<Pate::Plugin>("pate"))

Pate::Plugin::Plugin(QObject *parent, const QStringList &) :
    Kate::Plugin((Kate::Application *)parent),
    m_autoReload(false)
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
    return new Pate::PluginView(window);
}

/**
 * The configuration system uses one dictionary which is wrapped in Python to
 * make it appear as though it is module-specific.
 * Atomic Python types are stored by writing their representation to the config file
 * on save and evaluating them back to a Python type on load.
 * XX should probably pickle.
 */
void Pate::Plugin::readSessionConfig(KConfigBase *config, const QString &groupPrefix)
{
    KConfigGroup group = config->group(groupPrefix + CONFIG_SECTION);
    m_autoReload = group.readEntry("AutoReload", false);
    Pate::Engine::self()->readConfiguration(groupPrefix);
    Pate::Engine::self()->moduleCallFunction("_sessionCreated");
}

void Pate::Plugin::writeSessionConfig(KConfigBase *config, const QString &groupPrefix)
{
    KConfigGroup group = config->group(groupPrefix + CONFIG_SECTION);
    group.writeEntry("AutoReload", m_autoReload);
    group.sync();
}

uint Pate::Plugin::configPages() const
{
    // The Manager page is always present.
    uint pages = 1;

    // Count the number of plugins which need their own custom page.
    m_moduleConfigPages.clear();
    QStandardItem *root = Pate::Engine::self()->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); i++) {
        QStandardItem *directoryItem = root->child(i);

        // Walk the plugins in this directory.
        for (int j = 0; j < directoryItem->rowCount(); j++) {
            QStandardItem *pluginItem = directoryItem->child(j);
            if (pluginItem->checkState() != Qt::Checked) {
                // Don't even try.
                continue;
            }

            // TODO: Query the engine for this information, and then extend
            // our sibling functions to get the necessary information from
            // the plugins who want to play.
            QString pluginName = directoryItem->child(j)->text();
            PyObject *configPages = Pate::Engine::self()->moduleGetConfigPages(PQ(pluginName));
            for(Py_ssize_t k = 0, l = PyList_Size(configPages); k < l; ++k) {
                // Add an action for this plugin.
                PyObject *tuple = PyList_GetItem(configPages, k);
                m_moduleConfigPages.append(tuple);
                pages++;
            }
        }
    }
    return pages;
}

Kate::PluginConfigPage *Pate::Plugin::configPage(uint number, QWidget *parent, const char *name)
{
    Q_UNUSED(name);

    if (!number) {
        return new Pate::ConfigPage(parent, this);
    }
    if (number > (uint)m_moduleConfigPages.size()) {
        return 0;
    }
    number--;
    PyObject *tuple = m_moduleConfigPages.at(number);
    PyObject *func = PyTuple_GetItem(tuple, 0);
    PyObject *w = Pate::Engine::self()->wrap(parent, "PyQt4.QtGui.QWidget");
    PyObject *arguments = Py_BuildValue("(Oz)", w, name);
    Py_INCREF(func);
    PyObject *result = PyObject_CallObject(func, arguments);
    if (!result) {
        Py::traceback("failed to call plugin page");
        return 0;
    }
    return (Kate::PluginConfigPage *)Pate::Engine::self()->unwrap(result);
}

QString Pate::Plugin::configPageName(uint number) const
{
    if (!number) {
        return i18n("Pâté");
    }
    if (number > (uint)m_moduleConfigPages.size()) {
        return QString();
    }
    number--;
    PyObject *tuple = m_moduleConfigPages.at(number);
    PyObject *configPage = PyTuple_GetItem(tuple, 1);
    PyObject *name = PyTuple_GetItem(configPage, 0);
    return PyString_AsString(name);
}

QString Pate::Plugin::configPageFullName(uint number) const
{
    if (!number) {
        return i18n("Pâté Python Scripting");
    }
    if (number > (uint)m_moduleConfigPages.size()) {
        return QString();
    }
    number--;
    PyObject *tuple = m_moduleConfigPages.at(number);
    PyObject *configPage = PyTuple_GetItem(tuple, 1);
    PyObject *fullName = PyTuple_GetItem(configPage, 1);
    return PyString_AsString(fullName);
}

KIcon Pate::Plugin::configPageIcon(uint number) const
{
    if (!number) {
        return KIcon("applications-development");
    }
    if (number > (uint)m_moduleConfigPages.size()) {
        return KIcon();
    }
    number--;
    PyObject *tuple = m_moduleConfigPages.at(number);
    PyObject *configPage = PyTuple_GetItem(tuple, 1);
    PyObject *icon = PyTuple_GetItem(configPage, 2);
    return *(KIcon *)Pate::Engine::self()->unwrap(icon);
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
    connect(m_manager.autoReload, SIGNAL(clicked(bool)), this, SIGNAL(changed()));
    connect(m_manager.reload, SIGNAL(clicked(bool)), Pate::Engine::self(), SLOT(reloadModules()));
    connect(m_manager.reload, SIGNAL(clicked(bool)), SLOT(reloadPage()));

    // Add a tab for reference information.
    QWidget *infoWidget = new QWidget(m_manager.tabWidget);
    m_info.setupUi(infoWidget);
    m_manager.tabWidget->addTab(infoWidget, i18n("Modules"));
    connect(m_info.topics, SIGNAL(currentIndexChanged(int)), SLOT(infoTopicChanged(int)));
    reloadPage();
}

void Pate::ConfigPage::reloadPage()
{
    m_manager.tree->resizeColumnToContents(0);
    m_manager.tree->expandAll();
    QString topic;

    // Add a topic for each built-in packages, using stacked page 0.
    m_info.topics->clear();
    topic = QLatin1String("kate");
    m_info.topics->addItem(KIcon("applications-development"), topic, QVariant(BUILT_IN));
    topic = QLatin1String("kate.gui");
    m_info.topics->addItem(KIcon("applications-development"), topic, QVariant(BUILT_IN));
    topic = QLatin1String("pate");
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
    if (-1 == topicIndex) {
        // We are being reset.
        Py_XDECREF(m_pluginActions);
        m_pluginActions = 0;
        return;
    }

    // Display the information for the selected module/plugin.
    QString topic = m_info.topics->itemText(topicIndex);
    int optionalSection = m_info.topics->itemData(topicIndex).toInt();
    m_info.help->setHtml(Pate::Engine::self()->moduleGetHelp(PQ(topic)));
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
        m_pluginActions = Pate::Engine::self()->moduleGetActions(PQ(topic));
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
    m_info.text->setText(PyString_AsString(text));
    if (Py_None == icon) {
        m_info.icon->setIcon(QIcon());
    } else if (PyString_Check(icon)) {
        m_info.icon->setIcon(KIcon(PyString_AsString(icon)));
    } else {
        m_info.icon->setIcon(*(QPixmap *)PyCObject_AsVoidPtr(icon));
    }
    m_info.icon->setText(PyString_AsString(icon));
    m_info.shortcut->setText(PyString_AsString(shortcut));
    m_info.menu->setText(PyString_AsString(menu));
}

void Pate::ConfigPage::apply()
{
    // Retrieve the settings from the UI and reflect them in the plugin.
    m_plugin->m_autoReload = m_manager.autoReload->isChecked();
}

void Pate::ConfigPage::reset()
{
    // Retrieve the settings from the plugin and reflect them in the UI.
    m_manager.autoReload->setChecked(m_plugin->m_autoReload);
}

void Pate::ConfigPage::defaults()
{
    // Set the UI to have default settings.
    m_manager.autoReload->setChecked(false);
    emit changed();
}

#include "plugin.moc"
