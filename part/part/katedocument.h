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

#include "kateglobal.h"
#include "katesearch.h"
#include "katetextline.h"
#include "../interfaces/document.h"

#include <qobject.h>
#include <qptrlist.h>
#include <qcolor.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qdialog.h>

#include <qintdict.h>
#include <qdatetime.h>
#include <kglobalsettings.h>
#include <kspell.h>

class KateUndo;
class KateUndoGroup;
class KateCmd;
class KateCodeFoldingTree;
class KateBuffer;
class KateView;
class KateViewInternal;

class Attribute {
  public:
    Attribute () { ; };

    QColor col;
    QColor selCol;
    bool bold;
    bool italic;
};

class KateFontMetrics : public QFontMetrics
{
  private:
    short *warray[256];

  public:
    KateFontMetrics(const QFont& f) : QFontMetrics(f)
    {
      for (int i=0; i<256; i++) warray[i]=0;
    }

    ~KateFontMetrics()
    {
      for (int i=0; i<256; i++)
        if (warray[i]) delete[] warray[i];
    }

    int width(QChar c)
    {
      uchar cell=c.cell();
      uchar row=c.row();

      short *wa=warray[row];

      if (!wa)
      {
        wa=warray[row]=new short[256];
        for (int i=0; i<256; i++) wa[i]=-1;
      }
      if (wa[cell]<0) wa[cell]=(short) QFontMetrics::width(c);

      return (int)wa[cell];
    }

    int width(QString s) { return QFontMetrics::width(s); }
};

class FontStruct
{
  public:
    FontStruct():myFont(KGlobalSettings::fixedFont()), myFontBold(KGlobalSettings::fixedFont()),
        myFontItalic(KGlobalSettings::fixedFont()), myFontBI(KGlobalSettings::fixedFont()),
        myFontMetrics (myFont), myFontMetricsBold (myFontBold), myFontMetricsItalic (myFontItalic),
        myFontMetricsBI (myFontBI){;}
  ~FontStruct(){;}

  void updateFontData(int tabChars)
  {
    int maxAscent, maxDescent;
    int tabWidth;
    maxAscent = myFontMetrics.ascent();
    maxDescent = myFontMetrics.descent();
    tabWidth = myFontMetrics.width(' ');

    fontHeight = maxAscent + maxDescent + 1;
    fontAscent = maxAscent;
    m_tabWidth = tabChars*tabWidth;
  };

  QFont myFont, myFontBold, myFontItalic, myFontBI;
  KateFontMetrics myFontMetrics, myFontMetricsBold, myFontMetricsItalic, myFontMetricsBI;
  int m_tabWidth;
  int fontHeight;
  int fontAscent;
};

class KateCursor : public Kate::Cursor
{
  public:
    KateCursor (class KateDocument *doc);
    ~KateCursor ();

    void position ( uint *line, uint *col ) const;

    bool setPosition ( uint line, uint col );

    bool insertText ( const QString& text );

    bool removeText ( uint numberOfCharacters );

    QChar currentChar () const;

  private:
    class KateDocument *myDoc;
};

//
// Kate KTextEditor::Document class (and even KTextEditor::Editor ;)
//
class KateDocument : public Kate::Document
{
  Q_OBJECT
  friend class KateConfigDialog;
  friend class KateViewInternal;
  friend class KateView;
  friend class KateIconBorder;
  friend class KateUndoGroup;
  friend class KateUndo;
  friend class HlManager;
  friend class ColorConfig;

  public:
    KateDocument (bool bSingleViewMode=false, bool bBrowserView=false, bool bReadOnly=false, 
        QWidget *parentWidget = 0, const char *widgetName = 0, QObject * = 0, const char * = 0);
    ~KateDocument ();

  private:
    // only to make part work, don't change it !
    bool m_bSingleViewMode;
    bool m_bBrowserView;
    bool m_bReadOnly;

  //
  // KTextEditor::Document stuff
  //
  public:
    KTextEditor::View *createView( QWidget *parent, const char *name );
    QPtrList<KTextEditor::View> views () const;
    
  private:
    QPtrList<KateView> myViews;
    QPtrList<KTextEditor::View> _views;
    KateView *myActiveView;

  //
  // KTextEditor::EditInterface stuff
  //
  public slots:
    QString text ( uint startLine, uint startCol, uint endLine, uint endCol ) const;
    QString textLine ( uint line ) const;

    bool setText(const QString &);
    bool clear ();

    bool insertText ( uint line, uint col, const QString &s );
    bool removeText ( uint startLine, uint startCol, uint endLine, uint endCol );

