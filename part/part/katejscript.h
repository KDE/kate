/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhaumann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_JSCRIPT_H
#define KATE_JSCRIPT_H

#include <ktexteditor/commandinterface.h>
#include <ktexteditor/cursor.h>

#include <kdebug.h>

#include <QHash>
#include <QList>

/*
 * Some common stuff
 */
class KateDocument;
class KateView;
class KateJSGlobal;
class KateJSDocument;
class KateJSView;
class KateJSIndenter;
class KateDocCursor;
class KateIndentJScript;

/*
 * Cool, this is all we need here
 */
namespace KJS {
  class JSObject;
  class JSValue;
  class Interpreter;
  class ExecState;
  class Identifier;
  struct HashTable;
  class List;
}

/**
 * Helper class which includes a filename and the header information. The header
 * information is available as key/values (hash).
 */
class KateJScriptHeader
{
  public:
    QString url;
    QHash<QString, QString> pairs;
};

typedef QVector<KateJScriptHeader> KateJScriptHeaderVector;

/**
 * Kate jscript helper functions.
 */
class KateJScriptHelpers
{
  public:
    /**
     * Get a list of all files matching the wildcard \p resourceDir. The file
     * \p rcFile is used for caching. The returned vector contains a list of all
     * files as KateJScriptHeader. Every item has a hash, that includes all
     * key/value pairs. Only the given \p keys are read from the script header.
     */
    static QVector <KateJScriptHeader> findScripts(const QString& rcFile,
                                                   const QString& resourceDir,
                                                   const QStringList &keys);
  private:
    /** helper function used by findScripts() */
    static bool parseScriptHeader(const QString& filename, KateJScriptHeader& scriptHeader);

  private:
    explicit KateJScriptHelpers() {}
    explicit KateJScriptHelpers(const KateJScriptHelpers&) {}
};

/**
 * Whole Kate Part scripting in one classs
 * Allow subclassing to allow specialized scripting engine for indenters.
 * Subclasses *have to* call this constructur in their constructor.
 */
class KateJSInterpreterContext
{
  public:
    /** generate new global interpreter for part scripting */
    KateJSInterpreterContext(const QString &filename = "");

    /** be destructive */
    virtual ~KateJSInterpreterContext ();

    /**
     * create a js-wrapper for a given KateDocument @p doc. @p exec is the
     * KJS::ExecState. The return value is a instance of KateJSDocument.
     */
    KJS::JSObject *wrapDocument (KJS::ExecState *exec, KateDocument *doc);
    /**
     * create a js-wrapper for a given KateView @p view. @p exec is the
     * KJS::ExecState. The return value is a instance of KateJSView.
     */
    KJS::JSObject *wrapView (KJS::ExecState *exec, KateView *view);

    /**
     * execute given script
     * the script will get the doc and view exposed via document and view object
     * in global scope
     * @param view view to expose
     * @param script source code of script to execute
     * @param errorMsg error to return if no success
     * @return success or not?
     */
    bool evalSource(KateView *view, const QString &script, QString &errorMsg);
    bool evalFile(KateView* view, const QString& url, QString &errorMsg);

    KJS::Interpreter *interpreter () { return m_interpreter; }

    /**
     * Call @p function for the object @p lookupObj. The @p view is the KateView
     * the script operates on. @p parameter contains the list of parameters.
     * If the return value is 0, @p error contains an error message.
     *
     * Note: Use this function after a script is loaded.
     */
    KJS::JSValue* callFunction(KateView* view, KJS::JSObject* lookupObj,
                               const KJS::Identifier& function,
                               KJS::List parameter, QString& error);

  protected:
    /** global object of interpreter */
    KJS::JSObject *m_global;

    /** js interpreter */
    KJS::Interpreter *m_interpreter;

    /** object for document */
    KJS::JSObject *m_document;

    /** object for view */
    KJS::JSObject *m_view;
};


/**
 * manager for all js scripts the part knows about
 * this manager will act as a cache for already parsed scripts, too
 */
class KateJScriptManager : public KTextEditor::Command
{
  public:
    /**
     * Internal used Script Representation
     */
    class Script
    {
      public:
        Script () : interpreter (0), persistent (false) {}

