// This file is part of Pate, Kate' Python scripting plugin.
//
// A couple of useful macros and functions used inside of pate_engine.cpp and pate_plugin.cpp.
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

#ifndef PATE_UTILITIES_H
#define PATE_UTILITIES_H

#include "Python.h"

class QString;
class KConfigBase;

// save us some ruddy time when printing out QStrings with UTF-8
#define PQ(x) x.toUtf8().constData()

namespace Pate
{

namespace Py
{

/**
 * A wrapper for PyObject which takes care of reference counting when
 * allocated on the stack. Use this instead of PyObject * to avoid reference
 * leaks.
 */
class Object
{
public:
    Object() :
        m_object(0)
    {
    }

    Object(PyObject *object) :
        m_object(object)
    {
    }

    ~Object()
    {
        Py_XDECREF(m_object);
    }

    inline operator PyObject *()
    {
        return m_object;
    }

    /**
     * Unborrow a reference.
     */
    inline Object &operator++()
    {
        Py_INCREF(m_object);
        return *this;
    }

    inline PyObject **operator&()
    {
        Py_XDECREF(m_object);
        return &m_object;
    }

    inline Object &operator=(PyObject *object)
    {
        if (object != m_object) {
            Py_XDECREF(m_object);
            m_object = object;
        }
        return *this;
    }

private:
    PyObject *m_object;
};

/// Convert a QString to a Python unicode object
PyObject *unicode(const QString &string);

/// Call a function, displaying a traceback in STDERR if it fails
bool call(PyObject *function, PyObject *arguments);
bool call(PyObject *function);

/// Append a QString to a list as a Python unicode object
void appendStringToList(PyObject *list, const QString &value);

/**
 * Print and save (see @ref lastTraceback()) the current traceback in a form
 * approximating what Python would print:
 *
 * Traceback (most recent call last):
 *   File "/home/shahhaqu/.kde/share/apps/kate/pate/pluginmgr.py", line 13, in <module>
 *     import kdeui
 * ImportError: No module named kdeui
 * Could not import pluginmgr.
 */
void traceback(const QString &description);

/**
 * Store the last traceback we handled using @ref traceback().
 */
const QString &lastTraceback(void);

/// Create a Python dictionary from a KConfigBase instance,
/// writing the string representation of the values
void updateDictionaryFromConfiguration(PyObject *dictionary, KConfigBase *config);

/// Write a Python dictionary to a configuration object, converting
/// objects to their string representation along the way
void updateConfigurationFromDictionary(KConfigBase *config, PyObject *dictionary);

    extern const char *PATE_ENGINE;

    /**
     * Call the named module's named entry point.
     */
    extern bool functionCall(const char *functionName,
                             const char *moduleName = PATE_ENGINE);

    /**
     * Delete the item from the named module's dictionary.
     */
    extern bool itemStringDel(const char *item,
                              const char *moduleName = PATE_ENGINE);

    /**
     * Get the item from the named module's dictionary.
     */
    extern PyObject *itemString(const char *item,
                                const char *moduleName = PATE_ENGINE);

    /**
     * Get the item from the given dictionary.
     */
    extern PyObject *itemString(const char *item, PyObject *dict);

    /**
     * Set the item in the named module's dictionary.
     */
    extern bool itemStringSet(const char *item, PyObject *value,
                              const char *moduleName = PATE_ENGINE);

    /**
     * Get the Actions defined by a module. The returned object is
     * [ { function, ( text, icon, shortcut, menu ) }... ] for each module
     * function decorated with @action.
     */
    extern PyObject *moduleActions(const char *moduleName);

    /**
     * Get the ConfigPages defined by a module. The returned object is
     * [ { function, callable, ( name, fullName, icon ) }... ] for each module
     * function decorated with @configPage.
     */
    extern PyObject *moduleConfigPages(const char *moduleName);

    /**
     * Get the named module's dictionary.
     */
    extern PyObject *moduleDict(const char *moduleName = PATE_ENGINE);

    /**
     * Get the help text defined by a module.
     */
    extern QString moduleHelp(const char *moduleName);

    /**
     * Import the named module.
     */
    extern PyObject *moduleImport(const char *moduleName);

    /**
     * A void * for an arbitrary Qt/KDE object that has been wrapped by SIP. Nifty.
     */
    extern void *objectUnwrap(PyObject *o);

    /**
     * A PyObject * for an arbitrary Qt/KDE object using SIP wrapping. Nifty.
     */
    extern PyObject *objectWrap(void *o, QString className);

}

} // namespace Py, namespace Pate

#endif
