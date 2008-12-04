/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann (cullmann@kde.org)
   Copyright (C) 2005-2006 Dominik Haumann (dhdev@gmx.de)
   Copyright (C) 2008 Erlend Hamberg (ehamberg@gmail.com)

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_COMMANDINTERFACE_H
#define KDELIBS_KTEXTEDITOR_COMMANDINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/range.h>
#include <QtCore/QObject>

class QStringList;
class KCompletion;

namespace KTextEditor
{

class Editor;
class View;

/**
 * \brief An Editor command line command.
 *
 * \section cmd_intro Introduction
 *
 * The Command class represents a command for the editor command line. A
 * command simply consists of a string, for example \e find. To register a
 * command use CommandInterface::registerCommand(). The Editor itself queries
 * the command for a list of accepted strings/commands by calling cmds().
 * If the command gets invoked the function exec() is called, i.e. you have
 * to implement the \e reaction in exec(). Whenever the user needs help for
 * a command help() is called.
 *
 * \section cmd_information Command Information
 * To provide reasonable information about a specific command there are the
 * following accessor functions for a given command string:
 *  - name() returns a label
 *  - description() returns a descriptive text
 *  - category() returns a category into which the command fits.
 *
 * These getters allow KTextEditor implementations to plug commands into menus
 * and toolbars, so that a user can assign shortcuts.
 *
 * \section cmd_extension Command Extensions
 *
 * If your command needs to interactively react on changes while the user is
 * typing text - look at the \e ifind command in Kate for example - you have
 * to additionally derive your command from the class CommandExtension. The
 * command extension provides methods to give help on \e flags or add a
 * KCompletion object and process the typed text interactively. Besides that
 * the class RangeCommand enables you to support ranges so that you can apply
 * commands on regions of text.
 *
 * \see KTextEditor::CommandInterface, KTextEditor::CommandExtension,
 *      KTextEditor::RangeCommand
 * \author Christoph Cullmann \<cullmann@kde.org\>
 * \note KDE5: derive from QObject, so qobject_cast works for extension interfaces.
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
     * simply \e s, and for char:1212 simply \e char.
     * \return list of supported commands
     */
    virtual const QStringList &cmds () = 0;

    /**
     * Execute the command for the given \p view and \p cmd string.
     * Return the success value and a \p msg for status. As example we
     * consider a replace command. The replace command would return the number
     * of replaced strings as \p msg, like "16 replacements made." If an error
     * occurred in the usage it would return \e false and set the \p msg to
     * something like "missing argument." or such.
     *
     * \return \e true on success, otherwise \e false
     */
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg) = 0;

    /**
     * Shows help for the given \p view and \p cmd string.
     * If your command has a help text for \p cmd you have to return \e true
     * and set the \p msg to a meaningful text. The help text is embedded by
     * the Editor in a Qt::RichText enabled widget, e.g. a QToolTip.
     * \return \e true if your command has a help text, otherwise \e false
     */
    virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg) = 0;
};

/**
 * \brief Extension interface for a Command.
 *
 * \ingroup kte_group_command_extensions
 *
 * \section cmdext_intro Introduction
 *
 * The CommandExtension extends the Command interface allowing to interact
 * with commands during typing. This allows for completion and for example
 * the isearch plugin. If you develop a command that wants to complete or
 * process text as the user types the arguments, or that has flags, you can
 * have your command inherit this class.
 *
 * If your command supports flags return them by reimplementing
 * flagCompletions(). You can return your own KCompletion object if the
 * command has available completion data. If you want to interactively react
 * on changes return \e true in wantsToProcessText() for the given command
 * and reimplement processText().
 *
 * \see KTextEditor::CommandInterface, KTextEditor::Command, KCompletion
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT CommandExtension
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~CommandExtension() {}

    /**
     * Fill in a \p list of flags to complete from. Each flag is a single
     * letter, any following text in the string is taken to be a description
     * of the flag's meaning, and showed to the user as a hint.
     * Implement this method if your command has flags.
     *
     * This method is called each time the flag string in the typed command
     * is changed, so that the available flags can be adjusted. When
     * completions are displayed, existing flags are left out.
     * \param list flag list
     */ //### this is yet to be tried
    virtual void flagCompletions( QStringList&list ) = 0;

    /**
     * Return a KCompletion object that will substitute the command line
     * default one while typing the first argument of the command \p cmdname.
     * The text will be added to the command separated by one space character.
     *
     * Implement this method if your command can provide a completion object.
     *
     * \param view the view the command will work on
     * \param cmdname the command name associated with this request.
     * \return the completion object or NULL, if you do not support a
     *         completion object
     */
    virtual KCompletion *completionObject( KTextEditor::View *view,
                                           const QString & cmdname ) = 0;

    /**
     * Check, whether the command wants to process text interactively for the
     * given command with name \p cmdname.
     * If you return true, the command's processText() method is called
     * whenever the text in the command line changed.
     *
     * Reimplement this to return true, if your commands wants to process the
     * text while typing.
     *
     * \param cmdname the command name associated with this query.
     * \return \e true, if your command wants to process text interactively,
     *         otherwise \e false
     * \see processText()
     */
    virtual bool wantsToProcessText( const QString &cmdname ) = 0;

    /**
     * This is called by the command line each time the argument text for the
     * command changed, if wantsToProcessText() returns \e true.
     * \param view the current view
     * \param text the current command text typed by the user
     * \see wantsToProcessText()
     */ // ### yet to be tested. The obvious candidate is isearch.
    virtual void processText( KTextEditor::View *view, const QString &text ) = 0;
};