        /** complete path to script */
        QString filename;

        QString name;              ///< script's name, like 'C++' or 'Python'
        QString author;            ///< FirstName LastName <email-address>
        QString license;           ///< license, like GPL, LPGL, Artistic, ...
        QString version;           ///< scripts's version
        QString kateVersion;       ///< minimum required kate version

        /** type of this script */
        QString type;

        /** interpreter for this script.... */
        KateJSInterpreterContext *interpreter;

        /** should this interpreter persist? */
        bool persistent;
    };

  public:
    KateJScriptManager ();
    ~KateJScriptManager ();

  private:
    /**
     * go, search our scripts
     * @param force force cache updating?
     */
    void collectScripts(bool force = false);

  public:
    /**
     * get interpreter for given script basename
     * interpreter will be constructed on demand...
     * @param basename basename of the script...
     * @param persistent should this interpreter live forever?
     * @return interpreter, if script for this basename exists...
     */
    KateJSInterpreterContext *interpreter (const QString &name, bool persistent = false);

  //
  // Here we deal with the KTextEditor::Command stuff
  //
  public:
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec( KTextEditor::View *view, const QString &cmd, QString &errorMsg );

    /**
     * get help for a command
     * @param view view to use
     * @param cmd cmd name
     * @param msg help message
     * @return help available or not
     */
    bool help( KTextEditor::View *view, const QString &cmd, QString &msg );

    /**
     * supported commands as prefixes
     * @return prefix list
     */
    const QStringList &cmds();

  public:
    KateIndentJScript *indentationScript (const QString &scriptname) { return m_indentationScripts.value(scriptname); }

    int indentationScripts () { return m_indentationScriptsList.size(); }
    KateIndentJScript *indentationScriptByIndex (int index) { return m_indentationScriptsList[index]; }

  private:
    /**
     * hash of all scripts
     * hashs basename -> script...
     */
    QHash<QString, KateJScriptManager::Script*> m_scripts;

    /**
     * hash of types to basenames...
     */
    QHash<QString, QStringList> m_types;

    /**
     * hash of all existing indenter scripts
     * hashs basename -> script
     */
    QHash<QString, KateIndentJScript*> m_indentationScripts;

    /**
     * list of all indentation scripts
     */
    QList<KateIndentJScript*> m_indentationScriptsList;
};

/**
 * JS indentation class representing one indenter.
 */
class KateIndentJScript
{
  public:
    /** create new indenter object. parameters are self-explaining. Beat dominik if you disagree. */
    KateIndentJScript(const QString& basename, KateJScriptManager::Script *info);
    ~KateIndentJScript();

    /** get the supported characters the indenter wants to process */
    const QString &triggerCharacters(KateView* view);

    /** get new indent level. See KateJSIndentInterpreter::indent() for further information */
    int indent(KateView* view, const KTextEditor::Cursor& position,
               QChar typedChar, int indentWidth);

  public:
    inline const QString& basename() const { return m_basename; }

    KateJScriptManager::Script *info () { return m_info; }

  protected:
    /** loads the interpreter */
    void loadInterpreter(KateView* view);
    /** free the interpreter */
    void unloadInterpreter();

  private:
    KateJScriptManager::Script *m_info;

    KateJSInterpreterContext *m_script; ///< interpreter object

    /** additional js indenter object */
    KateJSIndenter *m_indenter;

    QString m_basename;          ///< filename without extension (use-case: command line: set-indent-mode c)

    QString m_triggerCharacters; ///< trigger characters the indenter supports
    bool m_triggerCharactersSet; ///< helper. if true, trigger characters are already read
};

class KateJSExceptionTranslator
{
  public:
    explicit KateJSExceptionTranslator(KJS::ExecState *exec,
                                       const KJS::HashTable& hashTable,
                                       int id,
                                       const KJS::List& args);
    ~KateJSExceptionTranslator();

    bool invalidArgs(int min, int max = -1);

  private:
    KJS::ExecState *m_exec;
    const KJS::HashTable& m_hashTable;
    int m_id;
    int m_args;
    bool m_trigger;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
