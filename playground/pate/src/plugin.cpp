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

#include <KAction>
#include <KDialog>
#include <KLocale>
#include <KGenericFactory>
#include <KConfigBase>
#include <KConfigGroup>

//#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
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
    //page->cbAutoSyncronize->setChecked(config.readEntry("AutoSyncronize", false));
    //page->cbSetEditor->setChecked(config.readEntry("SetEditor", false));

    Pate::Engine::self()->callModuleFunction("_sessionCreated");
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
    //config.writeEntry("AutoSyncronize", page->cbAutoSyncronize->isChecked());
    //config.writeEntry("SetEditor", page->cbSetEditor->isChecked());
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

Pate::ConfigPage::ConfigPage(QWidget *parent, Plugin *plugin) :
    Kate::PluginConfigPage(parent),
    m_plugin(plugin)
{
    kDebug() << "create ConfigPage";
    QVBoxLayout *lo = new QVBoxLayout(this);
    lo->setSpacing(KDialog::spacingHint());

    m_tree = new QTreeView();
    m_tree->setModel(Pate::Engine::self());
    m_tree->expandAll();
    lo->addWidget(m_tree);
    //cbAutoSyncronize = new QCheckBox(i18n("&Automatically synchronize the terminal with the current document when possible"), this);
    //lo->addWidget(cbAutoSyncronize);
    //cbSetEditor = new QCheckBox(i18n("Set &EDITOR environment variable to 'kate -b'"), this);
    //lo->addWidget(cbSetEditor);
    m_reload = new QPushButton(KIcon("system-reboot"), i18n("Reload all"), this);
    lo->addWidget(m_reload);
    QLabel *tmp = new QLabel(this);
    tmp->setText(i18n("Important: The document has to be closed to make the console application continue"));
    lo->addWidget(tmp);
    reset();
    lo->addStretch();
    //connect(cbAutoSyncronize, SIGNAL(stateChanged(int)), SIGNAL(changed()));
    //connect(cbSetEditor, SIGNAL(stateChanged(int)), SIGNAL(changed()));
    connect(m_reload, SIGNAL(clicked(bool)), Pate::Engine::self(), SLOT(reloadConfiguration()));
    connect(m_reload, SIGNAL(clicked(bool)), m_tree, SLOT(expandAll()));
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
