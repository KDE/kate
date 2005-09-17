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
 * KTextEditor::Command
 * Aims to capsule a command for the editor command line
 */
class KTEXTEDITOR_EXPORT Command
{
  public:
    virtual ~Command () {}

  public:
    /**
     * Pure text start part of the commands which can be handled by this object
     * which means i.e. for s/sdl/sdf/g => s or for char:1212 => char
     */
    virtual const QStringList &cmds () = 0;

    /**
     * Execute this command for the given view and cmd string, return a bool
     * about success, msg for status
     */
    virtual bool exec (KTextEditor::View *view, const QString &cmd, QString &msg) = 0;

    /**
     * Shows help for the given view and cmd string, return a bool
     * about success, msg for status
     */
    virtual bool help (KTextEditor::View *view, const QString &cmd, QString &msg) = 0;
};

/**
 * Extension to the Command interface, allowing to interact with commands
 * during typing. This allows for completion and for example the isearch
 * plugin. If you develop a command that wants to complete or process text
 * as thu user types the arguments, or that has flags, you can have
 * your command inherit this class.
 */
class KTEXTEDITOR_EXPORT CommandExtension
{
  public:
    virtual ~CommandExtension() {}

    /**
     * Fill in a list of flags to complete from. Each flag is a single letter,
     * any following text in the string is taken to be a description of the
     * flag's meaning, and showed to the user as a hint.
     * Implement this method if your command has flags.
     *
     * This method is called each time the flag string in the typed command
     * is changed, so that the available flags can be adjusted. When completions
     * are displayed, existing flags are left out.
     *
     */ //### this is yet to be tried
    virtual void flagCompletions( QStringList&list ) = 0;

    /**
     * @return a KCompletion object that will substitute the command line default
     * one while typing the first argument to the command. The text will be
     * added to the command seperated by one space character.
     *
     * Implement this method if your command can provide a completion object.
     *
     * @param cmdname The command name associated with this request.
     * @param view    The view the command will work on.
     */
    virtual KCompletion *completionObject( const QString & cmdname, KTextEditor::View *view ) = 0;

    /**
     * @return whether this command wants to process text interactively given the @p cmdname.
     * If true, the command's processText() method is called when the
     * text in the command line is changed.
     *
     * Reimplement this to return true, if your commands wants to process the
     * text as typed.
     *
     * @param cmdname the command name associated with this query.
     */
    virtual bool wantsToProcessText( const QString &cmdname ) = 0;

    /**
     * This is called by the commandline each time the argument text for the
     * command changes, if wantsToProcessText() returns true.
     * @param view The current view
     * @param text The current command text typed by the user.
     */ // ### yet to be tested. The obvious candidate is isearch.
    virtual void processText( KTextEditor::View *view, const QString &text ) = 0;
};

/**
 * CommandInterface
 * This interfaces is aimed for the KTextEditor::Editor and provides
 * a way for applications to enhance the editor command line with own
 * commands
 */
class KTEXTEDITOR_EXPORT CommandInterface
{
  public:
    /**
     * virtual destructor
     */
    virtual ~CommandInterface () {}

  public:
    /**
     * register given command
     * this works global, for all documents
     * @param cmd command to register
     * @return success
     */
    virtual bool registerCommand (Command *cmd) = 0;

    /**
     * unregister given command
     * this works global, for all documents
     * @param cmd command to unregister
     * @return success
     */
    virtual bool unregisterCommand (Command *cmd) = 0;

    /**
     * query for command
     * @param cmd name of command to query for
     * @return found command or 0
     */
    virtual Command *queryCommand (const QString &cmd) = 0;


};

}

Q_DECLARE_INTERFACE(KTextEditor::CommandInterface, "org.kde.KTextEditor.CommandInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
