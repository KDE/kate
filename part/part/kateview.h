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

#include "kateglobal.h"
#include "kateviewhighlightaction.h"
#include "katecodecompletion_iface_impl.h"
#include "kateiconborder.h"

#include "../interfaces/view.h"
#include "../interfaces/document.h"

#include <kparts/browserextension.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qstring.h>
#include <qdialog.h>

class KToggleAction;
class KActionMenu;
class KAction;
class KRecentFilesAction;
class KSelectAction;
class QTextDrag;
class KPrinter;
class Highlight;
//class KateIconBorder;
class KateDocument;
class KateBrowserExtension;

//state commands
enum State_commands {
  cmToggleInsert      = 1,
  cmToggleVertical    = 2
};

class KateViewInternal;
class KateView;

struct VConfig {
  KateView *view;
  KateTextCursor cursor;
  int cXPos;
  int flags;
};

struct BracketMark {
  KateTextCursor cursor;
  int sXPos;
  int eXPos;
};

//
// config + options for the user search
//
class SConfig
{
  public:
    KateTextCursor cursor;
    int flags;

    // Set the pattern to be used for searching.
    void setPattern(QString &newPattern);

    // The length of the last match found using pattern or regExp.
    int matchedLength;

    QString m_pattern;

    // The regular expression corresponding to pattern. Only guaranteed valid if
    // flags has sfRegularExpression set.
    QRegExp m_regExp;
};

//
// the real widget with the textarea inside
//
class KateViewInternal : public QWidget
{
    Q_OBJECT
    friend class KateDocument;
    friend class KateView;
    friend class KateIconBorder;
    friend class CodeCompletion_Impl;

  private:
    //uint iconBorderWidth;
    uint iconBorderHeight;

  public:
    KateViewInternal(KateView *view, KateDocument *doc);
    ~KateViewInternal();

    void doCursorCommand(VConfig &, int cmdNum);
    void doEditCommand(VConfig &, int cmdNum);

    void cursorLeft(VConfig &);
    void cursorRight(VConfig &);
    void wordLeft(VConfig &);
    void wordRight(VConfig &);
    void home(VConfig &);
    void end(VConfig &);
    void cursorUp(VConfig &);
    void cursorDown(VConfig &);
    void scrollUp(VConfig &);
    void scrollDown(VConfig &);
    void topOfView(VConfig &);
    void bottomOfView(VConfig &);
    void pageUp(VConfig &);
    void pageDown(VConfig &);
    void cursorPageUp(VConfig &);
    void cursorPageDown(VConfig &);
    void top(VConfig &);
    void bottom(VConfig &);
    void top_home(VConfig &c);
    void bottom_end(VConfig &c);

  private slots:
    void changeXPos(int);
    void changeYPos(int);
    void tripleClickTimeout();

  private:
    void getVConfig(VConfig &);
    void changeState(VConfig &);
    void insLine(int line);
    void delLine(int line);
    void updateCursor();
    void updateCursor(KateTextCursor &newCursor, bool keepSel=false);
    void clearDirtyCache(int height);
    void tagLines(int start, int end, int x1, int x2);
    void tagAll();
    void setPos(int x, int y);
    void center();

    void updateView(int flags);

    void paintTextLines(int xPos, int yPos);
    void paintCursor();
    void paintBracketMark();

    void placeCursor(int x, int y, int flags = 0);
    bool isTargetSelected(int x, int y);

    void doDrag();

    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void wheelEvent( QWheelEvent *e );
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void timerEvent(QTimerEvent *);

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

    KateView *myView;
    KateDocument *myDoc;
    QScrollBar *xScroll;
    QScrollBar *yScroll;
    KateIconBorder *leftBorder;

    // cursor position in pixels:
    int xCoord;
    int yCoord;

    int xPos;
    int yPos;

    int mouseX;
    int mouseY;
    int scrollX;
    int scrollY;
    int scrollTimer;

    KateTextCursor cursor;
    bool cursorOn;
    int cursorTimer;
    int cXPos;
    int cOldXPos;

    int startLine;
    int endLine;
    uint maxLen;

    bool possibleTripleClick;
    bool exposeCursor;
    int updateState;
    uint numLines;
    class KateLineRange *lineRanges;
    int newXPos;
    int newYPos;

    QPixmap *drawBuffer;

    BracketMark bm;

    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState       state;
      KateTextCursor      start;
      QTextDrag       *dragObject;
    } dragInfo;

  signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);
};

