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

/*
 * Cool, this is all we need here
 */
namespace KJS {
  class JSObject;
  class JSValue;
  class Interpreter;
  class ExecState;
  class Identifier;
  class HashTable;
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
 * Allow subclassing to allow specialized scripting engine for indenters
 */
class KateJSInterpreterContext
{
  public:
    /** generate new global interpreter for part scripting */
    KateJSInterpreterContext();

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

  protected:
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
 * Interpreter class for indentation.
 */
class KateJSIndentInterpreter : public KateJSInterpreterContext
{
  public:
    /**
     * Constructor. Creates a new interpreter context and loads the script
     * @p url. @p view is the current active KateView.
     */
    KateJSIndentInterpreter(KateView* view, const QString& url);
    /** destructor. The interpreter is already deleted in KateJSInterpreterContext */
    virtual ~KateJSIndentInterpreter();

    /** Get a list of trigger characters the indetner can process. */
    QString triggerCharacters();

    /**
     * Get the indentation level for the line in @p position. @p indentWidth is
     * the indentation with (per indent level) in spaces. @p typedChar contains
     * one of the following characters:
     * - the typed char (includes '\n')
     * - empty ("") for the action "Tools > Align"
     */
    int indent(KateView* view,
               const KTextEditor::Cursor& position,
               QChar typedChar,
               int indentWidth);

  protected:
    /** additional js indenter object */
    KateJSIndenter *m_indenter;
};

/**
 * JS indentation class representing one indenter.
 */
class KateIndentJScript
{
  public:
    /** create new indenter object. parameters are self-explaining. Beat dominik if you disagree. */
    KateIndentJScript(const QString& basename, const QString& url, const QString& name,
                      const QString& license, const QString& author, const QString& version,
                      const QString& kateVersion);
    ~KateIndentJScript();

    /** get the supported characters the indenter wants to process */
    const QString &triggerCharacters(KateView* view);

    /** get new indent level. See KateJSIndentInterpreter::indent() for further information */
    int indent(KateView* view, const KTextEditor::Cursor& position,
               QChar typedChar, int indentWidth);

  public:
    inline const QString& basename() const { return m_basename; }
    inline const QString& url() const { return m_url; }
    inline const QString& name() const { return m_name; }
    inline const QString& license() const { return m_license; }
    inline const QString& author() const { return m_author; }
    inline const QString& version() const { return m_version; }
    inline const QString& kateVersion() const { return m_kateVersion; }

  protected:
    /** loads the interpreter */
    void loadInterpreter(KateView* view);
    /** free the interpreter */
    void unloadInterpreter();

  private:
    KateJSIndentInterpreter *m_script; ///< interpreter object

    QString m_basename;          ///< filename without extension (use-case: command line: set-indent-mode c)
    QString m_url;               ///< full qualified location to the file
    QString m_name;              ///< indenter's name, like 'C++' or 'Python'
    QString m_author;            ///< FirstName LastName <email-address>
    QString m_license;           ///< license, like GPL, LPGL, Artistic, ...
    QString m_version;           ///< indenter version
    QString m_kateVersion;       ///< minimum required kate version

    QString m_triggerCharacters; ///< trigger characters the indenter supports
    bool m_triggerCharactersSet; ///< helper. if true, trigger characters are already read
};


class KateJScriptManager : public KTextEditor::Command
{
  private:
    /**
     * Internal used Script Representation
     */
    class Script
    {
      public:
        /** command name, as used for command line and more */
        QString command;

        /** translated description, can be used for e.g. status bars */
        QString help;

        /** filename of the script */
        QString url;
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

    KateJSInterpreterContext *m_jscript;

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

  private:
    /**
     * we need to know somewhere which scripts are around
     */
    QHash<QString, KateJScriptManager::Script*> m_scripts;
};

class KateIndentJScriptManager
{
  public:
    KateIndentJScriptManager ();
    virtual ~KateIndentJScriptManager ();
    virtual KateIndentJScript *script(const QString &scriptname) { return m_scripts.value(scriptname); }

    int scripts () { return m_scriptsList.size(); }
    KateIndentJScript *scriptByIndex (int index) { return m_scriptsList[index]; }

  private:
    /**
     * go, search our scripts
     * @param force force cache updating?
     */
    void collectScripts(bool force = false);

  private:
    /**
     * hash of all existing indenter scripts
     */
    QHash<QString, KateIndentJScript*> m_scripts;
    QList<KateIndentJScript*> m_scriptsList;
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
