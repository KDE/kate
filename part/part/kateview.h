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

#include "../interfaces/view.h"

#include "kateglobal.h"
#include "katedocument.h"
#include "katesearch.h"
#include "kateviewinternal.h"
#include "katecodecompletion_iface_impl.h"

class KToggleAction;
class KAction;
class KRecentFilesAction;
class KSelectAction;
class KateDocument;
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
    KateView( KateDocument* doc, QWidget* parent = 0L, const char* name = 0 );
    ~KateView ();

  //
  // KTextEditor::View
  //
  public:
    KTextEditor::Document* document() const       { return myDoc; }

  //
  // KTextEditor::ClipboardInterface
  //
  public slots:
    void cut()           { myDoc->cut();    }
    void copy() const    { myDoc->copy();   }
    // TODO: Factor out of myViewInternal
    void paste()         { myViewInternal->doPaste();  }

  //
  // KTextEditor::PopupMenuInterface
  //
  public:
    void installPopup( QPopupMenu* menu ) { m_rmbMenu = menu; }
    QPopupMenu* popup() const              { return m_rmbMenu;     }
    
  //
  // KTextEditor::ViewCursorInterface
  //
  public slots:
    QPoint cursorCoordinates()
        { return myViewInternal->cursorCoordinates();                 }
    void cursorPosition( uint* l, uint* c )
        { if( l ) *l = cursorLine(); if( c ) *c = cursorColumn();     }
    void cursorPositionReal( uint* l, uint* c )
        { if( l ) *l = cursorLine(); if( c ) *c = cursorColumnReal(); }
    bool setCursorPosition( uint line, uint col )
        { return setCursorPositionInternal( line, col, tabWidth() );  }
    bool setCursorPositionReal( uint line, uint col)
        { return setCursorPositionInternal( line, col, 1 );           }
    uint cursorLine()
        { return myViewInternal->getCursor().line;                    }
    uint cursorColumn()
        { return myDoc->currentColumn(myViewInternal->getCursor());   }
    uint cursorColumnReal()
        { return myViewInternal->getCursor().col;                     }
  signals:
    void cursorPositionChanged();

  //
  // KTextEditor::CodeCompletionInterface
  //
  public:
    void showArgHint( QStringList arg1, const QString& arg2, const QString& arg3 )
        { myCC_impl->showArgHint( arg1, arg2, arg3 ); }
    void showCompletionBox( QValueList<KTextEditor::CompletionEntry> arg1, int offset = 0, bool cs = true )
        { myCC_impl->showCompletionBox( arg1, offset, cs ); }
  signals:
    void completionAborted();
    void completionDone();
    void argHintHidden();
    void completionDone(KTextEditor::CompletionEntry);
    void filterInsertString(KTextEditor::CompletionEntry*,QString *);

  //
  // KTextEditor::DynWordWrapInterface
  //
  public:
    // These don't actually do anything!
    void setDynWordWrap( bool b ) { m_hasWrap = b; }
    bool dynWordWrap() const      { return m_hasWrap; }

  //
  // Kate::View
  //
  public:
    bool isOverwriteMode() const
        { return myDoc->_configFlags & KateDocument::cfOvr; }
    void setOverwriteMode( bool b );
    // TODO: As this method can be implemented in terms of KTextEditor
    // methods, it should be dropped.
    QString currentTextLine()
        { return getDoc()->textLine( cursorLine() ); }
    QString currentWord()
        { return myDoc->getWord( myViewInternal->getCursor() ); }