/**
 * \brief Command extension interface for the Editor.
 *
 * \ingroup kte_group_editor_extensions
 *
 * \section cmdiface_intro Introduction
 *
 * The CommandInterface extends the Editor to support command line commands.
 * An application or a Plugin can register new commands by using
 * registerCommand(). To unregister a command call unregisterCommand(). To
 * check, whether a command with a given name exists use queryCommand().
 *
 * \section cmdiface_access Accessing the CommandInterface
 *
 * The CommandInterface is supposed to be an extension interface for the
 * Editor, i.e. the Editor inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // editor is of type KTextEditor::Editor*
 * KTextEditor::CommandInterface *iface =
 *     qobject_cast<KTextEditor::CommandInterface*>( editor );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Editor, KTextEditor::Command,
 *      KTextEditor::CommandExtension
 * \author Christoph Cullmann \<cullmann@kde.org\>
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
     * Register a the new command \p cmd. The command will be registered for
     * all documents, i.e. every command is global.
     *
     * \param cmd command to register
     * \return \e true on success, otherwise \e false
     * \see unregisterCommand()
     */
    virtual bool registerCommand (Command *cmd) = 0;

    /**
     * Unregister the command \p cmd. The command will be unregistered for
     * all documents.
     *
     * \param cmd command to unregister
     * \return \e true on success, otherwise \e false
     * \see registerCommand()
     */
    virtual bool unregisterCommand (Command *cmd) = 0;

    /**
     * Query for the command \p cmd.
     * If the command \p cmd does not exist the return value is NULL.
     *
     * \param cmd name of command to query for
     * \return the found command or NULL if no such command exists
     */
    virtual Command *queryCommand (const QString &cmd) const = 0;

    /**
     * Get a list of all registered commands.
     * \return list of all commands
     * \see queryCommand(), commandList()
     */
    virtual QList<Command*> commands() const = 0;

    /**
     * Get a list of available command line strings.
     * \return command line strings
     * \see commands()
     */
    virtual QStringList commandList() const = 0;
};

/**
 * \brief Extension interface for a Command making the exec method take a line
 * range
 *
 * \ingroup kte_group_command_extensions
 *
 * \section cmdext_intro Introduction
 *
 * The RangeCommand extension extends the Command interface by making it
 * possible to send a range to a command indicating that it should only do its
 * work on those lines.
 *
 * The method supportsRange() takes a QString reference and should return true
 * if the given command name supports a range and false if not.
 *
 * \see KTextEditor::CommandInterface, KTextEditor::Command, KTextEditor::Range
 * \author Erlend Hamberg \<ehamberg@gmail.com\>
 * \since 4.2
 * \note KDE5: merge with KTextEditor::Command?
 */
class KTEXTEDITOR_EXPORT RangeCommand
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~RangeCommand() {}

    /**
     * Execute the command for the given \p range on the given \p view and \p
     * cmd string.  Return the success value and a \p msg for status.
     *
     * \return \e true on success, otherwise \e false
     */
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg,
        const KTextEditor::Range &range) = 0;

    /**
     * Find out if a given command can act on a range. This is used for checking
     * if a command should be called when the user also gave a range or if an
     * error should be raised.
     *
     * \return \e true if command supports acting on a range of lines, false if
     * not
     */
    virtual bool supportsRange (const QString &cmd) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::CommandInterface, "org.kde.KTextEditor.CommandInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
