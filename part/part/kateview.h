/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>  
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef kate_view_h
#define kate_view_h

#include "../interfaces/view.h"

#include "kateglobal.h"
#include "katedocument.h"
#include "katesearch.h"

class KToggleAction;
class KAction;
class KRecentFilesAction;
class KSelectAction;
class KateDocument;
class KateViewInternal;
class KateBookmarks;
class KateBrowserExtension;

//
// Kate KTextEditor::View class ;)
//
class KateView : public Kate::View
{
    Q_OBJECT
    friend class KateViewInternal;
    friend class KateDocument;
    friend class KateUndoGroup;
    friend class KateUndo;
    friend class KateIconBorder;
    friend class CodeCompletion_Impl;

  public:
    KateView (KateDocument *doc, QWidget *parent = 0L, const char * name = 0);
    ~KateView ();

  //
  // KTextEditor::ClipboardInterface stuff
  //
  public slots:
    /**
      Moves the marked text into the clipboard
    */
    void cut() {doEditCommand(KateView::cmCut);}
    /**
      Copies the marked text into the clipboard
    */
    void copy() const;
    /**
      Inserts text from the clipboard at the actual cursor position
    */
    void paste() {doEditCommand(KateView::cmPaste);}

  //
  // KTextEditor::ViewCursorInterface stuff
  //
  public slots:
    /** Get the current cursor coordinates in pixels. */
    QPoint cursorCoordinates ();

    /** Get the cursor position */
    void cursorPosition (uint *line, uint *col);

    /** Get the cursor position, calculated with 1 character per tab */
    void cursorPositionReal (uint *line, uint *col);

    /** Set the cursor position */
    bool setCursorPosition (uint line, uint col);

    /** Set the cursor position, use 1 character per tab */
    bool setCursorPositionReal (uint line, uint col);

    uint cursorLine ();
    uint cursorColumn ();
    uint cursorColumnReal ();

  signals:
    void cursorPositionChanged ();

  //
  // KTextEditor::PopupMenuInterface stuff
  //
  public:
    /**
      Install a Popup Menu. The Popup Menu will be activated on
      a right mouse button press event.
    */
    void installPopup(QPopupMenu *rmb_Menu);
    QPopupMenu* popup() { return m_rmbMenu; }

  //
  // KTextEditor::DynWordWrapInterface stuff
  //
  public:
    void setDynWordWrap (bool b);
    bool dynWordWrap () const;

  //
  // Kate::View stuff
  //
  public slots:
    // cursor movement slots
    void cursorLeft();
    void shiftCursorLeft();
    void cursorRight();
    void shiftCursorRight();
    void wordLeft();
    void shiftWordLeft();
    void wordRight();
    void shiftWordRight();
    void home();
    void shiftHome();
    void end();
    void shiftEnd();
    void up();
    void shiftUp();
    void down();
    void shiftDown();
    void scrollUp();
    void scrollDown();
    void topOfView();
    void bottomOfView();
    void pageUp();
    void shiftPageUp();
    void pageDown();
    void shiftPageDown();
    void top();
    void shiftTop();
    void bottom();
    void shiftBottom();

  public:
    /**
     Gets the text line where the cursor is on
    */
    QString currentTextLine();
    /**
     Gets the word where the cursor is on
    */
    QString currentWord();
    /**
     Gets the word at position x, y. Can be used to find
     the word under the mouse cursor
    */
//FIXME     QString word(int x, int y);
    /**
     Insert text at the current cursor position.
     The parameter @param mark is unused.
    */
    void insertText(const QString &);
    /**
     Returns true if the current document can be
     discarded. If the document is modified, the user is asked if he wants
     to save it. On "cancel" the function returns false.
    */
    bool canDiscard();

  public slots:
    /**
      Flushes the document of the text widget. The user is given
      a chance to save the current document if the current document has
      been modified.
    */
    void flush ();
    /**
      Saves the file if necessary under the current file name. If the current file
      name is Untitled, as it is after a call to newFile(), this routine will
      call saveAs().
    */
    saveResult save();
    /**
      Allows the user to save the file under a new name. This starts the
      automatic highlight selection.
    */
    saveResult saveAs();
    /**
      Moves the current line or the selection one position to the right
    */
    void indent() {doEditCommand(KateView::cmIndent);};
    /**
      Moves the current line or the selection one position to the left
    */
    void unIndent() {doEditCommand(KateView::cmUnindent);};
    /**
      Optimizes the selected indentation, replacing tabs and spaces as needed
    */
    void cleanIndent() {doEditCommand(KateView::cmCleanIndent);};
    /**
      comments out current line
    */
    void comment() {doEditCommand(KateView::cmComment);};
    /**
      removes comment signs in the current line
    */
    void uncomment() {doEditCommand(KateView::cmUncomment);};

