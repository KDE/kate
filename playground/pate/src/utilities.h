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


// terminal colours
#define TERMINAL_RED "\033[31m"
#define TERMINAL_CLEAR "\033[0m"

// save us some ruddy time when printing out QStrings with UTF-8
#define PQ(x) x.toUtf8().constData()

namespace Pate { namespace Py {

/// Convert a QString to a Python unicode object
PyObject *unicode(const QString &string);

/// Call a function, displaying a traceback in STDERR if it fails
bool call(PyObject *function, PyObject *arguments);
bool call(PyObject *function);

/// Append a QString to a list as a Python unicode object
void appendStringToList(PyObject *list, const QString &value);

/// Print a Python traceback to standard error when an error has occured,
/// giving a high-level description of what happened
void traceback(const QString &description);

/// Create a Python dictionary from a KConfigBase instance,
/// writing the string representation of the values
void updateDictionaryFromConfiguration(PyObject *dictionary, KConfigBase *config);

/// Write a Python dictionary to a configuration object, converting
/// objects to their string representation along the way
void updateConfigurationFromDictionary(KConfigBase *config, PyObject *dictionary);

}} // namespace Py, namespace Pate

#endif
