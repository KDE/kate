/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>

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

#ifndef __kate_jscript_h__
#define __kate_jscript_h__

#include <ktexteditor/commandinterface.h>

#include <kdebug.h>

#include <QHash>
#include <QList>

/*
 * Some common stuff
 */
class KateDocument;
class KateView;
class QString;
class KateJSDocument;
class KateJSView;
class KateJSIndenter;
class KateDocCursor;

/*
 * Cool, this is all we need here
 */
namespace KJS {
  class JSObject;
  class Interpreter;
  class ExecState;
  class HashTable;
  class List;
}

/**
 * Whole Kate Part scripting in one classs
 * Allow subclassing to allow specialized scripting engine for indenters
 */
class KateJScriptInterpreterContext
{
  public:
    /**
     * generate new global interpreter for part scripting
     */
    KateJScriptInterpreterContext ();

    /**
     * be destructive
     */
    virtual ~KateJScriptInterpreterContext ();

    /**
     * creates a JS wrapper object for given KateDocument
     * @param exec execution state, to find out interpreter to use
     * @param doc document object to wrap
     * @return new js wrapper object
     */
    KJS::JSObject *wrapDocument (KJS::ExecState *exec, KateDocument *doc);

    /**
     * creates a JS wrapper object for given KateView
     * @param exec execution state, to find out interpreter to use
     * @param view view object to wrap
     * @return new js wrapper object
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
    bool execute (KateView *view, const QString &script, QString &errorMsg);

  protected:
    /**
     * global object of interpreter
     */
    KJS::JSObject *m_global;

    /**
     * js interpreter
     */
    KJS::Interpreter *m_interpreter;

    /**
     * object for document
     */
    KJS::JSObject *m_document;

    /**
     * object for view
     */
    KJS::JSObject *m_view;
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
        /**
         * get desktop filename
         * @return desktop filename
         */
        inline QString desktopFilename () { return filename.left(filename.length()-2).append ("desktop"); }

      public:
        /** command name, as used for command line and more */
        QString command;

        /** translated gui name, can be used for e.g. menu entries */
        QString name;

        /** translated description, can be used for e.g. status bars */
        QString description;

        /** filename of the script */
        QString filename;

        /** has it a desktop file? */
        bool desktopFileExists;
    };

  public:
    KateJScriptManager ();
    ~KateJScriptManager ();

  private:
    /**
     * go, search our scripts
     * @param force force cache updating?
     */
    void collectScripts (bool force = false);

    KateJScriptInterpreterContext *m_jscript;

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

    /**
     * Get the \p cmd's readable name that can be put into a menu for
     * example. The string should be translated.
     * \param cmd command line string to get the name for
     */
    QString name (const QString& cmd) const;

    /**
     * Get the \p cmd's description that can be put into a status bar for
     * example. The string should be translated.
     * \param cmd command line string to get the description for
     */
    QString description (const QString& cmd) const;

    /**
     * Get the \p cmd's category under which the command can be put into a menu
     * for example. The returned string should be translated.
     * \param cmd command line string to get the description for
     */
    QString category (const QString& cmd) const;

  private:
    /**
     * we need to know somewhere which scripts are around
     */
    QHash<QString, KateJScriptManager::Script*> m_scripts;
};

class KateIndentJScriptManager;

class KateIndentJScript {
  public:
    KateIndentJScript(KateIndentJScriptManager *manager, const QString& internalName,
        const QString &filePath, const QString &niceName,
        const QString &license, const QString &author,
        int version, double kateVersion);
    ~KateIndentJScript();

    bool processChar( KateView *view, QChar c, QString &errorMsg );
    bool processNewline( KateView *view, const KateDocCursor &begin,
                         bool needcontinue, QString &errorMsg );
    bool canProcessNewLine();
    bool canProcessLine();
    bool canProcessIndent();
    bool processLine( KateView *view, const KateDocCursor &line, QString &errorMsg );
    bool processSection( KateView *view, const KateDocCursor &begin,
                         const KateDocCursor &end, QString &errorMsg );
    bool processIndent( KateView *view, uint line, int levels, QString &errorMsg );

  public:
    const QString& internalName();
    const QString& filePath();
    const QString& niceName();
    const QString& license();
    const QString& author();
    int version();
    double kateVersion();
  protected:
    QString filePath() const {return m_filePath;}
  private:
    KateIndentJScriptManager *m_manager;
    QString m_internalName;
    QString m_filePath;
    QString m_niceName;
    QString m_license;
    QString m_author;
    int m_version;
    double m_kateVersion;
  private:
    KateJSView *m_viewWrapper;
    KateJSDocument *m_docWrapper;
    KJS::JSObject *m_indenter;
    KJS::Interpreter *m_interpreter;
    bool setupInterpreter(QString &errorMsg);
    void deleteInterpreter();
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
    void collectScripts (bool force = false);
    bool parseScriptHeader(const QString &filePath, QString &niceName,
                           QString &license, QString &author,
                           int &version, double &kateversion);

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
