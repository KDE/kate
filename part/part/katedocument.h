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

#include "kateautoindent.h"
#include "katecursor.h"
#include "katetextline.h"

#include "../interfaces/document.h"

#include <ktexteditor/configinterfaceextension.h>
#include <ktexteditor/encodinginterface.h>
#include <ktexteditor/sessionconfiginterface.h>

#include <kservice.h>
#include <dcopobject.h>

#include <qintdict.h>
#include <qdatetime.h>

namespace KTextEditor { class Plugin; }

class KateUndoGroup;
class KateCmd;
class KateAttribute;
class KateCodeFoldingTree;
class KateBuffer;
class KateView;
class KateViewInternal;
class LineRange;
class KateArbitraryHighlight;
class KateSuperRange;
class KateLineInfo;
class KateBrowserExtension;
class KateDocumentConfig;

class KSpell;

class QTimer;

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
class KateDocument : public Kate::Document,
                     public KTextEditor::ConfigInterfaceExtension,
                     public KTextEditor::EncodingInterface, public KTextEditor::SessionConfigInterface,
                     public DCOPObject
{
  K_DCOP
  Q_OBJECT

  friend class KateViewInternal;
  friend class KateView;
  friend class ViewDefaultsConfig;
  friend class PluginConfigPage;
  friend class KateRenderer;

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
    static Kate::PluginList s_plugins;
    Kate::PluginList m_plugins;

  //
  // KTextEditor::Document stuff
  //
  public:
    KTextEditor::View *createView( QWidget *parent, const char *name );
    QPtrList<KTextEditor::View> views () const;

     KateView *activeView () const { return m_activeView; }

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

    bool editMarkLineAutoWrapped ( uint line, bool autowrapped );

    bool editWrapLine ( uint line, uint col, bool newLine = true );
    bool editUnWrapLine ( uint line, bool removeLine = true, uint length = 0 );

    bool editInsertLine ( uint line, const QString &s );
    bool editRemoveLine ( uint line );

    bool wrapText (uint startLine, uint endLine, uint col);

  signals:
    /**
     * Emitted each time text is inserted into a pre-existing line, including appends.
     * Does not include newly inserted lines at the moment. ?need
     */
    void editTextInserted ( uint line, uint col, uint len);

    /**
     * Emitted each time text is removed from a line, including truncates and space removal.
     */
    void editTextRemoved ( uint line, uint col, uint len);

    /**
     * Emmitted when text from @p line was wrapped at position pos onto line @p nextLine.
     */
    void editLineWrapped ( uint line, uint col, uint len );

    /**
     * Emitted each time text from @p nextLine was upwrapped onto @p line.
     */
    void editLineUnWrapped ( uint line, uint col );

    /**
     * Emitted whenever a line is inserted before @p line, becoming itself line @ line.
     */
    void editLineInserted ( uint line );

    /**
     * Emitted when a line is deleted.
     */
    void editLineRemoved ( uint line );

  private:
    void undoStart();
    void undoEnd();

  private slots:
    void undoCancel();

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
    bool editTagFrom;
    KateUndoGroup* m_editCurrentUndo;

  //
  // KTextEditor::SelectionInterface stuff
  //
  public slots:
    bool setSelection ( const KateTextCursor & start,
      const KateTextCursor & end );
    bool setSelection ( uint startLine, uint startCol,
      uint endLine, uint endCol );
    bool clearSelection ();
    bool clearSelection (bool redraw);

    bool hasSelection () const;
    QString selection () const ;

    bool removeSelectedText ();

    bool selectAll();

    //
    // KTextEditor::SelectionInterfaceExt
    //
    int selStartLine() { return selectStart.line(); };
    int selStartCol()  { return selectStart.col(); };
    int selEndLine()   { return selectEnd.line(); };
    int selEndCol()    { return selectEnd.col(); };

  private:
    // some internal functions to get selection state of a line/col
    bool lineColSelected (int line, int col);
    bool lineSelected (int line);
    bool lineEndSelected (int line, int endCol);
    bool lineHasSelected (int line);
    bool lineIsSelection (int line);

    // stores the current selection
    KateTextCursor selectStart;
    KateTextCursor selectEnd;
    KateTextCursor oldSelectStart;
    KateTextCursor oldSelectEnd;

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
    bool m_undoDontMerge;
    bool m_undoIgnoreCancel;
    QTimer* m_undoMergeTimer;
    // these two variables are for resetting the document to
    // non-modified if all changes have been undone...
    KateUndoGroup* lastUndoGroupWhenSaved;
    bool docWasSavedWhenUndoWasEmpty;

    // this sets
    void updateModified();

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
  // Kate::ArbitraryHighlightingInterface stuff
  //
  public:
    KateArbitraryHighlight* arbitraryHL() const { return m_arbitraryHL; };

  private slots:
    void tagArbitraryLines(KateView* view, KateSuperRange* range);

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
    QPixmap *markPixmap( MarkInterface::MarkTypes );
    QColor markColor( MarkInterface::MarkTypes );

    void setMarksUserChangable( uint markMask );
    uint editableMarks();

  signals:
    void marksChanged();
    void markChanged( KTextEditor::Mark, KTextEditor::MarkInterfaceExtension::MarkChangeAction );

  private:
    QIntDict<KTextEditor::Mark> m_marks;
    QIntDict<QPixmap>           m_markPixmaps;
    QIntDict<QString>           m_markDescriptions;
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
    /* Anders:
      I reimplemented this, since i need to check if backup succeeded
      if requested */
    bool save();

    bool openFile ();
    bool saveFile ();

    void setReadWrite ( bool rw = true );

    void setModified( bool m );

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
    Kate::ConfigPage *saveConfigPage( QWidget * );

    Kate::ActionMenu *hlActionMenu (const QString& text, QObject* parent = 0, const char* name = 0);
    Kate::ActionMenu *exportActionMenu (const QString& text, QObject* parent = 0, const char* name = 0);

  public:
    /**
     * Type chars in a view
     */
    bool typeChars ( KateView *type, const QString &chars );

    /**
     * gets the last line number (numLines() -1)
     */
    uint lastLine() const { return numLines()-1;}

    TextLine::Ptr kateTextLine(uint i);

    uint configFlags ();
    void setConfigFlags (uint flags);

    /**
       Tag the lines in the current selection.
     */
    void tagSelection();

    // Repaint all of all of the views
    void repaintViews(bool paintOnlyDirty = true);

  public slots:    //please keep prototypes and implementations in same order
    void tagLines(int start, int end);
    void tagLines(KateTextCursor start, KateTextCursor end);

   //export feature
   public slots:
     void exportAs(const QString&);

   private: //the following things should become plugins
   bool exportDocumentToHTML(QTextStream *outputStream,const QString &name);
   QString HTMLEncode(QChar theChar);

  signals:
    void modifiedChanged ();
    void preHighlightChanged(uint);

  public:
    class Highlight *highlight() { return m_highlight; }

  private slots:
    void internalHlChanged();
    void slotLoadingFinished();

  public:
    void addView(KTextEditor::View *);
    void removeView(KTextEditor::View *);

    void addSuperCursor(class KateSuperCursor *, bool privateC);
    void removeSuperCursor(class KateSuperCursor *, bool privateC);

    bool ownedView(KateView *);
    bool isLastView(int numViews);

    uint currentColumn( const KateTextCursor& );
    void newLine(             KateTextCursor&, KateViewInternal * ); // Changes input
    void backspace(     const KateTextCursor& );
    void del(           const KateTextCursor& );
    void transpose(     const KateTextCursor& );
    void cut();
    void copy();
    void paste ( KateView* view );

    void selectTo(     const KateTextCursor& from, const KateTextCursor& to );
    void selectWord(   const KateTextCursor& cursor );
    void selectLine(   const KateTextCursor& cursor );
    void selectLength( const KateTextCursor& cursor, int length );

  public:
    void insertIndentChars ( KateView *view );

    void indent ( KateView *view, uint line, int change );
    void comment ( KateView *view, uint line, int change );

    enum TextTransform { Uppercase, Lowercase, Capitalize };

    /**
      Handling uppercase, lowercase and capitalize for the view.

      If there is a selection, that is transformed, otherwise for uppercase or
      lowercase the character right of the cursor is transformed, for capitalize
      the word under the cursor is transformed.
    */
    void transform ( KateView *view, const KateTextCursor &, TextTransform );
    /**
      Unwrap a range of lines.
    */
    void joinLines( uint first, uint last );

  private:
    void optimizeLeadingSpace( uint line, int flags, int change );
    void replaceWithOptimizedSpace( uint line, uint upto_column, uint space, int flags );

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

  public:
    QString getWord( const KateTextCursor& cursor );

  public:
    void tagAll();
    void updateLines(int startLine, int endLine);
    void updateLines();
    void updateViews();

    void newBracketMark( const KateTextCursor& start, KateTextRange& bm );
    bool findMatchingBracket( KateTextCursor& start, KateTextCursor& end );

  private:
    void guiActivateEvent( KParts::GUIActivateEvent *ev );

  private slots:
    void slotBufferChanged();

  public:
    /**
     * Checks if the file on disk is newer than document contents.
     * If forceReload is true, the document is reloaded without asking the user,
     * otherwise [default] the user is asked what to do.
     */
    void isModOnHD(bool forceReload=false);

    QString docName () {return m_docName;};

    void setDocName (QString docName);

    void lineInfo (KateLineInfo *info, unsigned int line);

    KateCodeFoldingTree *foldingTree ();

  public slots:
    /**
     * Reloads the current document from disk if possible
     */
    void reloadFile();

  public slots:
    void setEncoding (const QString &e);
    QString encoding() const;

  public slots:
    void setWordWrap (bool on);
    bool wordWrap ();

    void setWordWrapAt (uint col);
    uint wordWrapAt ();

  public slots:
    void setPageUpDownMovesCursor(bool on);
    bool pageUpDownMovesCursor();

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
    // wrap the text of the document at the column col
    void wrapText (uint col);

  public slots:
     void applyWordWrap ();

  public:
    enum GetSearchTextFrom
    {
      Nowhere, SelectionOnly, SelectionWord, WordOnly, WordSelection
    };

    void setGetSearchTextFrom (int where);
    int getSearchTextFrom() const;

 // code folding
  public:
    unsigned int getRealLine(unsigned int virtualLine);
    unsigned int getVirtualLine(unsigned int realLine);
    unsigned int visibleLines ();

  signals:
    void codeFoldingUpdated();

  public slots:
    void dumpRegionTree();

  private slots:
    void slotModOnHdDirty (const QString &path);
    void slotModOnHdCreated (const QString &path);
    void slotModOnHdDeleted (const QString &path);

  public:
    // should cursor be wrapped ? take config + blockselection state in account
    bool wrapCursor ();

  public:
    void updateFileType (int newType, bool user = false);

    int fileType () const { return m_fileType; };

  //
  // REALLY internal data ;)
  //
  private:
    // text buffer
    KateBuffer *buffer;

    class Highlight *m_highlight;

    KateArbitraryHighlight* m_arbitraryHL;

    KateAutoIndent *m_indenter;

    static int m_getSearchTextFrom;

    bool hlSetByUser;

    bool m_modOnHd;
    unsigned char m_modOnHdReason;

    QString m_docName;
    int m_docNameNumber;

    // defaults for all views !!!
    static bool m_collapseTopLevelOnLoad;

    static bool s_configLoaded;

    QPtrList<class KateSuperCursor> m_superCursors;

    // file type !!!
    int m_fileType;
    bool m_fileTypeSetByUser;

  public slots:
  void spellcheck();
  void ready();
  void misspelling( const QString&, const QStringList&, unsigned int );
  void corrected  ( const QString&, const QString&, unsigned int);
  void spellResult( const QString& );
  void spellCleanDone();


  void slotQueryClose_save(bool *handled, bool* abortClosing);

  private:
    void makeAttribs ();
  
    void locatePosition( uint pos, uint& line, uint& col );
    KSpell*         m_kspell;
    int             m_mispellCount;
    int             m_replaceCount;
    bool            m_reloading;

  public:
    bool checkOverwrite( KURL u );
    
    static void setDefaultEncoding (const QString &encoding);

  /**
   * Configuration
   */
  public:
    KateDocumentConfig *config () { return m_config; };

    void updateConfig ();

  private:
    KateDocumentConfig *m_config;

  /**
   * Variable Reader
   * TODO add register functionality/ktexteditor interface
   */
  private:
    /**
      Reads all the variables in the document.
      Called when opening/saving a document
    */
    void readVariables(bool onlyViewAndRenderer = false);

    /**
      Reads and applies the variables in a single line
      TODO registered variables gets saved in a [map]
    */
    void readVariableLine( QString t, bool onlyViewAndRenderer = false );
    /**
      Sets a view variable in all the views.
    */
    void setViewVariable( QString var, QString val );
    /**
      @return weather a string value could be converted
      to a bool value as supported.
      The value is put in *result.
    */
    bool checkBoolValue( QString value, bool *result );
    /**
      @return weather a string value could be converted
      to a integer value.
      The value is put in *result.
    */
    bool checkIntValue( QString value, int *result );
    /*
      Feeds value into @p col using QColor::setNamedColor() and returns
      wheather the color is valid
    */
    bool checkColorValue( QString value, QColor &col );

    static QRegExp kvLine;
    static QRegExp kvVar;

  k_dcop:
    uint documentNumber () const;

};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
