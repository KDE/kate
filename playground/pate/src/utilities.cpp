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

#include "Python.h"

#include <QString>
#include <QStringList>

#include <kconfigbase.h>
#include <kconfiggroup.h>
#include <kdebug.h>

#include "utilities.h"

namespace Pate
{

namespace Py
{
static PyObject *runKateHandler(const char *moduleName, const char *handler);
static PyObject *functionCall(const char *functionName, const char *moduleName, PyObject *arguments);

const char *PATE_ENGINE = "pate";


PyObject *unicode(const QString &string) {
    PyObject *s = PyString_FromString(PQ(string));
    PyObject *u = PyUnicode_FromEncodedObject(s, "utf-8", "strict");
    Py_DECREF(s);
    return u;
}

void appendStringToList(PyObject *list, const QString &value) {
    PyObject *u = unicode(value);
    PyList_Append(list, u);
    Py_DECREF(u);
}

static QString s_traceback;

// Inspired by http://www.gossamer-threads.com/lists/python/python/150924.
void traceback(const QString &description)
{
    s_traceback.clear();
    if (!PyErr_Occurred()) {
        // Return an empty string on no error.
        return;
    }

    PyObject *exc_typ, *exc_val, *exc_tb;
    PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);
    PyErr_NormalizeException(&exc_typ, &exc_val, &exc_tb);

    if (exc_tb) {
        s_traceback = "Traceback (most recent call last):\n";
        PyObject *arguments = PyTuple_New(1);
        PyTuple_SetItem(arguments, 0, exc_tb);
        PyObject *result = functionCall("format_tb", "traceback", arguments);
        if (result) {
            for (int i = 0, j = PyList_Size(result); i < j; i++) {
                PyObject *tt = PyList_GetItem(result, i);
                PyObject *t = Py_BuildValue("(O)", tt);
                char *buffer;
                if (!PyArg_ParseTuple(t, "s", &buffer)) {
                    break;
                }
                s_traceback += buffer;
            }
            Py_DECREF(result);
        }
        Py_DECREF(exc_tb);
    }

    // If we have the value, don't bother with the type.
    if (exc_val) {
        PyObject *temp = PyObject_Str(exc_val);
        if (temp) {
            s_traceback += PyString_AsString(temp);
            s_traceback += "\n";
        }
        Py_DECREF(exc_val);
    } else {
        PyObject *temp = PyObject_Str(exc_typ);
        if (temp) {
            s_traceback += PyString_AsString(temp);
            s_traceback += "\n";
        }
    }
    if (exc_typ) {
        Py_DECREF(exc_typ);
    }
    s_traceback += description;
    kError() << s_traceback;
}

const QString &lastTraceback(void)
{
    return s_traceback;
}

/// Create a Python dictionary from a KConfigBase instance,
/// writing the string representation of the values
void updateDictionaryFromConfiguration(PyObject *dictionary, const KConfigBase *config)
{
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
            if (value) {
                PyDict_SetItemString(groupDictionary, PQ(key), value);
                Py_DECREF(value);
            }
            else {
                Py::traceback(QString("Bad config value: %1").arg(valueString));
            }
        }
        Py_DECREF(groupDictionary);
    }
    Py_DECREF(evaluationBuiltins);
    Py_DECREF(evaluationGlobals);
    Py_DECREF(evaluationLocals);
}

/// Write a Python dictionary to a configuration object, converting
/// objects to their string representation along the way
void updateConfigurationFromDictionary(KConfigBase *config, PyObject *dictionary)
{
    PyObject *groupName, *groupDictionary;
    Py_ssize_t position = 0;
    while (PyDict_Next(dictionary, &position, &groupName, &groupDictionary)) {
        if (!PyString_AsString(groupName)) {
            Py::traceback(QString("Configuration group name not a string"));
            continue;
        }
        if (!PyDict_Check(groupDictionary)) {
            kError() << "configuration value for key '" << PyString_AsString(groupName) << "' in top level is not a dictionary; ignoring";
            continue;
        }

        //  There is a group per module.
        KConfigGroup group = config->group(PyString_AsString(groupName));
        PyObject *key, *value;
        Py_ssize_t x = 0;
        while (PyDict_Next(groupDictionary, &x, &key, &value)) {
            if (!PyString_AsString(key)) {
                Py::traceback(QString("Configuration item key not a string"));
                continue;
            }
            QString keyString = PyString_AsString(key);
            PyObject *pyRepresentation = PyObject_Repr(value);
            if (!pyRepresentation) {
                Py::traceback(QString("Could not get the representation of the value for '%1'").arg(keyString));
                continue;
            }
            QString valueString = PyString_AsString(pyRepresentation);
            group.writeEntry(keyString, valueString);
            Py_DECREF(pyRepresentation);
        }
    }
}

