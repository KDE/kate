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

// config.h defines PATE_PYTHON_LIBRARY, the path to libpython.so
// on the build system
#include "config.h"
#include "Python.h"

#include <QLibrary>
#include <QString>
#include <QStringList>

#include <kconfigbase.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <KLocale>

#include "utilities.h"

#define THREADED 1

namespace Pate
{
const char *Python::PATE_ENGINE = "pate";
QLibrary *Python::s_pythonLibrary = 0;
PyThreadState *Python::s_pythonThreadState = 0;

Python::Python()
{
#if THREADED
    m_state = PyGILState_Ensure();
#endif
}

Python::~Python()
{
#if THREADED
    PyGILState_Release(m_state);
#endif
}

void Python::appendStringToList(PyObject *list, const QString &value)
{
    PyObject *u = unicode(value);
    PyList_Append(list, u);
    Py_DECREF(u);
}

bool Python::functionCall(const char *functionName, const char *moduleName)
{
    PyObject *result = functionCall(functionName, moduleName, PyTuple_New(0));
    if (!result) {
        return false;
    }
    Py_DECREF(result);
    return true;
}

PyObject *Python::functionCall(const char *functionName, const char *moduleName, PyObject *arguments)
{
    if (!arguments) {
        traceback(QString("Missing arguments for %1.%2").arg(moduleName).arg(functionName));
        return 0;
    }
    PyObject *func = itemString(functionName, moduleName);
    if (!func) {
        traceback(QString("Failed to resolve %1.%2").arg(moduleName).arg(functionName));
        return 0;
    }
    if (!PyCallable_Check(func)) {
        traceback(QString("Not callable %1.%2").arg(moduleName).arg(functionName));
        return 0;
    }
    PyObject *result = PyObject_CallObject(func, arguments);
    Py_DECREF(arguments);
    if (!result) {
        traceback(QString("No result from %1.%2").arg(moduleName).arg(functionName));
    }
    return result;
}

bool Python::itemStringDel(const char *item, const char *moduleName)
{
    PyObject *dict = moduleDict(moduleName);
    if (!dict) {
        return false;
    }
    if (!PyDict_DelItemString(dict, item)) {
        return true;
    }
    traceback(QString("Could not delete item string %1.%2").arg(moduleName).arg(item));
    return false;
}

PyObject *Python::itemString(const char *item, const char *moduleName)
{
    PyObject *value = itemString(item, moduleDict(moduleName));
    if (value) {
        return value;
    }
    traceback(QString("Could not get item string %1.%2").arg(moduleName).arg(item));
    return 0;
}

PyObject *Python::itemString(const char *item, PyObject *dict)
{
    if (!dict) {
        return 0;
    }
    PyObject *value = PyDict_GetItemString(dict, item);
    if (value) {
        return value;
    }
    traceback(QString("Could not get item string %1").arg(item));
    return 0;
}

bool Python::itemStringSet(const char *item, PyObject *value, const char *moduleName)
{
    PyObject *dict = moduleDict(moduleName);
    if (!dict) {
        return false;
    }
    if (PyDict_SetItemString(dict, item, value)) {
        traceback(QString("Could not set item string %1.%2").arg(moduleName).arg(item));
        return false;
    }
    return true;
}

PyObject *Python::kateHandler(const char *moduleName, const char *handler)
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

const QString &Python::lastTraceback(void) const
{
    return m_traceback;
}

void Python::libraryLoad()
{
    if (!s_pythonLibrary) {
        kDebug() << "Creating s_pythonLibrary";
        s_pythonLibrary = new QLibrary(PATE_PYTHON_LIBRARY);
        if (!s_pythonLibrary) {
            kError() << "Could not create" << PATE_PYTHON_LIBRARY;
        }
        s_pythonLibrary->setLoadHints(QLibrary::ExportExternalSymbolsHint);
        if (!s_pythonLibrary->load()) {
            kError() << "Could not load" << PATE_PYTHON_LIBRARY;
        }
        Py_InitializeEx(0);
        if (!Py_IsInitialized()) {
            kError() << "Could not initialise" << PATE_PYTHON_LIBRARY;
        }
#if THREADED
        PyEval_InitThreads();
        s_pythonThreadState = PyGILState_GetThisThreadState();
        PyEval_ReleaseThread(s_pythonThreadState);
#endif
    }
}

void Python::libraryUnload()
{
    if (s_pythonLibrary) {
        // Shut the interpreter down if it has been started.
        if (Py_IsInitialized()) {
#if THREADED
            PyEval_AcquireThread(s_pythonThreadState);
#endif
            //Py_Finalize();
        }
        if (s_pythonLibrary->isLoaded()) {
            s_pythonLibrary->unload();
        }
        delete s_pythonLibrary;
        s_pythonLibrary = 0;
    }
}

PyObject *Python::moduleActions(const char *moduleName)
{
    PyObject *result = kateHandler(moduleName, "moduleGetActions");
    return result;
}

PyObject *Python::moduleConfigPages(const char *moduleName)
{
    PyObject *result = kateHandler(moduleName, "moduleGetConfigPages");
    return result;
}

QString Python::moduleHelp(const char *moduleName)
{
    PyObject *result = kateHandler(moduleName, "moduleGetHelp");
    if (!result) {
        return QString();
    }
    #if PY_MAJOR_VERSION < 3
    QString r(PyString_AsString(result));
    #else
    QString r(PyUnicode_AsUnicode(result));
    #endif
    Py_DECREF(result);
    return r;
}

PyObject *Python::moduleDict(const char *moduleName)
{
    PyObject *module = moduleImport(moduleName);
    if (!module) {
        return 0;
    }
    PyObject *dictionary = PyModule_GetDict(module);
    if (dictionary) {
        return dictionary;
    }
    traceback(QString("Could not get dict %1").arg(moduleName));
    return 0;
}

PyObject *Python::moduleImport(const char *moduleName)
{
    PyObject *module = PyImport_ImportModule(moduleName);
    if (module) {
        return module;
    }
    traceback(QString("Could not import %1").arg(moduleName));
    return 0;
}

void *Python::objectUnwrap(PyObject *o)
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

PyObject *Python::objectWrap(void *o, QString fullClassName)
{
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

// Inspired by http://www.gossamer-threads.com/lists/python/python/150924.
void Python::traceback(const QString &description)
{
    m_traceback.clear();
    if (!PyErr_Occurred()) {
        // Return an empty string on no error.
        return;
    }

    PyObject *exc_typ, *exc_val, *exc_tb;
    PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);
    PyErr_NormalizeException(&exc_typ, &exc_val, &exc_tb);

    if (exc_tb) {
        m_traceback = "Traceback (most recent call last):\n";
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
                m_traceback += buffer;
            }
            Py_DECREF(result);
        }
        Py_DECREF(exc_tb);
    }

    // If we have the value, don't bother with the type.
    if (exc_val) {
        PyObject *temp = PyObject_Str(exc_val);
        if (temp) {
            #if PY_MAJOR_VERSION < 3
            m_traceback += PyString_AsString(temp);
            #else
            m_traceback += PyUnicode_AsUnicode(temp);
            #endif
            m_traceback += "\n";
        }
        Py_DECREF(exc_val);
    } else {
        PyObject *temp = PyObject_Str(exc_typ);
        if (temp) {
            #if PY_MAJOR_VERSION < 3
            m_traceback += PyString_AsString(temp);
            #else
            m_traceback += PyUnicode_AsUnicode(temp);
            #endif
            m_traceback += "\n";
        }
    }
    if (exc_typ) {
        Py_DECREF(exc_typ);
    }
    m_traceback += description;
    kError() << m_traceback;
}

