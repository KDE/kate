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

#ifndef kate_document_h
#define kate_document_h

#include <ktexteditor/configinterfaceextension.h>
#include <ktexteditor/encodinginterface.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/plugin.h>

#include "katecursor.h"
#include "katebuffer.h"
#include "katefont.h"
#include "katesearch.h"
#include "katetextline.h"
#include "katebrowserextension.h"
#include "../interfaces/document.h"
                                  
#include <ktrader.h>
#include <kservice.h>

#include <qobject.h>
#include <qptrlist.h>
#include <qdialog.h>

#include <qintdict.h>
#include <qdatetime.h>

class KateUndoGroup;
class KateCmd;
class KateCodeFoldingTree;
class KateBuffer;
class KateView;
class KateViewInternal; 

namespace Kate
{
  class PluginInfo
  {
    public:
      bool load;
      KService::Ptr service;
      KTextEditor::Plugin *plugin;
  };

  typedef QPtrList<PluginInfo> PluginList;
}

//
// Kate KTextEditor::Document class (and even KTextEditor::Editor ;)
//
class KateDocument : public Kate::Document, public KTextEditor::ConfigInterfaceExtension,
                     public KTextEditor::EncodingInterface, public KTextEditor::SessionConfigInterface
{
  Q_OBJECT
  friend class KateConfigDialog;
  friend class KateViewInternal;
  friend class KateView;
  friend class KateIconBorder;
  friend class ColorConfig;

  public:    
    KateDocument (bool bSingleViewMode=false, bool bBrowserView=false, bool bReadOnly=false, 
        QWidget *parentWidget = 0, const char *widgetName = 0, QObject * = 0, const char * = 0);
    ~KateDocument ();
    
    bool closeURL(); 
    
    Kate::PluginList *plugins () { return &m_plugins; };
    
    void loadAllEnabledPlugins ();
    void enableAllPluginsGUI (KateView *view);
    
    void loadPlugin (Kate::PluginInfo *item);
    void unloadPlugin (Kate::PluginInfo *item);
    void enablePluginGUI (Kate::PluginInfo *item, KateView *view);
    void enablePluginGUI (Kate::PluginInfo *item);
    void disablePluginGUI (Kate::PluginInfo *item);

  private:
    // only to make part work, don't change it !
    bool m_bSingleViewMode;
    bool m_bBrowserView;
    bool m_bReadOnly;
    KateBrowserExtension *m_extension;
    Kate::PluginList m_plugins;
    
  //
  // KTextEditor::Document stuff
  //
  public:
    KTextEditor::View *createView( QWidget *parent, const char *name );
    QPtrList<KTextEditor::View> views () const;
    
     inline KateView *activeView () const { return m_activeView; }
    
  private:
    QPtrList<KateView> m_views;
    QPtrList<KTextEditor::View> m_textEditViews;
    KateView *m_activeView;

  //
  // KTextEditor::ConfigInterfaceExtension stuff
  //
  public slots:
    uint configPages () const;
    KTextEditor::ConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name=0 );
    QString configPageName (uint number = 0) const;
    QString configPageFullName (uint number = 0) const;   
    QPixmap configPagePixmap (uint number = 0, int size = KIcon::SizeSmall) const;
    
  //
  // KTextEditor::EditInterface stuff
  //
  public slots:
    QString text() const;
  
    QString text ( uint startLine, uint startCol, uint endLine, uint endCol ) const;
    QString text ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise ) const;
    
    QString textLine ( uint line ) const;

    bool setText(const QString &);
    bool clear ();

    bool insertText ( uint line, uint col, const QString &s );
    bool insertText ( uint line, uint col, const QString &s, bool blockwise );
    
    bool removeText ( uint startLine, uint startCol, uint endLine, uint endCol );
    bool removeText ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise );

    bool insertLine ( uint line, const QString &s );
    bool removeLine ( uint line );

    uint numLines() const;
    uint numVisLines() const;
    uint length () const;
    int lineLength ( uint line ) const;

  signals:
    void textChanged ();
    void charactersInteractivelyInserted(int ,int ,const QString&);
    void backspacePressed();

  public:
    //
    // start edit / end edit (start/end undo, cursor update, view update)
    //
    void editStart (bool withUndo = true);
    void editEnd ();
    
    //
    // functions for insert/remove stuff (atomic)
    //
    bool editInsertText ( uint line, uint col, const QString &s );
    bool editRemoveText ( uint line, uint col, uint len );

    bool editWrapLine ( uint line, uint col );
    bool editUnWrapLine ( uint line, uint col);

    bool editInsertLine ( uint line, const QString &s );
    bool editRemoveLine ( uint line );

    bool wrapText (uint startLine, uint endLine, uint col);

  private:
    void editAddUndo (uint type, uint line, uint col, uint len, const QString &text);
    void editTagLine (uint line);
    void editRemoveTagLine (uint line);
    void editInsertTagLine (uint line);

    uint editSessionNumber;
    bool editIsRunning;
    bool noViewUpdates;
    bool editWithUndo;
    uint editTagLineStart;
    uint editTagLineEnd;
    KateUndoGroup *editCurrentUndo;

  //
  // KTextEditor::SelectionInterface stuff
  //
  public slots:
    bool setSelection ( const KateTextCursor & start,
			const KateTextCursor & end );
    bool setSelection ( uint startLine, uint startCol,
			uint endLine, uint endCol );
    bool clearSelection ();

    bool hasSelection () const;
    QString selection () const ;

    bool removeSelectedText ();

    bool selectAll();
    
    //
    // KTextEditor::SelectionInterfaceExt
    //
    int selStartLine() { return selectStart.line; };
    int selStartCol()  { return selectStart.col; };
    int selEndLine()   { return selectEnd.line; };
    int selEndCol()    { return selectEnd.col; };

  private:
    // some internal functions to get selection state of a line/col
    bool lineColSelected (int line, int col);
    bool lineSelected (int line);
    bool lineEndSelected (int line);
    bool lineHasSelected (int line);
    bool lineIsSelection (int line);

    // stores the current selection
    KateTextCursor selectStart;
    KateTextCursor selectEnd;
    
    // only to make the selection from the view easier
    KateTextCursor selectAnchor;

  signals:
    void selectionChanged ();

  //
  // KTextEditor::BlockSelectionInterface stuff
  //
  public slots:
    bool blockSelectionMode ();
    bool setBlockSelectionMode (bool on);
    bool toggleBlockSelectionMode ();
    
  private:
    // do we select normal or blockwise ?
    bool blockSelect;

  //
  // KTextEditor::UndoInterface stuff
  //
  public slots:
    void undo ();
    void redo ();
    void clearUndo ();
    void clearRedo ();

    uint undoCount () const;
    uint redoCount () const;

    uint undoSteps () const;
    void setUndoSteps ( uint steps );

  private:
    //
    // some internals for undo/redo
    //
    QPtrList<KateUndoGroup> undoItems;
    QPtrList<KateUndoGroup> redoItems;
    uint myUndoSteps;

  signals:
    void undoChanged ();

  //
  // KTextEditor::CursorInterface stuff
  //
  public slots:
    KTextEditor::Cursor *createCursor ();
    QPtrList<KTextEditor::Cursor> cursors () const;
    
  private:
    QPtrList<KTextEditor::Cursor> myCursors;

  //
  // KTextEditor::SearchInterface stuff
  //
  public slots:
    bool searchText (unsigned int startLine, unsigned int startCol,
        const QString &text, unsigned int *foundAtLine, unsigned int *foundAtCol,
        unsigned int *matchLen, bool casesensitive = true, bool backwards = false);
    bool searchText (unsigned int startLine, unsigned int startCol,
        const QRegExp &regexp, unsigned int *foundAtLine, unsigned int *foundAtCol,
        unsigned int *matchLen, bool backwards = false);

  //
  // KTextEditor::HighlightingInterface stuff
  //
  public slots:
    uint hlMode ();
    bool setHlMode (uint mode);
    uint hlModeCount ();
    QString hlModeName (uint mode);
    QString hlModeSectionName (uint mode);

  private:
    bool internalSetHlMode (uint mode);
    void setDontChangeHlOnSave();

  signals:
    void hlChanged ();

  //
  // KTextEditor::ConfigInterface stuff
  //
  public slots:
    void readConfig ();
    void writeConfig ();
    void readConfig (KConfig *);
    void writeConfig (KConfig *);
    void readSessionConfig (KConfig *);
    void writeSessionConfig (KConfig *);
    void configDialog ();

  //
  // KTextEditor::MarkInterface and MarkInterfaceExtension
  //
  public slots:
    uint mark( uint line );

    void setMark( uint line, uint markType );
    void clearMark( uint line );

    void addMark( uint line, uint markType );
    void removeMark( uint line, uint markType );

    QPtrList<KTextEditor::Mark> marks();
    void clearMarks();
    
    void setPixmap( MarkInterface::MarkTypes, const QPixmap& );
    void setDescription( MarkInterface::MarkTypes, const QString& );
    QString markDescription( MarkInterface::MarkTypes );
    QPixmap markPixmap( MarkInterface::MarkTypes );
    
    void setMarksUserChangable( uint markMask );
    uint editableMarks();
  
  signals:
    void marksChanged();
    void markChanged( KTextEditor::Mark, KTextEditor::MarkInterfaceExtension::MarkChangeAction );
  
  private:
    QIntDict<KTextEditor::Mark> m_marks;
    QIntDict<QPixmap>           m_markPixmaps;
    QIntDict<QString>           m_markDescriptions;
    bool                        restoreMarks;
    uint                        m_editableMarks;
  
  //
  // KTextEditor::PrintInterface
  //
  public slots:
    bool printDialog ();
    bool print ();

  //
  // KParts::ReadWrite stuff
  //
  public:
    bool openFile ();
    bool saveFile ();

    void setReadWrite( bool );
    bool isReadWrite() const;

    void setModified(bool);
    bool isModified() const;

  //
  // Kate::Document stuff
  //
  public:
    Kate::ConfigPage *colorConfigPage (QWidget *);
    Kate::ConfigPage *fontConfigPage (QWidget *);
    Kate::ConfigPage *indentConfigPage (QWidget *);
    Kate::ConfigPage *selectConfigPage (QWidget *);
    Kate::ConfigPage *editConfigPage (QWidget *);
    Kate::ConfigPage *keysConfigPage (QWidget *);
    Kate::ConfigPage *hlConfigPage (QWidget *);
    Kate::ConfigPage *viewDefaultsConfigPage (QWidget *);
    
    Kate::ActionMenu *hlActionMenu (const QString& text, QObject* parent = 0, const char* name = 0);
    Kate::ActionMenu *exportActionMenu (const QString& text, QObject* parent = 0, const char* name = 0);

  //
  // displaying related stuff
  //
  public:
     // use different fonts for screen and printing
    enum WhichFont
    {
      ViewFont = 1,
      PrintFont = 2
    };

    // ultimate paintLine function (supports startcol/endcol, startx/endx, draw of cursor, tabs + selections)
    bool paintTextLine ( QPainter &, uint line, int startcol, int endcol, int xPos, int y,
                                int xStart, int xEnd, int showCursor, bool replaceCursor, int cursorXPos,
                                bool showSelections, bool showTabs,WhichFont wf=ViewFont, bool currentLine = false,
                                bool printerfriendly = false, const BracketMark& bm = BracketMark() );

    uint textWidth(const TextLine::Ptr &, int cursorX,WhichFont wf=ViewFont);
    uint textWidth(const TextLine::Ptr &textLine, uint startcol, uint maxwidth, uint wrapsymwidth, WhichFont wf, bool *needWrap);
    uint textWidth(KateTextCursor &cursor);
    uint textWidth(KateTextCursor &cursor, int xPos,WhichFont wf=ViewFont);
    uint textPos(const TextLine::Ptr &, int xPos,WhichFont wf=ViewFont);
    uint textHeight(WhichFont wf=ViewFont);

    QColor &backCol(int x, int y);
    QColor &cursorCol(int x, int y);

    const FontStruct& getFontStruct( WhichFont wf );
    void setFont( WhichFont wf, QFont font );
    const QFont& getFont( WhichFont wf );
    const KateFontMetrics& getFontMetrics( WhichFont wf );

    bool selectBounds(uint line, uint &start, uint &end, uint lineLength);

  private:
    // fonts structures for the view + printing font
    FontStruct viewFont;
    FontStruct printFont;

  public:
    //
    // internal edit stuff (mostly for view)
    //
    bool insertChars ( int line, int col, const QString &chars, KateView *view );

    /**
     * gets the last line number (numLines() -1)
     */
    uint lastLine() const { return numLines()-1;}
    
    TextLine::Ptr kateTextLine(uint i);

    void setTabWidth(int);
    int tabWidth() {return tabChars;}
    void setNewDoc( bool );
    bool isNewDoc() const;
    
    /**
       Tag the lines in the current selection.
     */
    void tagSelection();

  public slots:    //please keep prototypes and implementations in same order
    void tagLines(int start, int end);

   //export feature
   public slots:
   	void exportAs(const QString&);

   private: //the following things should become plugins
   bool exportDocumentToHTML(QTextStream *outputStream,const QString &name);
   QString HTMLEncode(QChar theChar);

  signals:
    void modifiedChanged ();
    void preHighlightChanged(uint);

  private:
    Attribute *attribute (uint pos);

  public:
    class Highlight *highlight() { return m_highlight; }

  private:
    void makeAttribs();
    void updateFontData();

  private slots:
    void internalHlChanged();
    void slotLoadingFinished();

  public:
    void addView(KTextEditor::View *);
    void removeView(KTextEditor::View *);

    void addCursor(KTextEditor::Cursor *);
    void removeCursor(KTextEditor::Cursor *);

    bool ownedView(KateView *);
    bool isLastView(int numViews);

    uint currentColumn( const KateTextCursor& );
    void newLine(             KateTextCursor& ); // Changes input
    void backspace(     const KateTextCursor& );
    void del(           const KateTextCursor& );
    void transpose(     const KateTextCursor& );
    void cut();
    void copy();
    void paste( const KateTextCursor& cursor, KateView* view );

    void selectTo(     const KateTextCursor& from, const KateTextCursor& to );
    void selectWord(   const KateTextCursor& cursor );
    void selectLine(   const KateTextCursor& cursor );
    void selectLength( const KateTextCursor& cursor, int length );

    void indent(      uint line ) { doIndent( line,  1 );  }
    void unIndent(    uint line ) { doIndent( line, -1 );  }
    void cleanIndent( uint line ) { doIndent( line,  0 );  }
    void comment(     uint line ) { doComment( line,  1 ); }
    void unComment(   uint line ) { doComment( line, -1 ); }
    private:
    void doIndent( uint line, int change );
    void optimizeLeadingSpace( uint line, int flags, int change );
    void doComment( uint line, int change );
    public:

    QString getWord( const KateTextCursor& cursor );

  public:
    void tagAll();
    void updateLines(int startLine, int endLine);
    void updateLines();
    void updateViews();

    void newBracketMark( const KateTextCursor& start, BracketMark& );
    bool findMatchingBracket( KateTextCursor& start, KateTextCursor& end );

  private:
    void guiActivateEvent( KParts::GUIActivateEvent *ev );

  private:
    //
    // Comment, uncomment methods
    //
    bool removeStringFromBegining(int line, QString &str);
    bool removeStringFromEnd(int line, QString &str);

    bool nextNonSpaceCharPos(int &line, int &col);
    bool previousNonSpaceCharPos(int &line, int &col);

    void addStartLineCommentToSingleLine(int line);
    bool removeStartLineCommentFromSingleLine(int line);

    void addStartStopCommentToSingleLine(int line);
    bool removeStartStopCommentFromSingleLine(int line);

    void addStartStopCommentToSelection();
    void addStartLineCommentToSelection();

    bool removeStartStopCommentFromSelection();
    bool removeStartLineCommentFromSelection();

  private slots:
    void slotBufferChanged();
    void slotBufferUpdateHighlight(uint,uint);
    void slotBufferUpdateHighlight();

  public:
    /**
     * Checks if the file on disk is newer than document contents.
     * If forceReload is true, the document is reloaded without asking the user,
     * otherwise [default] the user is asked what to do.
     */
    void isModOnHD(bool forceReload=false);

    QString docName () {return m_docName;};

    void setDocName (QString docName);
		
		inline void lineInfo (KateLineInfo *info, unsigned int line)
	 {
	   buffer->lineInfo(info,line);
	 }
	 
	 inline KateCodeFoldingTree *foldingTree ()
	 {
	   return buffer->foldingTree();
	 }

  public slots:
    /**
     * Reloads the current document from disk if possible
     */
    void reloadFile();

  private slots:
    void slotModChanged ();

  public:
    KateCmd *cmd () { return myCmd; };

  public slots:
    void setEncoding (const QString &e) { myEncoding = e; };
    QString encoding() const { return myEncoding; };

  public slots:
    void setWordWrap (bool on);
    bool wordWrap () { return myWordWrap; };

    void setWordWrapAt (uint col);
    uint wordWrapAt () { return myWordWrapAt; };

  signals:
    void modStateChanged (Kate::Document *doc);
    void nameChanged (Kate::Document *doc);

  public slots:
    // clear buffer/filename - update the views
    void flush ();

  signals:
    /**
     * The file has been saved (perhaps the name has changed). The main window
     * can use this to change its caption
     */
    void fileNameChanged ();

  public:
    //end of line settings
    enum Eol_settings {eolUnix=0,eolDos=1,eolMacintosh=2};

  public:
    // wrap the text of the document at the column col
    void wrapText (uint col);

  public slots:
     void applyWordWrap ();

  public:
    uint configFlags ();
    void setConfigFlags (uint flags);

 // code folding
  public:
        unsigned int getRealLine(unsigned int virtualLine);
        unsigned int getVirtualLine(unsigned int realLine);
        unsigned int visibleLines ();

  signals:
	void codeFoldingUpdated();
  public slots:
	void dumpRegionTree();

  //
  // Some flags for internal ONLY use !
  //
  public:

    // result flags for katepart's internal dialogs
    enum DialogResults
    {
      srYes=QDialog::Accepted,
      srNo=10,
      srAll,
      srCancel=QDialog::Rejected
    };

  //
  // REALLY internal data ;)
  //
  private:
    // text buffer
    KateBuffer *buffer;

    QColor colors[4];
    class HlManager *hlManager;
    class Highlight *m_highlight;
    uint m_highlightedTill;
    uint m_highlightedEnd;
    QTimer *m_highlightTimer;

    int eolMode;
    int tabChars;

    bool readOnly;
    bool newDoc;          // True if the file is a new document (used to determine whether
                          // to check for overwriting files on save)
    bool modified;

    bool myWordWrap;
    uint myWordWrapAt;

    bool hlSetByUser;

    QString myEncoding;
       
    /**
     * updates mTime to reflect file on fs.
     * called from constructor and from saveFile.
     */
    void setMTime();
    class QFileInfo* fileInfo;
    class QDateTime mTime;
    QString m_docName;

    class KateCmd *myCmd;

    QMemArray<Attribute> myAttribs;
    
    //
    // core katedocument config !
    //
    uint _configFlags;

  public: 
    /**
      Allow the HlManager to fill the array
    */
    QMemArray<Attribute> *attribs() { return &myAttribs; }
};

#endif


