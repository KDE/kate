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

#include "katedocument.h"
#include "katesearch.h"
#include "kateviewinternal.h"
#include "katecodecompletion.h"

#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/viewstatusmsginterface.h>
#include <ktexteditor/plugin.h>

class KToggleAction;
class KAction;
class KRecentFilesAction;
class KSelectAction;
class KateDocument;
class KateBookmarks;

//
// Kate KTextEditor::View class ;)
//
class KateView : public Kate::View, public KTextEditor::SessionConfigInterface,
                            public KTextEditor::ViewStatusMsgInterface
{
    Q_OBJECT
    friend class KateViewInternal;
    friend class KateDocument;
    friend class KateUndoGroup;
    friend class KateUndo;
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
    void cut()           { m_doc->cut();    }
    void copy() const    { m_doc->copy();   }
    // TODO: Factor out of m_viewInternal
    void paste()         { m_viewInternal->doPaste();  }

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
        { return setCursorPositionInternal( line, col, tabWidth() );  }
    bool setCursorPositionReal( uint line, uint col)
        { return setCursorPositionInternal( line, col, 1 );           }
    uint cursorLine()
        { return m_viewInternal->getCursor().line;                    }
    uint cursorColumn()
        { return m_doc->currentColumn(m_viewInternal->getCursor());   }
    uint cursorColumnReal()
        { return m_viewInternal->getCursor().col;                     }
  signals:
    void cursorPositionChanged();

  //
  // KTextEditor::CodeCompletionInterface
  //
  public:
    void showArgHint( QStringList arg1, const QString& arg2, const QString& arg3 )
        { m_codeCompletion->showArgHint( arg1, arg2, arg3 ); }
    void showCompletionBox( QValueList<KTextEditor::CompletionEntry> arg1, int offset = 0, bool cs = true )
        { m_codeCompletion->showCompletionBox( arg1, offset, cs ); }
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
        { return m_doc->_configFlags & KateDocument::cfOvr; }
    void setOverwriteMode( bool b );
    // TODO: As this method can be implemented in terms of KTextEditor
    // methods, it should be dropped.
    QString currentTextLine()
        { return getDoc()->textLine( cursorLine() ); }
    QString currentWord()
        { return m_doc->getWord( m_viewInternal->getCursor() ); }
//    QString word(int x, int y)
//        { return m_doc->getWord( x, y ); }
    // TODO: As this method can be implemented in terms of KTextEditor
    // methods, it should be dropped.
    void insertText( const QString& text )
        { getDoc()->insertText( cursorLine(), cursorColumnReal(), text ); }
    bool canDiscard();
    int tabWidth()                { return m_doc->tabChars;      }
    void setTabWidth( int w )     { m_doc->setTabWidth(w);       }
    void setEncoding( QString e ) { m_doc->setEncoding(e);       }
    bool isLastView()             { return m_doc->isLastView(1); }

  public slots:
    void flush();
    saveResult save();
    saveResult saveAs();
    
    void indent()             { m_doc->indent( cursorLine() );       }
    void unIndent()           { m_doc->unIndent( cursorLine() );     }
    void cleanIndent()        { m_doc->cleanIndent( cursorLine() );  }
    void comment()            { m_doc->comment( cursorLine() );      }
    void uncomment()          { m_doc->unComment( cursorLine() );    }
    void killLine()           { m_doc->killLine( cursorLine() );     }
    
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
    void bottomOfView()       { m_viewInternal->bottomOfView();      }
    void pageUp()             { m_viewInternal->pageUp();            }
    void shiftPageUp()        { m_viewInternal->pageUp(true);        }
    void pageDown()           { m_viewInternal->pageDown();          }
    void shiftPageDown()      { m_viewInternal->pageDown(true);      }
    void top()                { m_viewInternal->top_home();          }
    void shiftTop()           { m_viewInternal->top_home(true);      }
    void bottom()             { m_viewInternal->bottom_end();        }
    void shiftBottom()        { m_viewInternal->bottom_end(true);    }

    void gotoLine();
    void gotoLineNumber( int linenumber );

  // config file / session management functions
  public:
    void readSessionConfig(KConfig *);
    void writeSessionConfig(KConfig *);  

  public slots:
    int getEol()                  { return m_doc->eolMode; }
    void setEol( int eol );
    void find()                   { m_search->find();            }
    void replace()                { m_search->replace();         }
    void findAgain( bool back )   { m_search->findAgain( back ); }
    void findAgain()              { findAgain( false );          }
    void findPrev()               { findAgain( true );           }
    void slotEditCommand();

    void setFoldingMarkersOn( bool enable ); // Not in Kate::View, but should be
    void setIconBorder( bool enable );
    void setLineNumbersOn( bool enable );
    void toggleFoldingMarkers();
    void toggleIconBorder();
    void toggleLineNumbersOn();

  public:
    bool iconBorder();
    bool lineNumbersOn();
    bool foldingMarkersOn();
    Kate::Document* getDoc()    { return m_doc; }
    
    void setActive( bool b )    { m_active = b; }
    bool isActive()             { return m_active; }
    
  public slots:
    void slotIncFontSizes();
    void slotDecFontSizes();
    void gotoMark( KTextEditor::Mark* mark ) { setCursorPositionReal( mark->line, 0 ); }

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

  public slots:
    void slotNewUndo();
    void slotUpdate();
    void toggleInsert();
    void reloadFile();

  signals:
    void dropEventPass(QDropEvent*);
    void viewStatusMsg (const QString &msg);
    
  protected:
    void customEvent( QCustomEvent* );
    void contextMenuEvent( QContextMenuEvent* );
    void resizeEvent( QResizeEvent* );
    int checkOverwrite( KURL );

  private slots:
    void slotGotFocus();
    void slotLostFocus();
    void slotDropEventPass( QDropEvent* ev );
    void slotSetEncoding( const QString& descriptiveName );
    void updateFoldingMarkersAction();
    void slotStatusMsg ();

  private:
    void setupConnections();
    void setupActions();
    void setupEditActions();
    void setupCodeFolding();
    void setupCodeCompletion();
    void setupViewPlugins();
    
    bool setCursorPositionInternal( uint line, uint col, uint tabwidth );

    KActionCollection*     m_editActions;
    KAction*               m_editUndo;
    KAction*               m_editRedo;
    KRecentFilesAction*    m_fileRecent; 
    KToggleAction*         m_toggleFoldingMarkers;
    KSelectAction*         m_setEndOfLine;
    KSelectAction*         m_setEncoding;
    Kate::ActionMenu*      m_setHighlight;

    KateDocument*          m_doc;
    KateViewInternal*      m_viewInternal;
    KateSearch*            m_search;
    KateBookmarks*         m_bookmarks;
    QPopupMenu*            m_rmbMenu;
    KateCodeCompletion*    m_codeCompletion;

    bool       m_active;
    bool       m_hasWrap;
};

#endif