    void keyReturn() {doEditCommand(KateView::cmReturn);};
    void keyDelete() {doEditCommand(KateView::cmDelete);};
    void backspace() {doEditCommand(KateView::cmBackspace);};
    void transpose() {doEditCommand(KateView::cmTranspose);};
    void killLine() {doEditCommand(KateView::cmKillLine);};

//search/replace functions
  public slots:
    void find()                   { m_search->find(); }
    void replace()                { m_search->replace(); }
    void findAgain( bool back )   { m_search->findAgain( back ); }
    void findAgain()              { findAgain( false ); }
    void findPrev()               { findAgain( true ); }

    void gotoLine();
    void gotoLineNumber( int linenumber );

//code completion
  private:
    CodeCompletion_Impl *myCC_impl;
    void initCodeCompletionImplementation();

  public:
    void showArgHint(QStringList arg1, const QString &arg2, const QString &arg3);
    void showCompletionBox(QValueList<KTextEditor::CompletionEntry> arg1, int arg2= 0, bool arg3=true);

  signals:
    void completionAborted();
    void completionDone();
    void argHintHidden();
    void completionDone(KTextEditor::CompletionEntry);
    void filterInsertString(KTextEditor::CompletionEntry*,QString *);


//config file / session management functions
  public:
    /**
      Reads session config out of the KConfig object. This also includes
      the actual cursor position and the bookmarks.
    */
    void readSessionConfig(KConfig *);
    /**
      Writes session config into the KConfig object
    */
    void writeSessionConfig(KConfig *);

  // syntax highlight
  public slots:
    /**
      Get the end of line mode (Unix, Macintosh or Dos)
    */
    int getEol();
    /**
      Set the end of line mode (Unix, Macintosh or Dos)
    */
    void setEol(int eol);
  
public:
    enum Edit_commands {
        cmReturn = 1,
        cmDelete, cmBackspace, cmKillLine,
        cmCut, cmCopy, cmPaste,
        cmIndent, cmUnindent, cmCleanIndent,
        cmComment, cmUncomment,
        cmTranspose
    };

public:
    bool iconBorder();
    bool lineNumbersOn();
    void setupEditKeys();
    bool isOverwriteMode() const;
    void setOverwriteMode( bool b );
    int tabWidth();
    void setTabWidth(int);
    void setEncoding (QString e);
    bool isLastView();
    
    void setActive( bool b )    { m_active = b; }
    bool isActive() /* const */ { return m_active; }
    
    int iconBorderStatus() const { return m_iconBorderStatus; }

public:
    // Is it really necessary to have 3 methods for this?! :)
    KTextEditor::Document* document() const       { return myDoc; }
    Kate::Document*        getDoc() /* const */   { return myDoc; }
    KateDocument*          doc()      const       { return myDoc; }

public slots:
    void slotEditCommand ();
    void setIconBorder (bool enable);
    void setLineNumbersOn(bool enable);
    void setFoldingMarkersOn(bool enable);
    void toggleIconBorder ();
    void toggleLineNumbersOn();
    void gotoMark (KTextEditor::Mark *mark) { setCursorPositionReal( mark->line, 0 ); }
    void slotIncFontSizes ();
    void slotDecFontSizes ();
    void setFocus ();
    void slotNewUndo ();
    void slotUpdate();
    void toggleInsert();
    void slotRegionVisibilityChangedAt(unsigned int);
    void slotRegionBeginEndAddedRemoved(unsigned int);
    void slotCodeFoldingChanged();
    void reloadFile();

signals:
    void gotFocus (Kate::View *);
    void dropEventPass(QDropEvent*);
    /**
      Modified flag or config flags have changed
    */
    void newStatus();
    
protected:
    void keyPressEvent( QKeyEvent *ev );
    void customEvent( QCustomEvent *ev );
    void contextMenuEvent( QContextMenuEvent *ev );
    void resizeEvent( QResizeEvent* );
    bool eventFilter( QObject* o, QEvent* e );
 
    /*
     * Check if the given URL already exists. Currently used by both save() and saveAs()
     *
     * Asks the user for permission and returns the message box result and defaults to
     * KMessageBox::Yes in case of doubt
     */
    int checkOverwrite( KURL u );

private slots:
    void slotDropEventPass( QDropEvent* ev );
    void dropEventPassEmited( QDropEvent* e );
    void slotSetEncoding( const QString& descriptiveName );

private:
    KAccel* createEditKeys();
    void setupActions();
    
    void updateIconBorder();
    
    void doCursorCommand( int cmdNum );
    void doEditCommand( int cmdNum );
    void setCursorPositionInternal( int line, int col, int tabwidth );

    KAction*               m_editUndo;
    KAction*               m_editRedo;
//    KToggleAction* viewBorder;
//    KToggleAction* viewLineNumbers;
    KRecentFilesAction*    m_fileRecent;
    KSelectAction*         m_setEndOfLine;
    KSelectAction*         m_setEncoding;
    Kate::ActionMenu*      m_setHighlight;

    KateDocument*          myDoc;
    KateViewInternal*      myViewInternal;
    KAccel*                m_editAccels;
    KateSearch*            m_search;
    KateBookmarks*         m_bookmarks;
    KateBrowserExtension*  m_extension;
    QPopupMenu*            m_rmbMenu;

    bool       m_active;
    int        m_iconBorderStatus;
    bool       m_hasWrap;
};

#endif