PyObject *Python::unicode(const QString &string)
{
    #if PY_MAJOR_VERSION < 3
    PyObject *s = PyString_FromString(PQ(string));
    #else
    PyObject *s = PyUnicode_FromString(PQ(string));
    #endif
    PyObject *u = PyUnicode_FromEncodedObject(s, "utf-8", "strict");
    Py_DECREF(s);
    return u;
}

void Python::updateConfigurationFromDictionary(KConfigBase *config, PyObject *dictionary)
{
    PyObject *groupKey;
    PyObject *groupDictionary;
    Py_ssize_t position = 0;
    while (PyDict_Next(dictionary, &position, &groupKey, &groupDictionary)) {
        #if PY_MAJOR_VERSION < 3
        if (!PyString_Check(groupKey)) {
        #else
        if (!PyUnicode_Check(groupKey)) {
        #endif
            traceback(i18n("Configuration group name not a string"));
            continue;
        }
        #if PY_MAJOR_VERSION < 3
        QString groupName = PyString_AsString(groupKey);
        #else
        QString groupName = PyUnicode_AsUnicode(groupKey);
        #endif
        if (!PyDict_Check(groupDictionary)) {
            traceback(i18n("Configuration group %1 top level key not a dictionary").arg(groupName));
            continue;
        }

        //  There is a group per module.
        KConfigGroup group = config->group(groupName);
        PyObject *key;
        PyObject *value;
        Py_ssize_t x = 0;
        while (PyDict_Next(groupDictionary, &x, &key, &value)) {
            #if PY_MAJOR_VERSION < 3
            if (!PyString_Check(key)) {
            #else
            if (!PyUnicode_Check(key)) {
            #endif
                traceback(i18n("Configuration group %1 itemKey not a string").arg(groupName));
                continue;
            }
            PyObject *arguments = Py_BuildValue("(O)", value);
            PyObject *pickled = functionCall("dumps", "pickle", arguments);
            #if PY_MAJOR_VERSION < 3
            group.writeEntry(PyString_AsString(key), QString(PyString_AsString(pickled)));
            #else
            group.writeEntry(PyUnicode_AsUnicode(key), QString(PyUnicode_AsUnicode(pickled)));
            #endif
            Py_DECREF(pickled);
        }
    }
}

void Python::updateDictionaryFromConfiguration(PyObject *dictionary, const KConfigBase *config)
{
    kDebug() << config->groupList();
    foreach(QString groupName, config->groupList()) {
        KConfigGroup group = config->group(groupName);
        PyObject *groupDictionary = PyDict_New();
        PyDict_SetItemString(dictionary, PQ(groupName), groupDictionary);
        foreach(QString key, group.keyList()) {
            QString pickled = group.readEntry(key);
            PyObject *arguments = Py_BuildValue("(s)", PQ(pickled));
            PyObject *value = functionCall("loads", "pickle", arguments);
            if (value) {
                PyDict_SetItemString(groupDictionary, PQ(key), value);
                Py_DECREF(value);
            }
            else {
                traceback(QString("Bad config value: %1.%2=%3").arg(groupName).arg(key).arg(pickled));
            }
        }
        Py_DECREF(groupDictionary);
    }
}

} // namespace Pate

