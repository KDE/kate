
/// This file is part of the KDE libraries
/// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License as published by the Free Software Foundation; either
/// version 2 of the License, or (at your option) version 3.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
///
/// You should have received a copy of the GNU Library General Public License
/// along with this library; see the file COPYING.LIB.  If not, write to
/// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
/// Boston, MA 02110-1301, USA.

#ifndef KATE_SCRIPT_H
#define KATE_SCRIPT_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QStringList>

#include <QtScript/QScriptValue>
#include <QtScript/QScriptable>

class QScriptEngine;
class QScriptContext;

class KateDocument;
class KateView;

class KateScriptDocument;
class KateScriptView;

namespace Kate {
  enum ScriptType {
    /** The script is an indenter */
    IndentationScript,
    /** Don't know what kind of script this is */
    UnknownScript
  };

  /** Top-level script functions */
  namespace Script {
    QScriptValue debug(QScriptContext *context, QScriptEngine *engine);
  }
}

//BEGIN KateScriptInformation

/**
 * Access to the meta data stored at the top of a script.
 */
class KateScriptInformation {
  public:
    /** The name of the script. */
    QString name;
    /** A license (e.g LGPL) under which this is distributed */
    QString license;
    /** Author of this script in the form "John Smith <john@example.com>" */
    QString author;
    /** Script version */
    QString version;
    /** Kate version this script is targeted to */
    QString kateVersion;
    /**
     * The script type. One of:
     *   - KateScriptInformation::IndentationScript
     *   - ...
     */
    Kate::ScriptType type;
    /**
     * If this script is an indenter, then the indentLanguages member specifies
     * which languages this is an indenter for. The values must correspond with
     * the name of a programming language given in a highlighting file (e.g "TI Basic")
     */
    QStringList indentLanguages;
    /**
     * If this script is an indenter, this value controls the priority it will take
     * when an indenter for one of the supported languages is requested and multiple
     * indenters are found
     */
    int priority;
    /**
     * other meta information specified in the file that we don't single out
     */
    QHash<QString, QString> other;

    /**
     * basename of script
     */
    QString baseName;
};

//END

//BEGIN KateScript

/**
 * KateScript objects represent a script that can be executed and inspected.
 */
class KateScript {
  public:
    /**
     * Create a new script representation, passing a file @p url to it and a
     * KateScriptInformation instance. Loading of the script will happen
     * lazily
     */
    KateScript(const QString &url, const KateScriptInformation &information);
    ~KateScript();

    /** The script's URL */
    const QString &url() { return m_url; }

    /** Metadata for the script */
    const KateScriptInformation &information() { return m_information; }

    /**
     * Load the script. If loading is successful, returns true. Otherwise, returns
     * returns false and an error message will be set (see errorMessage()).
     * Note that you don't have to call this -- it is called as necessary by the
     * functions that require it.
     * Subsequent calls to load will return the value it returned the first time.
     */
    bool load();

    /**
     * set view for this script for the execution
     * will trigger load!
     */
    bool setView (KateView *view);

    /**
     * Get a QScriptValue for a global item in the script given its name, or an
     * invalid QScriptValue if no such global item exists.
     */
    QScriptValue global(const QString &name);

    /**
     * Return a function in the script of the given name, or an invalid QScriptValue
     * if no such function exists.
     */
    QScriptValue function(const QString &name);

    /** Return a context-specific error message */
    const QString &errorMessage() { return m_errorMessage; }

    /** Displays the backtrace when a script has errored out */
    void displayBacktrace(const QScriptValue &error, const QString &header = QString());

    /** Clears any uncaught exceptions in the script engine. */
    void clearExceptions();

  private:
    /** Add our custom functions to m_engine when it has been initialised */
    void initEngine();

    /** Whether or not there has been a call to load */
    bool m_loaded;
    /** Whether or not the script loaded successfully into memory */
    bool m_loadSuccessful;
    /** The script's URL */
    QString m_url;
    /** Information about the script */
    KateScriptInformation m_information;
    /** An error message set when an error occurs */
    QString m_errorMessage;

  protected:
    /** The Qt interpreter for this script */
    QScriptEngine *m_engine;

  private:
    /** document/view wrapper objects */
    KateScriptDocument *m_document;
    KateScriptView *m_view;
};

//END

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
