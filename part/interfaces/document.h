/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KATE_DOCUMENT_INCLUDE_
#define _KATE_DOCUMENT_INCLUDE_

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/editinterface.h>
#include <ktexteditor/undointerface.h>
#include <ktexteditor/cursorinterface.h>
#include <ktexteditor/documentinfo.h>
#include <ktexteditor/selectioninterface.h>
#include <ktexteditor/selectioninterfaceext.h>
#include <ktexteditor/blockselectioninterface.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/highlightinginterface.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/configinterfaceextension.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/markinterfaceextension.h>
#include <ktexteditor/wordwrapinterface.h>
#include <ktexteditor/printinterface.h>
#include <ktexteditor/variableinterface.h>

#include <kaction.h>

class KCompletion;

/**
 * Kate namespace
 * All classes in this namespace must stay BC
 * during one major release series (e.g. 3.x, 4.x, ...)
 */
namespace Kate
{

class View;

class KATEPARTINTERFACES_EXPORT Cursor : public KTextEditor::Cursor
{
  public:
    Cursor () { ; };
    virtual ~Cursor () { ; };
};

class KATEPARTINTERFACES_EXPORT ConfigPage : public KTextEditor::ConfigPage
{
  Q_OBJECT

  public:
    ConfigPage ( QWidget *parent=0, const char *name=0 ) : KTextEditor::ConfigPage (parent, name) { ; };
    virtual ~ConfigPage () { ; };

  public slots:
    virtual void apply () { ; };
    virtual void reload () { ; };
    virtual void reset () {};
    virtual void defaults () {};

  protected slots:
    void slotChanged();
};

class KATEPARTINTERFACES_EXPORT ActionMenu : public KActionMenu
{
  Q_OBJECT

  public:
    ActionMenu ( const QString& text, QObject* parent = 0, const char* name = 0 )
      : KActionMenu(text, parent, name) { ; };
    virtual ~ActionMenu () { ; };

  public:
    virtual void updateMenu (class Document *) = 0;
};

/**
 * Kate Commands
 */
class KATEPARTINTERFACES_EXPORT Command
{
  public:
    Command () {};
    virtual ~Command () {};

  public:
    /**
     * Pure text start part of the commands which can be handled by this object
     * which means i.e. for s/sdl/sdf/g => s or for char:1212 => char
     */
    virtual QStringList cmds () = 0;

    /**
     * Execute this command for the given view and cmd string, return a bool
     * about success, msg for status
     */
    virtual bool exec (View *view, const QString &cmd, QString &msg) = 0;

    /**
     * Shows help for the given view and cmd string, return a bool
     * about success, msg for status
     */
    virtual bool help (View *view, const QString &cmd, QString &msg) = 0;
};

/**
 * Extension to the Command interface, allowing to interact with commands
 * during typing. This allows for completion and for example the isearch
 * plugin. If you develop a command that wants to complete or process text
 * as thu user types the arguments, or that has flags, you can have
 * your command inherit this class.
 */
class CommandExtension
{
  public:
    CommandExtension() {;}
    virtual ~CommandExtension() {;}

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
     * @param current the current flags string
     * @param list Add the strings here.
     */ //### this is yet to be tried
    virtual void flagCompletions( QStringList& /*list*/ ) {;}

    /**
     * @return a KCompletion object that will substitute the command line default
     * one while typing the first argument to the command. The text will be
     * added to the command seperated by one space character.
     *
     * Implement this method if your command can provide a completion object.
     *
     * @param cmdname The command name associated with this request.
     */
    virtual KCompletion *completionObject( const QString &/*cmdname*/, Kate::View */*view*/ ) { return 0L; }

    /**
     * @return wheather this command want to process text interactively given @p cmdname.
     * If true, the commands processText() method is called when the
     * text in the command line is changed.
     *
     * Reimplement this to return true, if you commands wants to process the
     * text as typed.
     *
     * @param cmdname the command name associated with this query.
     */
    virtual bool wantsToProcessText( const QString &/*cmdname*/) { return false; }