    bool insertLine ( uint line, const QString &s );
    bool removeLine ( uint line );

    uint numLines() const;
    uint numVisLines() const;
    uint length () const;
    int lineLength ( uint line ) const;

  signals:
    void textChanged ();
    void charactersInteractivelyInserted(int ,int ,const QString&);

  public:
    //
    // start edit / end edit (start/end undo, cursor update, view update)
    //
    void editStart (bool withUndo = true);
    void editEnd ();

    bool wrapText (uint startLine, uint endLine, uint col);

  private:
    void editAddUndo (KateUndo *undo);
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
    // functions for insert/remove stuff (atomic)
    //
    bool editInsertText ( uint line, uint col, const QString &s );
    bool editRemoveText ( uint line, uint col, uint len );

    bool editWrapLine ( uint line, uint col );
    bool editUnWrapLine ( uint line, uint col);

    bool editInsertLine ( uint line, const QString &s );
    bool editRemoveLine ( uint line );

  //
  // KTextEditor::SelectionInterface stuff
  //
  public slots:
    bool setSelection ( uint startLine, uint startCol, uint endLine, uint endCol );
    bool clearSelection ();

    bool hasSelection () const;
    QString selection () const ;

    bool removeSelectedText ();

    bool selectAll();

  private:
    // some internal functions to get selection state of a line/col
    bool lineColSelected (int line, int col);
    bool lineSelected (int line);
    bool lineEndSelected (int line);
    bool lineHasSelected (int line);

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
  // KTextEditor::MarkInterface stuff
  //
  public slots:
    uint mark (uint line);

    void setMark (uint line, uint markType);
    void clearMark (uint line);

    void addMark (uint line, uint markType);
    void removeMark (uint line, uint markType);

    QPtrList<KTextEditor::Mark> marks ();
    void clearMarks ();

  signals:
    void marksChanged ();

  private:
    QPtrList<KTextEditor::Mark> myMarks;
    bool restoreMarks;

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
    Kate::ConfigPage *kSpellConfigPage (QWidget *);
    Kate::ConfigPage *hlConfigPage (QWidget *);

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
    bool paintTextLine ( QPainter &, uint line, int startcol, int endcol, int y,
                                int xStart, int xEnd, int showCursor, bool replaceCursor,
                                bool showSelections, bool showTabs,WhichFont wf=ViewFont, bool currentLine = false, bool printerfriendly = false);

    uint textWidth(const TextLine::Ptr &, int cursorX,WhichFont wf=ViewFont);
    uint textWidth(const TextLine::Ptr &textLine, uint startcol, uint maxwidth, uint wrapsymwidth, WhichFont wf, bool *needWrap);
    uint textWidth(KateTextCursor &cursor);
    uint textWidth(bool wrapCursor, KateTextCursor &cursor, int xPos,WhichFont wf=ViewFont);
    uint textPos(const TextLine::Ptr &, int xPos,WhichFont wf=ViewFont);
    uint textHeight(WhichFont wf=ViewFont);

    QColor &backCol(int x, int y);
    QColor &cursorCol(int x, int y);

    void setFont (WhichFont wf, QFont font);

    QFont getFont (WhichFont wf);
    KateFontMetrics getFontMetrics (WhichFont wf);

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

    /**
     * get the length in pixels of the given line
     */
    int textLength(int line) const;
    
    TextLine::Ptr kateTextLine(uint i);

    void setTabWidth(int);
    int tabWidth() {return tabChars;}
    void setNewDoc( bool );
    bool isNewDoc() const;

  //
  // KSpell stuff
  //
  public slots:    //please keep prototypes and implementations in same order
    void spellcheck();
    void spellcheck2(KSpell*);
    void misspelling (const QString & word, const QStringList &, unsigned int pos);
    void corrected (const QString & originalword, const QString & newword, unsigned int);
    void spellResult (const QString &newtext);
    void spellCleanDone();
    void tagLines(int start, int end);

  signals:
    /**
     * This says spellchecking is <i>percent</i> done.
     */
    void  spellcheck_progress (unsigned int percent);
    /**
     * Emitted when spellcheck is complete.
     */
    void spellcheck_done ();

  private:
    // all spell checker data stored in here
    struct _kspell {
      KSpell *kspell;
      KSpellConfig *ksc;
      QString spell_tmptext;
      bool kspellon;              // are we doing a spell check?
      int kspellMispellCount;     // how many words suggested for replacement so far
      int kspellReplaceCount;     // how many words actually replaced so far
      bool kspellPristine;        // doing spell check on a clean document?
    } kspell;

   //export feature
   public slots:
   	void exportAs(const QString&);

