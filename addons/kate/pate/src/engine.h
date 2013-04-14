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

public:
    static Engine *self();

    /// Close the interpreter and unload it from memory. Called
    /// automatically by the destructor, so you shouldn't need it yourself
    static void del();


public slots:
    /**
     * Load the configuration.
     */
    void readConfiguration(const QString &groupPrefix);
    /// Write out the configuration.
    void saveConfiguration();
    /// (re)Load the configured modules.
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
    void loadModules();
    void unloadModules();

private:
    static Engine *s_self;

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
