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

    /// The root configuration used by Python objects. It is a Python
    /// dictionary
    PyObject *configuration();

    QString help(const QString &topic) const;

    /**
     * Call the Pate module's named entry point.
     */
    bool moduleCallFunction(const char *functionName,
                            const char *moduleName = PATE_ENGINE) const;

    /**
     * Get the named module's dictionary.
     */
    PyObject *moduleGetDict(const char *moduleName = PATE_ENGINE) const;

    /**
     * Delete the item from the named module's dictionary.
     */
    bool moduleDelItemString(const char *item,
                             const char *moduleName = PATE_ENGINE) const;

    /**
     * Get the item from the named module's dictionary.
     */
    PyObject *moduleGetItemString(const char *item,
                                  const char *moduleName = PATE_ENGINE) const;

    /**
     * Get the item from the given dictionary.
     */
    PyObject *moduleGetItemString(const char *item, PyObject *dict) const;

    /**
     * Set the item in the named module's dictionary.
     */
    bool moduleSetItemString(const char *item, PyObject *value,
                             const char *moduleName = PATE_ENGINE) const;

    /**
     * Import the named module.
     */
    PyObject *moduleImport(const char *moduleName) const;

    /**
     * Get the Actions defined by a plugin. The returned object is
     * [ { function, ( text, icon, shortcut, menu ) }... ] for each plugin
     * function decorated with @action.
     */
    PyObject *pluginActions(const QString &plugin) const;

    /// A PyObject* for an arbitrary Qt/KDE object that has been wrapped
    /// by SIP. Nifty.
    PyObject *wrap(void *o, QString className);

// signals:
//     void populateConfiguration(PyObject *configurationDictionary);

public slots:
    /// Write out the configuration dictionary to disk
    void saveConfiguration();
    /// (re)Load the configuration into memory from disk
    void reloadConfiguration();

protected:

    Engine(QObject *parent);
    ~Engine();

    /// Start the interpreter.
    bool init();

    /**
     * Walk over the model, loading all usable plugins into a PyObject module
     * dictionary.
     */
    void loadPlugins();
    void unloadPlugins();

private:
    static Engine *m_self;
    QLibrary *m_pythonLibrary;
    PyThreadState *m_pythonThreadState;
    PyObject *m_configuration;
    bool m_pluginsLoaded;
};


} // namespace Pate

#endif