    /**
     * This is called by the commandline each time the argument text for the
     * command changes, if wantsToProcessText() returns true.
     * @param text The current command text typed by the user.
     */ // ### yet to be tested. The obvious candidate is isearch.
    virtual void processText( const QString &/*text*/ ) {;}
};

/** This interface provides access to the Kate Document class.
*/
class KATEPARTINTERFACES_EXPORT Document : public KTextEditor::Document, public KTextEditor::EditInterface,
                     public KTextEditor::UndoInterface, public KTextEditor::CursorInterface,
                     public KTextEditor::SelectionInterface, public KTextEditor::SearchInterface,
                     public KTextEditor::HighlightingInterface, public KTextEditor::BlockSelectionInterface,
                     public KTextEditor::ConfigInterface, public KTextEditor::MarkInterface,
                     public KTextEditor::PrintInterface, public KTextEditor::WordWrapInterface,
                     public KTextEditor::MarkInterfaceExtension,
                     public KTextEditor::SelectionInterfaceExt
{
  Q_OBJECT

  public:
    Document ();
    Document ( QObject* parent, const char* name );
    virtual ~Document ();

  /**
   * Commands handling
   */
  public:
    static bool registerCommand (Command *cmd);
    static bool unregisterCommand (Command *cmd);
    static Command *queryCommand (const QString &cmd);

  public:
    /**
     * deprecated for KDE 4.0, just does reloadFile, which will ask
     * the normal "do you want it really" questions
     */
    virtual void isModOnHD(bool =false) { ; };

    /**
     * Returns the document name.
     */
    virtual QString docName () { return 0L; };

    /**
     * Sets the document name.
     * deprecated for KDE 4.0, is done internally, calling it won't hurt
     * but changes nothing beside triggers signal
     */
    virtual void setDocName (QString ) { ; };

    virtual ActionMenu *hlActionMenu (const QString& , QObject* =0, const char* = 0) = 0;
    virtual ActionMenu *exportActionMenu (const QString& , QObject* =0, const char* = 0) = 0;

  public slots:
    // clear buffer/filename - update the views
    virtual void flush () { ; };

    /**
     * Reloads the current document from disk if possible
     */
    virtual void reloadFile() = 0;

    /**
     * Spellchecking
     */
    virtual void spellcheck() {};

    virtual void exportAs(const QString &) = 0;

    virtual void applyWordWrap () = 0;


  public:
    virtual void setWordWrap (bool ) = 0;
    virtual bool wordWrap () = 0;

    virtual void setWordWrapAt (unsigned int) = 0;
    virtual uint wordWrapAt () = 0;


    virtual void setEncoding (const QString &e) = 0;
    virtual QString encoding() const = 0;

  /** @deprecated */
  // FIXME: Remove when BIC allowed.
  public:
    virtual ConfigPage *colorConfigPage (QWidget *) = 0;
    virtual ConfigPage *fontConfigPage (QWidget *) = 0;
    virtual ConfigPage *indentConfigPage (QWidget *) = 0;
    virtual ConfigPage *selectConfigPage (QWidget *) = 0;
    virtual ConfigPage *editConfigPage (QWidget *) = 0;
    virtual ConfigPage *keysConfigPage (QWidget *) = 0;
    virtual ConfigPage *kSpellConfigPage (QWidget *) { return 0L; }
    virtual ConfigPage *hlConfigPage (QWidget *) = 0;

  public:
    virtual uint configFlags () = 0;
    virtual void setConfigFlags (uint flags) = 0;

    // Flags for katedocument config !
    enum ConfigFlags
    {
      cfAutoIndent= 0x1,
      cfBackspaceIndents= 0x2,
      cfWordWrap= 0x4,
      cfReplaceTabs= 0x8,
      cfRemoveSpaces = 0x10,
      cfWrapCursor= 0x20,
      cfAutoBrackets= 0x40,
      cfPersistent= 0x80,
      cfKeepSelection= 0x100,
      cfDelOnInput= 0x400,
      cfXorSelect= 0x800,
      cfOvr= 0x1000,
      cfMark= 0x2000,
      cfKeepIndentProfile= 0x8000,
      cfKeepExtraSpaces= 0x10000,
      cfTabIndents= 0x80000,
      cfShowTabs= 0x200000,
      cfSpaceIndent= 0x400000,
      cfSmartHome = 0x800000
    };

  signals:
    /* reason = 0 nothing, 1 dirty, 2 created, 3 deleted */
    void modifiedOnDisc (Kate::Document *doc, bool isModified, unsigned char reason);

  /*
   * there static methodes are usefull to turn on/off the dialogs
   * kate part shows up on open file errors + file changed warnings
   * open file errors default on, file changed warnings default off, better
   * for other apps beside kate app using the part
   */
  public:
    // default true
    static void setOpenErrorDialogsActivated (bool on);

    // default false
    static void setFileChangedDialogsActivated (bool on);

    static const QString &defaultEncoding ();

  protected:
    static bool s_openErrorDialogsActivated;
    static bool s_fileChangedDialogsActivated;

    static QString s_defaultEncoding;
};

/**
 * Extensions to the Document Interface
 * @since 3.3
 */
class KATEPARTINTERFACES_EXPORT DocumentExt
 : public KTextEditor::DocumentInfoInterface,
   public KTextEditor::VariableInterface
{
  public:
    DocumentExt ();
    virtual ~DocumentExt ();

  public:
    /**
     * Reasons why a document is modified on disk.
     */
    enum ModifiedOnDiskReason {
      Unmodified = 0, ///< Not modified
      Modified = 1,   ///< The file was modified by another program
      Created = 2,    ///< The file was created by another program
      Deleted = 3     ///< The file was deleted
    };

  public:
    /**
     * For client apps that want to deal with files modified on disk, it is
     * nessecary to reset this property.
     * @p reason is a ModifiedOnDiskReason.
     */
    virtual void setModifiedOnDisk( int reason ) = 0;

  /**
   * These stuff is implemented as slots in the real document
   */
  public:
    /**
     * Ask the user what to do, if the file is modified on disk.
     * The @p v argument is used to avoid asking again, when the
     * editor regains focus after the dialog is hidden.
     */
    virtual void slotModifiedOnDisk( View *v=0 ) = 0;
};

/**
 * query if given document is a Kate::Document
 * @param doc KTextEditor document
 * @return 0 if no success, else the Kate::Document
 */
KATEPARTINTERFACES_EXPORT Document *document (KTextEditor::Document *doc);

/**
 * query if given document is a Kate::DocumentExt
 * @param doc KTextEditor document
 * @return 0 if no success, else the Kate::DocumentExt
 */
KATEPARTINTERFACES_EXPORT DocumentExt *documentExt (KTextEditor::Document *doc);

KATEPARTINTERFACES_EXPORT Document *createDocument ( QObject *parent = 0, const char *name = 0 );

}

#endif