//    QString word(int x, int y)
//        { return myDoc->getWord( x, y ); }
    // TODO: As this method can be implemented in terms of KTextEditor
    // methods, it should be dropped.
    void insertText( const QString& text )
        { getDoc()->insertText( cursorLine(), cursorColumnReal(), text ); }
    bool canDiscard();
    int tabWidth()                { return myDoc->tabChars;      }
    void setTabWidth( int w )     { myDoc->setTabWidth(w);       }
    void setEncoding( QString e ) { myDoc->setEncoding(e);       }
    bool isLastView()             { return myDoc->isLastView(1); }

  public slots:
    void flush();
    saveResult save();
    saveResult saveAs();
    
    void indent()      { myDoc->indent( cursorLine() );      }
    void unIndent()    { myDoc->unIndent( cursorLine() );    }
    void cleanIndent() { myDoc->cleanIndent( cursorLine() ); }
    void comment()     { myDoc->comment( cursorLine() );     }
    void uncomment()   { myDoc->unComment( cursorLine() );   }
    // TODO: Factor these out of myViewInternal
    void keyReturn()   { myViewInternal->doReturn();      }
    void keyDelete()   { myViewInternal->doDelete();      }
    void backspace()   { myViewInternal->doBackspace();   }
    void transpose()   { myViewInternal->doTranspose();   }
    void killLine()    { myDoc->killLine( cursorLine() );    }
    
    void cursorLeft()         { myViewInternal->cursorLeft();      }
    void shiftCursorLeft()    { myViewInternal->cursorLeft(true);  }
    void cursorRight()        { myViewInternal->cursorRight();     }
    void shiftCursorRight()   { myViewInternal->cursorRight(true); }
    void wordLeft()           { myViewInternal->wordLeft();        }
    void shiftWordLeft()      { myViewInternal->wordLeft(true);    }
    void wordRight()          { myViewInternal->wordRight();       }
    void shiftWordRight()     { myViewInternal->wordRight(true);   }
    void home()               { myViewInternal->home();            }
    void shiftHome()          { myViewInternal->home(true);        }
    void end()                { myViewInternal->end();             }
    void shiftEnd()           { myViewInternal->end(true);         }
    void up()                 { myViewInternal->cursorUp();        }
    void shiftUp()            { myViewInternal->cursorUp(true);    }
    void down()               { myViewInternal->cursorDown();      }
    void shiftDown()          { myViewInternal->cursorDown(true);  }
    void scrollUp()           { myViewInternal->scrollUp();        }
    void scrollDown()         { myViewInternal->scrollDown();      }
    void topOfView()          { myViewInternal->topOfView();       }
    void bottomOfView()       { myViewInternal->bottomOfView();    }
    void pageUp()             { myViewInternal->pageUp();          }
    void shiftPageUp()        { myViewInternal->pageUp(true);      }
    void pageDown()           { myViewInternal->pageDown();        }
    void shiftPageDown()      { myViewInternal->pageDown(true);    }
    void top()                { myViewInternal->top_home();        }
    void shiftTop()           { myViewInternal->top_home(true);    }
    void bottom()             { myViewInternal->bottom_end();      }
    void shiftBottom()        { myViewInternal->bottom_end(true);  }

    void gotoLine();
    void gotoLineNumber( int linenumber );

  // config file / session management functions
  public:
    void readSessionConfig(KConfig *);
    void writeSessionConfig(KConfig *);

  public slots:
    int getEol()                  { return myDoc->eolMode; }
    void setEol( int eol );
    void setFocus();
    
    void find()                   { m_search->find();            }
    void replace()                { m_search->replace();         }
    void findAgain( bool back )   { m_search->findAgain( back ); }
    void findAgain()              { findAgain( false );          }
    void findPrev()               { findAgain( true );           }
    void slotEditCommand();
    void setIconBorder( bool enable );
    void toggleIconBorder();
    void setLineNumbersOn( bool enable );
    void toggleLineNumbersOn();

  public:
    bool iconBorder();
    bool lineNumbersOn();
    Kate::Document* getDoc()    { return myDoc; }
    
    void setActive( bool b )    { m_active = b; }
    bool isActive()             { return m_active; }
    
  public slots:
    void slotIncFontSizes();
    void slotDecFontSizes();
    void gotoMark( KTextEditor::Mark* mark ) { setCursorPositionReal( mark->line, 0 ); }

  //
  // Extras
  //
  public:
    int iconBorderStatus() const     { return m_iconBorderStatus; }
    // Is it really necessary to have 3 methods for this?! :)
    KateDocument*  doc() const       { return myDoc; }
    void setupEditKeys();

  public slots:
    void setFoldingMarkersOn(bool enable);
    void slotNewUndo();
    void slotUpdate();
    void toggleInsert();
    void slotRegionVisibilityChangedAt(unsigned int);
    void slotRegionBeginEndAddedRemoved(unsigned int);
    void slotCodeFoldingChanged();
    void reloadFile();

  signals:
    void gotFocus (Kate::View *);
    void dropEventPass(QDropEvent*);
    void newStatus();
    
  protected:
    void keyPressEvent( QKeyEvent *ev );
    void customEvent( QCustomEvent *ev );
    void contextMenuEvent( QContextMenuEvent *ev );
    void resizeEvent( QResizeEvent* );
    bool eventFilter( QObject* o, QEvent* e );
    int checkOverwrite( KURL u );

  private slots:
    void slotDropEventPass( QDropEvent* ev );
    void slotSetEncoding( const QString& descriptiveName );

  private:
    KAccel* createEditKeys();
    void setupActions();
    void initCodeCompletionImplementation();

    void updateIconBorder();
    
    void doCursorCommand( int cmdNum );
    void doEditCommand( int cmdNum );
    bool setCursorPositionInternal( uint line, uint col, uint tabwidth );

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
    CodeCompletion_Impl*   myCC_impl;

    bool       m_active;
    int        m_iconBorderStatus;
    bool       m_hasWrap;
};

#endif