//
// Kate KTextEditor::View class ;)
//
class KateView : public Kate::View
{
    Q_OBJECT
    friend class KateViewInternal;
    friend class KateDocument;
    friend class KateUndoGroup;
    friend class KateIconBorder;
    friend class CodeCompletion_Impl;

  public:
    KateView (KateDocument *doc, QWidget *parent = 0L, const char * name = 0);
    ~KateView ();

  //
  // KTextEditor::View stuff
  //
  public:
    KateDocument *document();

  //
  // KTextEditor::ClipboardInterface stuff
  //
  public slots:
    /**
     * Moves the marked text into the clipboard
     */
    void cut() {doEditCommand(KateView::cmCut);}
    /**
     * Copies the marked text into the clipboard
     */
    void copy() const;
    /**
     * Inserts text from the clipboard at the actual cursor position
     */
    void paste() {doEditCommand(KateView::cmPaste);}

  //
  // KTextEditor::ViewCursorInterface stuff
  //
  public slots:
    /**
     * Get the current cursor coordinates in pixels.
     */
    QPoint cursorCoordinates ();

    /**
     * Get the cursor position
     */
    void cursorPosition (uint *line, uint *col);

    /**
     * Get the cursor position, calculated with 1 character per tab
     */
    void cursorPositionReal (uint *line, uint *col);

    /**
     * Set the cursor position
     */
    bool setCursorPosition (uint line, uint col);

    /**
     * Set the cursor position, use 1 character per tab
     */
    bool setCursorPositionReal (uint line, uint col);

    uint cursorLine ();
    uint cursorColumn ();
    uint cursorColumnReal ();
    
  private:
    /**
     * Sets the current cursor position (only for internal use)
     */
    void setCursorPositionInternal(int line, int col, int tabwidth);

  signals:
    void cursorPositionChanged ();

  //
  // KTextEditor::PopupMenuInterface stuff
  //
  public:
    /**
     * Install a Popup Menu. The Popup Menu will be activated on
     * a right mouse button press event.
     */
    void installPopup(QPopupMenu *rmb_Menu);

  private:
    QPopupMenu *rmbMenu;
    
  //
  // KTextEditor::DynWordWrapInterface stuff
  //
  public:
    void setDynWordWrap (bool b);
    bool dynWordWrap () const;
    
  private:
    bool _hasWrap;

  //
  // internal KateView stuff
  //
  public:
    bool isOverwriteMode() const;
    void setOverwriteMode( bool b );

    int tabWidth();
    void setTabWidth(int);
    void setEncoding (QString e);

    /**
     * Returns true if this editor is the only owner of its document
     */
    bool isLastView();
    /**
     * Returns the document object
     */
    KateDocument *doc();

    void setupActions();

    KAction *editUndo, *editRedo, *bookmarkToggle, *bookmarkClear;

    KActionMenu *bookmarkMenu;
//    KToggleAction *viewBorder;
//    KToggleAction *viewLineNumbers;
    KRecentFilesAction *fileRecent;
    KSelectAction *setEndOfLine;
    Kate::ActionMenu *setHighlight;

  private slots:
    void slotDropEventPass( QDropEvent * ev );

  public slots:
    void slotUpdate();
    void slotFileStatusChanged();

  public slots:
    /**
     * Toggles Insert mode
     */
    void toggleInsert();

  signals:
    /**
     * Modified flag or config flags have changed
     */
    void newStatus();

  protected:
    void keyPressEvent( QKeyEvent *ev );
    void customEvent( QCustomEvent *ev );
    virtual void contextMenuEvent( QContextMenuEvent *ev );

    /**
     * Check if the given URL already exists. Currently used by both save() and saveAs()
     *
     * Asks the user for permission and returns the message box result and defaults to
     * KMessageBox::Yes in case of doubt
     */
    int checkOverwrite( KURL u );

//text access
  public:
     /**
      * Gets the text line where the cursor is on
      */
     QString currentTextLine();
     /**
      * Gets the word where the cursor is on
      */
     QString currentWord();
     /**
      * Gets the word at position x, y. Can be used to find
      * the word under the mouse cursor
      */
     QString word(int x, int y);
     /**
      * Insert text at the current cursor position.
      * The parameter @param mark is unused.
      */
     void insertText(const QString &);

  public:

    /**
     * Returns true if the current document can be
     * discarded. If the document is modified, the user is asked if he wants
     * to save it. On "cancel" the function returns false.
     */
    bool canDiscard();