   private: //the following things should become plugins
   bool exportDocumentToHTML(QTextStream *outputStream,const QString &name);
   QString HTMLEncode(QChar theChar);

  //spell checker
  public:
    /**
     * Returns the KSpellConfig object
     */
    KSpellConfig *ksConfig(void) {return kspell.ksc;}
    /**
     * Sets the KSpellConfig object.  (The object is
     *  copied internally.)
     */
    void setKSConfig (const KSpellConfig _ksc) {*kspell.ksc=_ksc;}

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

    uint currentColumn(KateTextCursor &cursor);
    void newLine(VConfig &);
    void killLine(VConfig &);
    void backspace(uint line, uint col);
    void transpose(KateTextCursor &cursor);
    void del(VConfig &);
    void cut(VConfig &);
    void copy(int flags);
    /**
     * Inserts the text in the clipboard and adds to the cursor
     * of the VConfig object the length of the inserted text.
     */
    void paste(VConfig &);

    void selectTo(VConfig &c, KateTextCursor &cursor, int cXPos);
    void selectWord(KateTextCursor &cursor, int flags);
    void selectLine(KateTextCursor &cursor, int flags);
    void selectLength(KateTextCursor &cursor, int length, int flags);

    void indent(VConfig &c) {doIndent(c, 1);}
    void unIndent(VConfig &c) {doIndent(c, -1);}
    void cleanIndent(VConfig &c) {doIndent(c, 0);}
    // called by indent/unIndent/cleanIndent
    // just does some setup and then calls optimizeLeadingSpace()
    void doIndent(VConfig &, int change);
    // optimize leading whitespace on a single line - see kwdoc.cpp for full description
    void optimizeLeadingSpace(int line, int flags, int change);

    void comment(VConfig &c) {doComment(c, 1);}
    void unComment(VConfig &c) {doComment(c, -1);}
    void doComment(VConfig &, int change);

    QString text() const;
    QString getWord(KateTextCursor &cursor);

  public:
    void tagAll();
    void updateLines(int startLine, int endLine);
    void updateLines();
    void updateViews(int flags = 0);
    void updateEditAccels();

    bool doSearch(SConfig &s, const QString &searchFor);

  public:
    void setPseudoModal(QWidget *);

    void newBracketMark(KateTextCursor &, BracketMark &);

  private:
    void guiActivateEvent( KParts::GUIActivateEvent *ev );

  private:
    //
    // Comment, uncomment methods
    //
    bool removeStringFromBegining(int line, QString &str);
    bool removeStringFromEnd(int line, QString &str);

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

    QString docName () {return myDocName;};

    void setDocName (QString docName);

  public slots:
    /**
     * Reloads the current document from disk if possible
     */
    void reloadFile();

  private slots:
    void slotModChanged ();

  public:
    KateCmd *cmd () { return myCmd; };

  public:
    void setEncoding (QString e) { myEncoding = e; };
    QString encoding() { return myEncoding; };

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

  // for the DCOP interface
  public:
    void open (const QString &name=0);

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
    
    // folding tree
    KateCodeFoldingTree *regionTree;

    QColor colors[3];
    class HlManager *hlManager;
    class Highlight *m_highlight;
    uint m_highlightedTill;
    uint m_highlightedEnd;
    QTimer *m_highlightTimer;

    int eolMode;
    int tabChars;

    bool newDocGeometry;

    bool readOnly;
    bool newDoc;          // True if the file is a new document (used to determine whether
                          // to check for overwriting files on save)
    bool modified;

    bool myWordWrap;
    uint myWordWrapAt;

    bool hlSetByUser;

    QString myEncoding;

    QWidget *pseudoModal;   //the replace prompt is pseudo modal

    /**
     * updates mTime to reflect file on fs.
     * called from constructor and from saveFile.
     */
    void setMTime();
    class QFileInfo* fileInfo;
    class QDateTime mTime;
    QString myDocName;

    class KateCmd *myCmd;

    QMemArray<Attribute> myAttribs;

    //
    // core katedocument config !
    //
    uint _configFlags;

   /**
    * Implementation of the mark interface
    **/
  public:
    virtual void setPixmap(MarkInterface::MarkTypes, const QPixmap &);
    virtual void setDescription(MarkInterface::MarkTypes, const QString &);
    virtual void setMarksUserChangable(uint markMask);

  signals:
    virtual void markChanged (KTextEditor::Mark mark, MarkInterfaceExtension::MarkChangeAction action);
//--------
  private:
	uint m_editableMarks;
  public:
	uint editableMarks();
};

#endif


