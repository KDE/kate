// This file is part of Pate, Kate' Python scripting plugin.
//
// Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
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

#include <iostream>

#include "Python.h"

#include <QString>
#include <QStringList>

#include <kconfigbase.h>
#include <kconfiggroup.h>
#include <kdebug.h>

#include "utilities.h"


namespace Pate { namespace Py {

PyObject *unicode(const QString &string) {
    PyObject *s = PyString_FromString(PQ(string));
    PyObject *u = PyUnicode_FromEncodedObject(s, "utf-8", "strict");
    Py_DECREF(s);
    return u;
}

bool call(PyObject *function, PyObject *arguments) {
    PyObject *result = PyObject_CallObject(function, arguments);
    if(result != NULL) {
        // success
        Py_DECREF(result);
        return true;
    }
    std::cerr << TERMINAL_RED << "Py::call:\n";
    PyErr_Print();
    std::cerr << TERMINAL_CLEAR;
    return false;
}

bool call(PyObject *function) {
    // Overload: call a Python callable with no arguments
    PyObject *arguments = PyTuple_New(0);
    bool result = call(function, arguments);
    Py_DECREF(arguments);
    return result;
}

void appendStringToList(PyObject *list, const QString &value) {
    PyObject *u = unicode(value);
    PyList_Append(list, u);
    Py_DECREF(u);
}

void traceback(const QString &description) {
    std::cerr << TERMINAL_RED;
    PyErr_Print();
    std::cerr << PQ(description) << TERMINAL_CLEAR << std::endl;
}


/// Create a Python dictionary from a KConfigBase instance, 
/// writing the string representation of the values
void updateDictionaryFromConfiguration(PyObject *dictionary, KConfigBase *config) {
    // relatively safe evaluation environment for Pythonizing the serialised types:
    PyObject *evaluationLocals = PyDict_New();
    PyObject *evaluationGlobals = PyDict_New();
    PyObject *evaluationBuiltins = PyDict_New();
    PyDict_SetItemString(evaluationGlobals, "__builtin__", evaluationBuiltins);
    kDebug() << config->groupList();
    foreach(QString groupName, config->groupList()) {
        KConfigGroup group = config->group(groupName);
        PyObject *groupDictionary = PyDict_New();
        PyDict_SetItemString(dictionary, PQ(groupName), groupDictionary);
        foreach(QString key, group.keyList()) {
            QString valueString = group.readEntry(key);
            PyObject *value = PyRun_String(PQ(group.readEntry(key)), Py_eval_input, evaluationLocals, evaluationGlobals);
            if(value) {
                PyDict_SetItemString(groupDictionary, PQ(key), value);
            }
            else {
                Py::traceback(QString("Bad config value: %1").arg(valueString));
            }
        }
    }
    Py_DECREF(evaluationBuiltins);
    Py_DECREF(evaluationGlobals);
    Py_DECREF(evaluationLocals);
}

/// Write a Python dictionary to a configuration object, converting
/// objects to their string representation along the way
void updateConfigurationFromDictionary(KConfigBase *config, PyObject *dictionary) {
    PyObject *groupName, *groupDictionary;
    Py_ssize_t position = 0;
    while(PyDict_Next(dictionary, &position, &groupName, &groupDictionary)) {
        Py_ssize_t x = 0;
        PyObject *key, *value;
        if(!PyString_AsString(groupName)) {
            Py::traceback(QString("Configuration group name not a string"));
            continue;
        }
        if(!PyDict_Check(groupDictionary)) {
            std::cerr << TERMINAL_RED << "configuration value for key '" << PyString_AsString(groupName) << "' in top level is not a dictionary; ignoring" << TERMINAL_CLEAR << '\n';
            continue;
        }
        KConfigGroup group = config->group(PyString_AsString(groupName));
        while(PyDict_Next(groupDictionary, &x, &key, &value)) {
            if(!PyString_AsString(key)) {
                Py::traceback(QString("Configuration item key not a string"));
                continue;
            }
            QString keyString = PyString_AsString(key);
            PyObject *pyRepresentation = PyObject_Repr(value);
            if(!pyRepresentation) {
                Py::traceback(QString("Could not get the representation of the value for '%1'").arg(keyString));
                continue;
            }
            QString valueString = PyString_AsString(pyRepresentation);
            group.writeEntry(keyString, valueString);
            Py_DECREF(pyRepresentation);
        }
    }
}


}} // namespace Py, namespace Pate

