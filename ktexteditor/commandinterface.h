/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann (cullmann@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_COMMANDINTERFACE_H
#define KDELIBS_KTEXTEDITOR_COMMANDINTERFACE_H

#include <kdelibs_export.h>
#include <qobject.h>

class QStringList;
class KCompletion;

namespace KTextEditor
{

class Document;
class Editor;
class View;

/**
 * An Editor command line command.
 *
 * <b>Introduction</b>\n
 *
 * The Command class represents a command for the editor command line. A
 * command simply consists of a string, for example @e find. To register a
 * command use CommandInterface::registerCommand(). The Editor itself queries
 * the command for a list of accepted strings/commands by calling cmds().
 * If the command gets invoked the function exec() is called, i.e. you have
 * to implement the @e reaction in exec(). Whenever the user needs help for
 * a command help() is called.
 *
 * <b>Command Extensions</b>\n
 *
 * @todo document CommandExtension
 *
 * @see KTextEditor::CommandInterface(), KTextEditor::CommandExtension
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Command
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~Command () {}

  public:
    /**
     * Return a list of strings a command may begin with.
     * A string is the start part of a pure text which can be handled by this
     * command, i.e. for the command s/sdl/sdf/g the corresponding string is
     * simply @e s, and for char:1212 simply @e char.
     * @return list of supported commands
     */
    virtual const QStringList &cmds () = 0;

    /**
     * Execute the command for the given @p view and @p cmd string.
     * Return the success value and a @p msg for status, i.e. if you return
     * @e true, the @p msg is ignored.
     * @return @e true on success, otherwise @e false
     */
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg) = 0;

    /**
     * Shows help for the given @p view and @p cmd string.
     * Return the success value and a @p msg for status, i.e. if you return
     * @e true, the @p msg is ignored.
     * @return @e true on success, otherwise @e false
     */
    virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg) = 0;
};

/**
 * Extension interface for a Command.
 *
 * <b>Introduction</b>\n
 *
 * The CommandExtension extends the Command interface allowing to interact
 * with commands during typing. This allows for completion and for example
 * the isearch plugin. If you develop a command that wants to complete or
 * process text as the user types the arguments, or that has flags, you can
 * have your command inherit this class.
 *
 * If your commands supports flags return them by reimplementing
 * flagCompletions(). You can return your own KCompletion object if the
 * command has available completion data. If you want to interactively react
 * on changes return @e true in wantsToProcessText() for the given command
 * and reimplement processText().
 *
 * @see KTextEditor::CommandInterface(), KTextEditor::Command, KCompletion
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT CommandExtension
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~CommandExtension() {}

    /**
     * Fill in a @p list of flags to complete from. Each flag is a single
     * letter, any following text in the string is taken to be a description
     * of the flag's meaning, and showed to the user as a hint.
     * Implement this method if your command has flags.
     *
     * This method is called each time the flag string in the typed command
     * is changed, so that the available flags can be adjusted. When
     * completions are displayed, existing flags are left out.
     * @param list flag list
     */ //### this is yet to be tried
    virtual void flagCompletions( QStringList&list ) = 0;

    /**
     * Return a KCompletion object that will substitute the command line
     * default one while typing the first argument of the command @p cmdname.
     * The text will be added to the command seperated by one space character.
     *
     * Implement this method if your command can provide a completion object.
     *
     * @param cmdname the command name associated with this request.
     * @param view the view the command will work on
     * @return the completion object or NULL, if you do not support a
     *         completion object
     */
    virtual KCompletion *completionObject( const QString & cmdname, KTextEditor::View *view ) = 0;

    /**
     * Check, whether the command wants to process text interactively for the
     * given command with name @p cmdname.
     * If you return true, the command's processText() method is called
     * whenever the text in the command line changed.
     *
     * Reimplement this to return true, if your commands wants to process the
     * text while typing.
     *
     * @param cmdname the command name associated with this query.
     * @return @e true, if your command wants to process text interactively,
     *         otherwise @e false
     * @see processText()
     */
    virtual bool wantsToProcessText( const QString &cmdname ) = 0;

    /**
     * This is called by the command line each time the argument text for the
     * command changed, if wantsToProcessText() returns @e true.
     * @param view the current view
     * @param text the current command text typed by the user
     * @see wantsToProcessText()
     */ // ### yet to be tested. The obvious candidate is isearch.
    virtual void processText( KTextEditor::View *view, const QString &text ) = 0;
};

/**
 * Command extension interface for the Editor.
 *
 * <b>Introduction</b>\n
 *
 * The CommandInterface extends the Editor to support command line commands.
 * An application or a Plugin can register new commands by using
 * registerCommand(). To unregister a command call unregisterCommand(). To
 * check, whether a command with a given name exists use queryCommand().
 *
 * <b>Accessing the CommandInterface</b>\n
 *
 * The CommandInterface is supposed to be an extension interface for the
 * Editor, i.e. the Editor inherits the interface @e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * @code
 *   // editor is of type KTextEditor::Editor*
 *   KTextEditor::CommandInterface *iface =
 *       qobject_cast<KTextEditor::CommandInterface*>( editor );
 *
 *   if( iface ) {
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * @endcode
 *
 * @see KTextEditor::Editor, KTextEditor::Command,
 *      KTextEditor::CommandExtension
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT CommandInterface
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~CommandInterface () {}

  public:
    /**
     * Register a the new command @p cmd. The command will be registered for
     * all documents, i.e. every command is global.
     *
     * @param cmd command to register
     * @return @e true on success, otherwise @e false
     * @see unregisterCommand()
     */
    virtual bool registerCommand (Command *cmd) = 0;

    /**
     * Unregister the command @p cmd. The command will be unregistered for
     * all documents.
     *
     * @param cmd command to unregister
     * @return success
     * @see registerCommand()
     */
    virtual bool unregisterCommand (Command *cmd) = 0;

    /**
     * Query for the command @p cmd.
     * If the command @p cmd does not exist the return value is NULL.
     *
     * @param cmd name of command to query for
     * @return the found command or NULL if no such command exists
     */
    virtual Command *queryCommand (const QString &cmd) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::CommandInterface, "org.kde.KTextEditor.CommandInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