bool functionCall(const char *functionName, const char *moduleName)
{
    PyObject *result = functionCall(functionName, moduleName, PyTuple_New(0));
    if (!result) {
        return false;
    }
    Py_DECREF(result);
    return true;
}

static PyObject *functionCall(const char *functionName, const char *moduleName, PyObject *arguments)
{
    if (!arguments) {
        Py::traceback(QString("Missing arguments for %1.%2").arg(moduleName).arg(functionName));
        return 0;
    }
    PyObject *func = itemString(functionName, moduleName);
    if (!func) {
        Py::traceback(QString("Failed to resolve %1.%2").arg(moduleName).arg(functionName));
        return 0;
    }
    if (!PyCallable_Check(func)) {
        Py::traceback(QString("Not callable %1.%2").arg(moduleName).arg(functionName));
        return 0;
    }
    PyObject *result = PyObject_CallObject(func, arguments);
    Py_DECREF(arguments);
    if (!result) {
        Py::traceback(QString("No result from %1.%2").arg(moduleName).arg(functionName));
    }
    return result;
}

bool itemStringDel(const char *item, const char *moduleName)
{
    PyObject *dict = moduleDict(moduleName);
    if (!dict) {
        return false;
    }
    if (!PyDict_DelItemString(dict, item)) {
        return true;
    }
    Py::traceback(QString("Could not delete item string %1.%2").arg(moduleName).arg(item));
    return false;
}

PyObject *itemString(const char *item, const char *moduleName)
{
    PyObject *value = itemString(item, moduleDict(moduleName));
    if (value) {
        return value;
    }
    Py::traceback(QString("Could not get item string %1.%2").arg(moduleName).arg(item));
    return 0;
}

PyObject *itemString(const char *item, PyObject *dict)
{
    if (!dict) {
        return 0;
    }
    PyObject *value = PyDict_GetItemString(dict, item);
    if (value) {
        return value;
    }
    Py::traceback(QString("Could not get item string %1").arg(item));
    return 0;
}

bool itemStringSet(const char *item, PyObject *value, const char *moduleName)
{
    PyObject *dict = moduleDict(moduleName);
    if (!dict) {
        return false;
    }
    if (PyDict_SetItemString(dict, item, value)) {
        Py::traceback(QString("Could not set item string %1.%2").arg(moduleName).arg(item));
        return false;
    }
    return true;
}

PyObject *moduleActions(const char *moduleName)
{
    PyObject *result = runKateHandler(moduleName, "moduleGetActions");
    return result;
}

PyObject *moduleConfigPages(const char *moduleName)
{
    PyObject *result = runKateHandler(moduleName, "moduleGetConfigPages");
    return result;
}

QString moduleHelp(const char *moduleName)
{
    PyObject *result = runKateHandler(moduleName, "moduleGetHelp");
    if (!result) {
        return QString();
    }
    QString r(PyString_AsString(result));
    Py_DECREF(result);
    return r;
}

PyObject *moduleDict(const char *moduleName)
{
    PyObject *module = moduleImport(moduleName);
    if (!module) {
        return 0;
    }
    PyObject *dictionary = PyModule_GetDict(module);
    if (dictionary) {
        return dictionary;
    }
    Py::traceback(QString("Could not get dict %1").arg(moduleName));
    return 0;
}

PyObject *moduleImport(const char *moduleName)
{
    PyObject *module = PyImport_ImportModule(moduleName);
    if (module) {
        return module;
    }
    Py::traceback(QString("Could not import %1").arg(moduleName));
    return 0;
}

void *objectUnwrap(PyObject *o)
{
    PyObject *arguments = Py_BuildValue("(O)", o);
    PyObject *result = functionCall("unwrapinstance", "sip", arguments);
    if (!result) {
        return 0;
    }
    void *r = (void *)(ptrdiff_t)PyLong_AsLongLong(result);
    Py_DECREF(result);
    return r;
}

PyObject *objectWrap(void *o, QString fullClassName) {
    QString classModuleName = fullClassName.section('.', 0, -2);
    QString className = fullClassName.section('.', -1);
    PyObject *classObject = itemString(PQ(className), PQ(classModuleName));
    if (!classObject) {
        return 0;
    }
    PyObject *arguments = Py_BuildValue("NO", PyLong_FromVoidPtr(o), classObject);
    PyObject *result = functionCall("wrapinstance", "sip", arguments);
    if (!result) {
        return 0;
    }
    return result;
}

/**
 * Run a handler function supplied by the kate module on another module.
 *
 * @return 0 or a new reference to the result.
 */
static PyObject *runKateHandler(const char *moduleName, const char *handler)
{
    PyObject *module = moduleImport(moduleName);
    if (!module) {
        return 0;
    }
    PyObject *arguments = Py_BuildValue("(O)", (PyObject *)module);
    PyObject *result = functionCall(handler, "kate", arguments);
    if (!result) {
        return 0;
    }
    return result;
}

}

} // namespace Py, namespace Pate

