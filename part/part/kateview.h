/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
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

#include "katedocument.h"
#include "kateviewinternal.h"
#include "kateconfig.h"

#include "../interfaces/view.h"

#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/viewstatusmsginterface.h>
#include <ktexteditor/texthintinterface.h>

#include <qguardedptr.h>

class KateDocument;
class KateBookmarks;
class KateSearch;
class KateCmdLine;
class KateCodeCompletion;
class KateViewConfig;
class KateViewSchemaAction;
class KateRenderer;

class KToggleAction;
class KAction;
class KRecentFilesAction;
class KSelectAction;

class QGridLayout;

//
// Kate KTextEditor::View class ;)
//
class KateView : public Kate::View,
                 public KTextEditor::SessionConfigInterface,
                 public KTextEditor::ViewStatusMsgInterface,
                 public KTextEditor::TextHintInterface
{
    Q_OBJECT

    friend class KateViewInternal;
    friend class KateIconBorder;
    friend class KateCodeCompletion;

  public:
    KateView( KateDocument* doc, QWidget* parent = 0L, const char* name = 0 );
    ~KateView ();

  //
  // KTextEditor::View
  //
  public:
    KTextEditor::Document* document() const       { return m_doc; }

  //
  // KTextEditor::ClipboardInterface
  //
  public slots:
    void cut()           { m_doc->cut(); m_viewInternal->repaint(); }
    void copy() const    { m_doc->copy();   }
    // TODO: Factor out of m_viewInternal
    void paste()         { m_viewInternal->doPaste(); m_viewInternal->repaint(); }

  //
  // KTextEditor::PopupMenuInterface
  //
  public:
    void installPopup( QPopupMenu* menu ) { m_rmbMenu = menu; }
    QPopupMenu* popup() const             { return m_rmbMenu; }

  //
  // KTextEditor::ViewCursorInterface
  //
  public slots:
    QPoint cursorCoordinates()
        { return m_viewInternal->cursorCoordinates();                 }
    void cursorPosition( uint* l, uint* c )
        { if( l ) *l = cursorLine(); if( c ) *c = cursorColumn();     }
    void cursorPositionReal( uint* l, uint* c )
        { if( l ) *l = cursorLine(); if( c ) *c = cursorColumnReal(); }
    bool setCursorPosition( uint line, uint col )
        { return setCursorPositionInternal( line, col, tabWidth(), true );  }
    bool setCursorPositionReal( uint line, uint col)
        { return setCursorPositionInternal( line, col, 1, true );           }
    uint cursorLine()
        { return m_viewInternal->getCursor().line();                    }
    uint cursorColumn();
    uint cursorColumnReal()
        { return m_viewInternal->getCursor().col();                     }

  signals:
    void cursorPositionChanged();

  //
  // KTextEditor::CodeCompletionInterface
  //
  public slots:
    void showArgHint( QStringList arg1, const QString& arg2, const QString& arg3 );
    void showCompletionBox( QValueList<KTextEditor::CompletionEntry> arg1, int offset = 0, bool cs = true );

  signals:
    void completionAborted();
    void completionDone();
    void argHintHidden();
    void completionDone(KTextEditor::CompletionEntry);
    void filterInsertString(KTextEditor::CompletionEntry*,QString *);
    void aboutToShowCompletionBox();

  //
  // KTextEditor::TextHintInterface
  //
  public:
    void enableTextHints(int timeout);
    void disableTextHints();

  signals:
    void needTextHint(int line, int col, QString &text);

  //
  // KTextEditor::DynWordWrapInterface
  //
  public:
    void setDynWordWrap( bool b );
    bool dynWordWrap() const      { return m_hasWrap; }

  //BEGIN EDIT STUFF
  public:
    void editStart ();
    void editEnd (int editTagLineStart, int editTagLineEnd, bool tagFrom);

    void editSetCursor (const KateTextCursor &cursor);
  //END

  //BEGIN TAG & CLEAR
  public:
    bool tagLine (const KateTextCursor& virtualCursor);

    bool tagLines (int start, int end, bool realLines = false );
    bool tagLines (KateTextCursor start, KateTextCursor end, bool realCursors = false);

    void tagAll ();

    void clear ();

    void repaintText (bool paintOnlyDirty = false);

    void updateView (bool changed = false);
  //END

  //
  // Kate::View
  //
  public:
    bool isOverwriteMode() const;
    void setOverwriteMode( bool b );

    QString currentTextLine()
        { return getDoc()->textLine( cursorLine() ); }
    QString currentWord()
        { return m_doc->getWord( m_viewInternal->getCursor() ); }
    void insertText( const QString& text )
        { getDoc()->insertText( cursorLine(), cursorColumnReal(), text ); }
    bool canDiscard();
    int tabWidth()                { return m_doc->config()->tabWidth(); }
    void setTabWidth( int w )     { m_doc->config()->setTabWidth(w);  }
    void setEncoding( QString e ) { m_doc->setEncoding(e);       }
    bool isLastView()             { return m_doc->isLastView(1); }

  public slots:
    void flush();
    saveResult save();
    saveResult saveAs();

    void indent()             { m_doc->indent( this, cursorLine(), 1 );  }
    void unIndent()           { m_doc->indent( this, cursorLine(), -1 ); }
    void cleanIndent()        { m_doc->indent( this, cursorLine(), 0 );  }
    void align()              { m_doc->align( cursorLine() ); }
    void comment()            { m_doc->comment( this, cursorLine(), cursorColumnReal(), 1 );  }
    void uncomment()          { m_doc->comment( this, cursorLine(), cursorColumnReal(),-1 ); }
    void killLine()           { m_doc->removeLine( cursorLine() ); }

    /**
      Uppercases selected text, or an alphabetic character next to the cursor.
    */
    void uppercase() { m_doc->transform( this, m_viewInternal->cursor, KateDocument::Uppercase ); }
    /**
      Lowercases selected text, or an alphabetic character next to the cursor.
    */
    void lowercase() { m_doc->transform( this, m_viewInternal->cursor, KateDocument::Lowercase ); }
    /**
      Capitalizes the selection (makes each word start with an uppercase) or
      the word under the cursor.
    */
    void capitalize() { m_doc->transform( this, m_viewInternal->cursor, KateDocument::Capitalize ); }
    /**
      Joins lines touched by the selection
    */
    void joinLines();


    void keyReturn()          { m_viewInternal->doReturn();          }
    void backspace()          { m_viewInternal->doBackspace();       }
    void deleteWordLeft()     { m_viewInternal->doDeleteWordLeft();  }
    void keyDelete()          { m_viewInternal->doDelete();          }
    void deleteWordRight()    { m_viewInternal->doDeleteWordRight(); }
    void transpose()          { m_viewInternal->doTranspose();       }
    void cursorLeft()         { m_viewInternal->cursorLeft();        }
    void shiftCursorLeft()    { m_viewInternal->cursorLeft(true);    }
    void cursorRight()        { m_viewInternal->cursorRight();       }
    void shiftCursorRight()   { m_viewInternal->cursorRight(true);   }
    void wordLeft()           { m_viewInternal->wordLeft();          }
    void shiftWordLeft()      { m_viewInternal->wordLeft(true);      }
    void wordRight()          { m_viewInternal->wordRight();         }
    void shiftWordRight()     { m_viewInternal->wordRight(true);     }
    void home()               { m_viewInternal->home();              }
    void shiftHome()          { m_viewInternal->home(true);          }
    void end()                { m_viewInternal->end();               }
    void shiftEnd()           { m_viewInternal->end(true);           }
    void up()                 { m_viewInternal->cursorUp();          }
    void shiftUp()            { m_viewInternal->cursorUp(true);      }
    void down()               { m_viewInternal->cursorDown();        }
    void shiftDown()          { m_viewInternal->cursorDown(true);    }
    void scrollUp()           { m_viewInternal->scrollUp();          }
    void scrollDown()         { m_viewInternal->scrollDown();        }
    void topOfView()          { m_viewInternal->topOfView();         }
    void shiftTopOfView()     { m_viewInternal->topOfView(true);     }
    void bottomOfView()       { m_viewInternal->bottomOfView();      }
    void shiftBottomOfView()  { m_viewInternal->bottomOfView(true);  }
    void pageUp()             { m_viewInternal->pageUp();            }
    void shiftPageUp()        { m_viewInternal->pageUp(true);        }
    void pageDown()           { m_viewInternal->pageDown();          }
    void shiftPageDown()      { m_viewInternal->pageDown(true);      }
    void top()                { m_viewInternal->top_home();          }
    void shiftTop()           { m_viewInternal->top_home(true);      }
    void bottom()             { m_viewInternal->bottom_end();        }
    void shiftBottom()        { m_viewInternal->bottom_end(true);    }
    void toMatchingBracket()  { m_viewInternal->cursorToMatchingBracket();}
    void shiftToMatchingBracket()  { m_viewInternal->cursorToMatchingBracket(true);}

    void gotoLine();
    void gotoLineNumber( int linenumber );

  // config file / session management functions
  public:
    void readSessionConfig(KConfig *);
    void writeSessionConfig(KConfig *);

  public slots:
    int getEol();
    void setEol( int eol );
    void find();
    void find( const QString&, long, bool add=true ); ///< proxy for KateSearch
    void replace();
    void replace( const QString&, const QString &, long ); ///< proxy for KateSearch
    void findAgain( bool back );
    void findAgain()              { findAgain( false );          }
    void findPrev()               { findAgain( true );           }

    void setFoldingMarkersOn( bool enable ); // Not in Kate::View, but should be
    void setIconBorder( bool enable );
    void setLineNumbersOn( bool enable );
    void setScrollBarMarks( bool enable );
    void showCmdLine ( bool enable );
    void toggleFoldingMarkers();
    void toggleIconBorder();
    void toggleLineNumbersOn();
    void toggleScrollBarMarks();
    void toggleDynWordWrap ();
    void toggleCmdLine ();
    void setDynWrapIndicators(int mode);

  public:
    KateRenderer *renderer ();

    bool iconBorder();
    bool lineNumbersOn();
    bool scrollBarMarks();
    int dynWrapIndicators();
    bool foldingMarkersOn();
    Kate::Document* getDoc()    { return m_doc; }

    void setActive( bool b )    { m_active = b; }
    bool isActive()             { return m_active; }

  public slots:
    void gotoMark( KTextEditor::Mark* mark ) { setCursorPositionInternal ( mark->line, 0, 1 ); }
    void selectionChanged ();

  signals:
    void gotFocus( Kate::View* );
    void lostFocus( Kate::View* );
    void newStatus(); // Not in Kate::View, but should be (Kate app connects to it)

  //
  // Extras
  //
  public:
    // Is it really necessary to have 3 methods for this?! :)
    KateDocument*  doc() const       { return m_doc; }

    KActionCollection* editActionCollection() const { return m_editActions; }

  public slots:
    void slotNewUndo();
    void slotUpdate();
    void toggleInsert();
    void reloadFile();
    void toggleWWMarker();
    void toggleWriteLock();
    void switchToCmdLine ();
    void slotReadWriteChanged ();

  signals:
    void dropEventPass(QDropEvent*);
    void viewStatusMsg (const QString &msg);

  public:
    bool setCursorPositionInternal( uint line, uint col, uint tabwidth = 1, bool calledExternally = false );

  protected:
    void contextMenuEvent( QContextMenuEvent* );
    bool checkOverwrite( KURL );

  public slots:
    void slotSelectionTypeChanged();

  private slots:
    void slotGotFocus();
    void slotLostFocus();
    void slotDropEventPass( QDropEvent* ev );
    void slotStatusMsg();
    void slotSaveCanceled( const QString& error );
    void slotExpandToplevel();
    void slotCollapseLocal();
    void slotExpandLocal();

  private:
    void setupConnections();
    void setupActions();
    void setupEditActions();
    void setupCodeFolding();
    void setupCodeCompletion();

    KActionCollection*     m_editActions;
    KAction*               m_editUndo;
    KAction*               m_editRedo;
    KRecentFilesAction*    m_fileRecent;
    KToggleAction*         m_toggleFoldingMarkers;
    KToggleAction*         m_toggleIconBar;
    KToggleAction*         m_toggleLineNumbers;
    KToggleAction*         m_toggleScrollBarMarks;
    KToggleAction*         m_toggleDynWrap;
    KSelectAction*         m_setDynWrapIndicators;
    KToggleAction*         m_toggleWWMarker;
    KAction*         m_switchCmdLine;

    KSelectAction*         m_setEndOfLine;

    Kate::ActionMenu*      m_setHighlight;
    Kate::ActionMenu*      m_setFileType;
    KToggleAction*         m_toggleWriteLock;
    KateViewSchemaAction*  m_schemaMenu;

    KAction *m_cut;
    KAction *m_copy;
    KAction *m_paste;
    KAction *m_selectAll;
    KAction *m_deSelect;

    KToggleAction *m_toggleBlockSelection;
    KToggleAction *m_toggleInsert;

    KateDocument*          m_doc;
    KateViewInternal*      m_viewInternal;
    KateRenderer*          m_renderer;
    KateSearch*            m_search;
    KateBookmarks*         m_bookmarks;
    QGuardedPtr<QPopupMenu>  m_rmbMenu;
    KateCodeCompletion*    m_codeCompletion;

    KateCmdLine *m_cmdLine;
    bool m_cmdLineOn;

    QGridLayout *m_grid;

    bool       m_active;
    bool       m_hasWrap;

  private slots:
    void slotNeedTextHint(int line, int col, QString &text);
    void slotHlChanged();

  /**
   * Configuration
   */
  public:
    inline KateViewConfig *config () { return m_config; };

    void updateConfig ();

    void updateDocumentConfig();

    void updateRendererConfig();

  private slots:
    void updateFoldingConfig ();
    void toggleBlockSelectionMode ();

  private:
    KateViewConfig *m_config;
    bool m_startingUp;
    bool m_updatingDocumentConfig;
};

#endif