  public slots:
    /**
     * Flushes the document of the text widget. The user is given
     * a chance to save the current document if the current document has
     * been modified.
     */
    void flush ();
    /**
     * Saves the file if necessary under the current file name. If the current file
     * name is Untitled, as it is after a call to newFile(), this routine will
     * call saveAs().
     */
    saveResult save();
    /**
     * Allows the user to save the file under a new name. This starts the
     * automatic highlight selection.
     */
    saveResult saveAs();
    /**
     * Moves the current line or the selection one position to the right
     */
    void indent() {doEditCommand(KateView::cmIndent);};
    /**
     * Moves the current line or the selection one position to the left
     */
    void unIndent() {doEditCommand(KateView::cmUnindent);};
    /**
     * Optimizes the selected indentation, replacing tabs and spaces as needed
     */
    void cleanIndent() {doEditCommand(KateView::cmCleanIndent);};
    /**
     * comments out current line
     */
    void comment() {doEditCommand(KateView::cmComment);};
    /**
     * removes comment signs in the current line
     */
    void uncomment() {doEditCommand(KateView::cmUncomment);};

    void keyReturn() {doEditCommand(KateView::cmReturn);};
    void keyDelete() {doEditCommand(KateView::cmDelete);};
    void backspace() {doEditCommand(KateView::cmBackspace);};
    void killLine() {doEditCommand(KateView::cmKillLine);};


    // cursor commands...
private:
    KAccel *m_editAccels;
    void setupEditKeys();
public slots:
    void cursorLeft() {doCursorCommand(KateView::cmLeft);};
    void shiftCursorLeft() {doCursorCommand(KateView::cmLeft | selectFlag);};
    void cursorRight() {doCursorCommand(KateView::cmRight);}
    void shiftCursorRight() {doCursorCommand(KateView::cmRight | selectFlag);}
    void wordLeft() {doCursorCommand(KateView::cmWordLeft);};
    void shiftWordLeft() {doCursorCommand(KateView::cmWordLeft | selectFlag);};
    void wordRight() {doCursorCommand(KateView::cmWordRight);};
    void shiftWordRight() {doCursorCommand(KateView::cmWordRight | selectFlag);};
    void home() {doCursorCommand(KateView::cmHome);};
    void shiftHome() {doCursorCommand(KateView::cmHome | selectFlag);};
    void end() {doCursorCommand(KateView::cmEnd);};
    void shiftEnd() {doCursorCommand(KateView::cmEnd | selectFlag);};
    void up() {doCursorCommand(KateView::cmUp);};
    void shiftUp() {doCursorCommand(KateView::cmUp | selectFlag);};
    void down() {doCursorCommand(KateView::cmDown);};
    void shiftDown() {doCursorCommand(KateView::cmDown | selectFlag);};
    void scrollUp() {doCursorCommand(KateView::cmScrollUp);};
    void scrollDown() {doCursorCommand(KateView::cmScrollDown);};
    void topOfView() {doCursorCommand(KateView::cmTopOfView);};
    void bottomOfView() {doCursorCommand(KateView::cmBottomOfView);};
    void pageUp() {doCursorCommand(KateView::cmPageUp);};
    void shiftPageUp() {doCursorCommand(KateView::cmPageUp | selectFlag);};
    void pageDown() {doCursorCommand(KateView::cmPageDown);};
    void shiftPageDown() {doCursorCommand(KateView::cmPageDown | selectFlag);};
    void top() {doCursorCommand(KateView::cmTop);};
    void shiftTop() {doCursorCommand(KateView::cmTop | selectFlag);};
    void bottom() {doCursorCommand(KateView::cmBottom);};
    void shiftBottom() {doCursorCommand(KateView::cmBottom | selectFlag);};

		void slotNewUndo ();

//search/replace functions
  public slots:
    /**
     * Presents a search dialog to the user
     */
    void find();
    /**
     * Presents a replace dialog to the user
     */
    void replace();

    /**
     * Presents a "Goto Line" dialog to the user
     */
    void gotoLine();

    /**
     * Goes to a given line number; also called by the gotoline slot.
     */
    void gotoLineNumber( int linenumber );

  private:
    void initSearch(SConfig &, int flags);
    void continueSearch(SConfig &);
    void findAgain(SConfig &);
    void replaceAgain();
    void doReplaceAction(int result, bool found = false);
    void exposeFound(KateTextCursor &cursor, int slen, int flags, bool replace);
    void deleteReplacePrompt();
    bool askReplaceEnd();

  private slots:
    void replaceSlot();

  private:
    uint searchFlags;
    int replaces;
    QDialog *replacePrompt;

  signals:
    void bookAddChanged(bool enabled);
    void bookClearChanged(bool enabled);

//code completion
  private:
    CodeCompletion_Impl *myCC_impl;
    void initCodeCompletionImplementation();

