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

#ifndef _KATE_VIEW_INCLUDE_
#define _KATE_VIEW_INCLUDE_

#include <qpopupmenu.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/clipboardinterface.h>
#include <ktexteditor/popupmenuinterface.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/dynwordwrapinterface.h>

class KConfig;

namespace Kate
{

class Document;

/**
  The Kate::View text editor interface.
  @author Cullmann Christoph, modified by rokrau (6/21/01)
*/
class View : public KTextEditor::View, public KTextEditor::ClipboardInterface,
              public KTextEditor::PopupMenuInterface, public KTextEditor::ViewCursorInterface,
              public KTextEditor::CodeCompletionInterface, public KTextEditor::DynWordWrapInterface
{
  Q_OBJECT

  public:
    /**
     Return values for "save" related commands.
    */
    enum saveResult { SAVE_OK, SAVE_CANCEL, SAVE_RETRY, SAVE_ERROR };
    /**
     Constructor (should much rather take a reference to the document).
    */
    View ( KTextEditor::Document *, QWidget *, const char *name = 0 );
    /**
     Destructor, you need a destructor if Scott Meyers says so.
    */
    virtual ~View ();
    /**
     Set editor mode
    */
    virtual bool isOverwriteMode() const  { return false; };
    /**
     Get editor mode
    */
    virtual void setOverwriteMode( bool ) { ; };
    /**
      Gets the text line where the cursor is on
    */
    virtual QString currentTextLine() { return 0L; };
    /**
      Gets the word where the cursor is on
    */
    virtual QString currentWord() { return 0L; };
    /**
      Gets the word at position x, y. Can be used to find
      the word under the mouse cursor
    */
    virtual QString word(int , int ) { return 0L; };
    /**
      Insert text at the current cursor position.
      The parameter @param mark is unused.
    */
    virtual void insertText(const QString & ) { ; };
    /**
      Works exactly like closeURL() of KParts::ReadWritePart
    */
    virtual bool canDiscard() { return false; };
    
  public:
    virtual int tabWidth() = 0;
    virtual void setTabWidth(int) = 0;
    virtual void setEncoding (QString e) = 0;

    /**
      Returns true if this editor is the only owner of its document
    */
    virtual bool isLastView() = 0;

  public slots:
    /**
     Flushes the document of the text widget. The user is given
     a chance to save the current document if the current document has
     been modified.
    */
    virtual void flush () { ; };
    /**
      Saves the file under the current file name. If the current file
      name is Untitled, as it is after a call to newFile(), this routine will
      call saveAs().
    */
    virtual saveResult save() { return SAVE_CANCEL; };
    /**
      Allows the user to save the file under a new name.
    */
    virtual saveResult saveAs() { return SAVE_CANCEL; };
    /**
      Moves the current line or the selection one position to the right.
    */
    virtual void indent() { ; };
    /**
      Moves the current line or the selection one position to the left.
    */
    virtual void unIndent() { ; };
    /**
      Optimizes the selected indentation, replacing tabs and spaces as needed.
    */
    virtual void cleanIndent() { ; };
    /**
      Comments out current line.
    */
    virtual void comment() { ; };
    /**
      Removes comment signs in the current line.
    */
    virtual void uncomment() { ; };
    /**
      Some simply key commands.
    */
    virtual void keyReturn () { ; };
    virtual void keyDelete () { ; };
    virtual void backspace () { ; };
    virtual void killLine () { ; };
    /**
      Move cursor in the view
    */
    virtual void cursorLeft () { ; };
    virtual void shiftCursorLeft () { ; };
    virtual void cursorRight () { ; };
    virtual void shiftCursorRight () { ; };
    virtual void wordLeft () { ; };
    virtual void shiftWordLeft () { ; };
    virtual void wordRight () { ; };
    virtual void shiftWordRight () { ; };
    virtual void home () { ; };
    virtual void shiftHome () { ; };
    virtual void end () { ; };
    virtual void shiftEnd () { ; };
    virtual void up () { ; };
    virtual void shiftUp () { ; };
    virtual void down () { ; };
    virtual void shiftDown () { ; };
    virtual void scrollUp () { ; };
    virtual void scrollDown () { ; };
    virtual void topOfView () { ; };
    virtual void bottomOfView () { ; };
    virtual void pageUp () { ; };
    virtual void shiftPageUp () { ; };
    virtual void pageDown () { ; };
    virtual void shiftPageDown () { ; };
    virtual void top () { ; };
    virtual void shiftTop () { ; };
    virtual void bottom () { ; };
    virtual void shiftBottom () { ; };
    /**
      Presents a search dialog to the user.
    */
    virtual void find() { ; };
    /**
      Presents a replace dialog to the user.
    */
    virtual void replace() { ; };
    /**
      Presents a "Goto Line" dialog to the user.
    */
    virtual void gotoLine() { ; };

  public:
    /**
      Reads session config out of the KConfig object. This also includes
      the actual cursor position and the bookmarks.
    */
    virtual void readSessionConfig(KConfig *) { ; };
    /**
      Writes session config into the KConfig object.
    */
    virtual void writeSessionConfig(KConfig *) { ; };

  public slots:
    /**
      Get the end of line mode (Unix, Macintosh or Dos).
    */
    virtual int getEol() { return 0L; };
    /**
      Set the end of line mode (Unix, Macintosh or Dos).
    */
    virtual void setEol(int) { ; };
    /**
      Set focus to the current window.
    */
    // Should remove this, it's redundant.
    virtual void setFocus () { QWidget::setFocus(); } 
    /**
      Searches for the last searched text forward from cursor position.
      @param bool forward determines the search direction.
    */
    virtual void findAgain(bool ) { ; };
    /**
      Searches for the last searched text forward from cursor position.
      Searches forward from current cursor position.
    */
    virtual void findAgain () { ; };
    /**
      Searches for the last searched text forward from cursor position.
      Searches backward from current cursor position.
    */
    virtual void findPrev () { ; };
    /**
      Presents an edit command popup window, where the user can
      apply a shell command to the contents of the current window.
    */
    virtual void slotEditCommand () { ; };
    /**
      Sets icon border on or off depending on
      @param bool enable.
    */
    virtual void setIconBorder (bool) { ; };
    /**
      Toggles icon border.
    */
    virtual void toggleIconBorder () { ; };
    /**
      Sets display of line numbers on/off depending on @param enable
    */
    virtual void setLineNumbersOn (bool) {};
    /**
      Toggles display of lineNumbers
    */
    virtual void toggleLineNumbersOn () {};

  public:
    /**
      Returns whether iconborder is visible.
    */
    virtual bool iconBorder() { return false; };
    /**
      @return Wheather line numbers display is on
    */
    virtual bool lineNumbersOn() { return false; };
    /**
     Returns a pointer to the document of the view.
    */
    virtual Document *getDoc () { return 0L; };

  public slots:
    /**
      Increase font size.
    */
    virtual void slotIncFontSizes () { ; };
    /**
      Decrease font size.
    */
    virtual void slotDecFontSizes () { ; };

    virtual void gotoMark (KTextEditor::Mark *mark) = 0;
    
    /**
     * @deprecated No longer does anything. Use KTextEditor
     * equivalents
     */
    // TODO: Remove when BIC is allowed
    virtual void toggleBookmark () {};

    virtual void gotoLineNumber( int ) = 0;

  signals:
    void gotFocus (View *);
//  void newStatus(); // Kate app connects to this signal, should be in the interface

  public:
    virtual void setActive (bool b) = 0;
    virtual bool isActive () = 0;
};
};

#endif
