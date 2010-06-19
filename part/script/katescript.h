// This file is part of the KDE libraries
// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
// Copyright (C) 2009 Dominik Haumann <dhaumann kde org>
// Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>
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
    /** The script contains command line commands */
    CommandLineScript,
    /** Don't know what kind of script this is */
    UnknownScript
  };
}

//BEGIN KateScriptHeader

class KateScriptHeader
{
  public:
    KateScriptHeader() : m_revision(0), m_scriptType(Kate::UnknownScript)
    {}
    virtual ~KateScriptHeader()
    {}

    inline void setLicense(const QString& license)
    { m_license = license; }
    inline const QString& license() const
    { return m_license; }

    inline void setAuthor(const QString& author)
    { m_author = author; }
    inline const QString& author() const
    { return m_author; }

    inline void setRevision(int revision)
    { m_revision = revision; }
    inline int revision() const
    { return m_revision; }

    inline void setKateVersion(const QString& kateVersion)
    { m_kateVersion = kateVersion; }
    inline const QString& kateVersion() const
    { return m_kateVersion; }

    inline void setCatalog(const QString& catalog)
    { m_i18nCatalog = catalog; }
    inline const QString& catalog() const
    { return m_i18nCatalog; }

    inline void setScriptType(Kate::ScriptType scriptType)
    { m_scriptType = scriptType; }
    inline Kate::ScriptType scriptType() const
    { return m_scriptType; }

  private:
    QString m_license;        ///< the script's license, e.g. LGPL
    QString m_author;         ///< the script author, e.g. "John Smith <john@example.com>"
    int m_revision;           ///< script revision, a simple number, e.g. 1, 2, 3, ...
    QString m_kateVersion;    ///< required katepart version
    QString m_i18nCatalog;        ///< i18n catalog
    Kate::ScriptType m_scriptType;  ///< the script type
};
//END

//BEGIN KateScript

/**
 * KateScript objects represent a script that can be executed and inspected.
 */
class KateScript {
  public:

    enum InputType {
      InputURL,
      InputSCRIPT
    };

    /**
     * Create a new script representation, passing either a file or the script
     * content @p urlOrScript to it.
     * In case of a file, loading of the script will happen lazily.
     */
    KateScript(const QString &urlOrScript, enum InputType inputType = InputURL);
    virtual ~KateScript();

    /** The script's URL */
    const QString &url() { return m_url; }

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

    /** set the general header after construction of the script */
    void setGeneralHeader(const KateScriptHeader& generalHeader);
    /** Return the general header */
    KateScriptHeader& generalHeader();

  protected:
    /** Checks for exception and gives feedback on the console. */
    bool hasException(const QScriptValue& object, const QString& file);

    /** read complete file contents */
    static bool readFile(const QString& sourceUrl, QString& sourceCode);

  private:
    /** init API, can fail on error in api files */
    bool initApi ();

    /** Add our custom functions to m_engine when it has been initialised */
    void initEngine();

    /** Whether or not there has been a call to load */
    bool m_loaded;
    /** Whether or not the script loaded successfully into memory */
    bool m_loadSuccessful;
    /** The script's URL */
    QString m_url;
    /** An error message set when an error occurs */
    QString m_errorMessage;

  protected:
    /** The Qt interpreter for this script */
    QScriptEngine *m_engine;

  private:
    /** general header data */
    KateScriptHeader m_generalHeader;

    /** document/view wrapper objects */
    KateScriptDocument *m_document;
    KateScriptView *m_view;

  public:
    static void reloadScriptingApi();

  private:
    /** True, if the katepartapi.js file is already loaded, otherwise false */
    static bool s_scriptingApiLoaded;

    /** if input is script or url**/
    enum InputType m_inputType;
    QString m_script;
};

//END

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