  public:
    virtual void showArgHint(QStringList arg1, const QString &arg2, const QString &arg3)
    	{ myCC_impl->showArgHint(arg1,arg2,arg3);}
    virtual void showCompletionBox(QValueList<KTextEditor::CompletionEntry> arg1, int arg2= 0, bool arg3=true)
    	{ myCC_impl->showCompletionBox(arg1,arg2,arg3);}

  signals:
    void completionAborted();
    void completionDone();
    void argHintHidden();
    void completionDone(KTextEditor::CompletionEntry);
    void filterInsertString(KTextEditor::CompletionEntry*,QString *);


//config file / session management functions
  public:
    /**
     * Reads session config out of the KConfig object. This also includes
     * the actual cursor position and the bookmarks.
     */
    void readSessionConfig(KConfig *);
    /**
     * Writes session config into the KConfig object
     */
    void writeSessionConfig(KConfig *);

  // syntax highlight
  public slots:
    /**
     * Get the end of line mode (Unix, Macintosh or Dos)
     */
    int getEol();
    /**
     * Set the end of line mode (Unix, Macintosh or Dos)
     */
    void setEol(int);

  private:
    void resizeEvent(QResizeEvent *);

    void doCursorCommand(int cmdNum);
    void doEditCommand(int cmdNum);

    KateViewInternal *myViewInternal;
    KateDocument *myDoc;

  private slots:
    void dropEventPassEmited (QDropEvent* e);

  signals:
    void dropEventPass(QDropEvent*);

  public:
    enum Select_flags {
  selectFlag          = 0x100000
    };

    enum Dialog_results {
      srYes=QDialog::Accepted,
      srNo=10,
      srAll,
      srCancel=QDialog::Rejected};

//update flags
    enum Update_flags {
     ufDocGeometry=1,
     ufUpdateOnScroll=2,
     ufPos=4,
     ufLeftBorder=8
     };

//cursor movement commands
    enum Cursor_commands
	   { cmLeft,cmRight,cmWordLeft,cmWordRight,
       cmHome,cmEnd,cmUp,cmDown,
       cmScrollUp,cmScrollDown,cmTopOfView,cmBottomOfView,
       cmPageUp,cmPageDown,cmCursorPageUp,cmCursorPageDown,
       cmTop,cmBottom};

//edit commands
    enum Edit_commands {
		    cmReturn=1,cmDelete,cmBackspace,cmKillLine,
        cmCut,cmCopy,cmPaste,cmIndent,cmUnindent,cmCleanIndent,
        cmComment,
        cmUncomment};
//find commands
    enum Find_commands { cmFind=1,cmReplace,cmFindAgain,cmGotoLine};

  public:
    void setActive (bool b);
    bool isActive ();

  private:
    bool active;
    //bool myIconBorder;// FIXME anders: remove
    int iconBorderStatus;
    QPtrList<KTextEditor::Mark> list;

  public slots:
    void setFocus ();
    void findAgain(bool back=false);
//    void findAgain () { findAgain(false); };
    void findPrev () { findAgain(true); };

  private:
    bool eventFilter(QObject* o, QEvent* e);

  signals:
    void gotFocus (Kate::View *);

  public slots:
    void slotEditCommand ();
    void setIconBorder (bool enable);
    void setLineNumbersOn(bool enable);
    void toggleIconBorder ();
    void toggleLineNumbersOn();
    void gotoMark (KTextEditor::Mark *mark);
    void toggleBookmark ();

  private:
    void updateIconBorder();

  public:
    bool iconBorder() { return /*myIconBorder;*/iconBorderStatus & KateIconBorder::Icons; } ;
    bool lineNumbersOn() { return iconBorderStatus & KateIconBorder::LineNumbers; }

  private slots:
    void bookmarkMenuAboutToShow();
    void gotoBookmark (int n);

  public:
    Kate::Document *getDoc ()
      { return (Kate::Document*) myDoc; };

  public slots:
    void slotIncFontSizes ();
    void slotDecFontSizes ();

  public:
    KTextEditor::Document *document () const { return (KTextEditor::Document *)myDoc; };

  //
  // cursor cache for document
  // here stores the document the view's cursor pos while editing before update
  //
  private:
    KateTextCursor cursorCache;
    bool cursorCacheChanged;
    KateBrowserExtension *extension;
};

class KateBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT

  public:
    KateBrowserExtension( KateDocument *doc, KateView *view );

  public slots:
    void copy();
    void slotSelectionChanged();
    void print();

  private:
    KateDocument *m_doc;
    KateView *m_view;
};

#endif






