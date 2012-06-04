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

#ifndef PATE_ENGINE_H
#define PATE_ENGINE_H

#include <QStandardItemModel>
#include <QList>
#include <QStringList>

#include "Python.h"

class QLibrary;

namespace Pate
{

/**
 * The Engine class hosts the Python interpreter, loading
 * it into memory within Kate, and then with finding and
 * loading all of the Pate plugins. It is implemented as
 * a singleton class.
*/
class Engine :
    public QStandardItemModel
{
    Q_OBJECT

    static const char *PATE_ENGINE;

public:
    static Engine *self();

    /// Close the interpreter and unload it from memory. Called
    /// automatically by the destructor, so you shouldn't need it yourself
    static void del();

    /**
     * Call the named module's named entry point.
     */
    static bool moduleCallFunction(const char *functionName,
                                   const char *moduleName = PATE_ENGINE);

    /**
     * Get the named module's dictionary.
     */
    static PyObject *moduleGetDict(const char *moduleName = PATE_ENGINE);

    /**
     * Delete the item from the named module's dictionary.
     */
    static bool moduleDelItemString(const char *item,
                                    const char *moduleName = PATE_ENGINE);

    /**
     * Get the item from the named module's dictionary.
     */
    static PyObject *moduleGetItemString(const char *item,
                                         const char *moduleName = PATE_ENGINE);

    /**
     * Get the item from the given dictionary.
     */
    static PyObject *moduleGetItemString(const char *item, PyObject *dict);

    /**
     * Set the item in the named module's dictionary.
     */
    static bool moduleSetItemString(const char *item, PyObject *value,
                                    const char *moduleName = PATE_ENGINE);

    /**
     * Import the named module.
     */
    static PyObject *moduleImport(const char *moduleName);

    /**
     * Get the Actions defined by a module. The returned object is
     * [ { function, ( text, icon, shortcut, menu ) }... ] for each module
     * function decorated with @action.
     */
    static PyObject *moduleGetActions(const char *moduleName);

    /**
     * Get the ConfigPages defined by a module. The returned object is
     * [ { function, ( name, fullName, icon ) }... ] for each module function
     * decorated with @configPage.
     */
    static PyObject *moduleGetConfigPages(const char *moduleName);

    /**
     * Get the help text defined by a module.
     */
    static QString moduleGetHelp(const char *moduleName);

    /**
     * A PyObject * for an arbitrary Qt/KDE object using SIP wrapping. Nifty.
     */
    static PyObject *wrap(void *o, QString className);

    /**
     * A void * for an arbitrary Qt/KDE object that has been wrapped by SIP. Nifty.
     */
    static void *unwrap(PyObject *o);

// signals:
//     void populateConfiguration(PyObject *configurationDictionary);

public slots:
    /// Load the configuration.
    void readConfiguration(const QString &groupPrefix);
    /// Write out the configuration.
    void saveConfiguration();
    /// (re)Load the configured modules.
    void reloadModules();

protected:

    Engine(QObject *parent);
    ~Engine();

    /// Start the interpreter.
    bool init();

    /**
     * Walk over the model, loading all usable plugins into a PyObject module
     * dictionary.
     */
    void loadModules();
    void unloadModules();

private:
    static Engine *m_self;
    QLibrary *m_pythonLibrary;
    PyThreadState *m_pythonThreadState;

    /**
     * The root configuration used by Pate itself.
     */
    QString m_pateConfigGroup;

    /**
     * The root configuration used by Pate's Python objects. It is a Python
     * dictionary.
     */
    PyObject *m_configuration;
    bool m_pluginsLoaded;
};


} // namespace Pate

#endif
