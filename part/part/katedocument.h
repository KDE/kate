/* This file is part of the KDE libraries
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
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

#ifndef _KATE_DOCUMENT_H_
#define _KATE_DOCUMENT_H_

#include "katesupercursor.h"
#include "katetextline.h"
#include "kateundo.h"
#include "katebuffer.h"
#include "katecodefoldinghelpers.h"

#include "../interfaces/document.h"

#include <ktexteditor/configinterfaceextension.h>
#include <ktexteditor/encodinginterface.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/editinterfaceext.h>
#include <ktexteditor/templateinterface.h>

#include <dcopobject.h>

#include <kmimetype.h>
#include <klocale.h>

#include <qintdict.h>
#include <qmap.h>
#include <qdatetime.h>

namespace KTextEditor { class Plugin; }

namespace KIO { class TransferJob; }

class KateUndoGroup;
class KateCmd;
class KateAttribute;
class KateAutoIndent;
class KateCodeFoldingTree;
class KateBuffer;
class KateView;
class KateViewInternal;
class KateArbitraryHighlight;
class KateSuperRange;
class KateLineInfo;
class KateBrowserExtension;
class KateDocumentConfig;
class KateHighlighting;
class KatePartPluginItem;
class KatePartPluginInfo;

class KSpell;
class KTempFile;

class QTimer;

class KateKeyInterceptorFunctor;

//
// Kate KTextEditor::Document class (and even KTextEditor::Editor ;)
//
class KateDocument : public Kate::Document,
                     public Kate::DocumentExt,
                     public KTextEditor::ConfigInterfaceExtension,
                     public KTextEditor::EncodingInterface,
                     public KTextEditor::SessionConfigInterface,
                     public KTextEditor::EditInterfaceExt,
                     public KTextEditor::TemplateInterface,
                     public DCOPObject
{
  K_DCOP
  Q_OBJECT

  friend class KateViewInternal;
  friend class KateRenderer;

  public:
    KateDocument (bool bSingleViewMode=false, bool bBrowserView=false, bool bReadOnly=false,
        QWidget *parentWidget = 0, const char *widgetName = 0, QObject * = 0, const char * = 0);
    ~KateDocument ();

    bool closeURL();

  //
  // Plugins section
  //
  public:
    void unloadAllPlugins ();

    void enableAllPluginsGUI (KateView *view);
    void disableAllPluginsGUI (KateView *view);

    void loadPlugin (uint pluginIndex);
    void unloadPlugin (uint pluginIndex);

    void enablePluginGUI (KTextEditor::Plugin *plugin, KateView *view);
    void enablePluginGUI (KTextEditor::Plugin *plugin);

    void disablePluginGUI (KTextEditor::Plugin *plugin, KateView *view);
    void disablePluginGUI (KTextEditor::Plugin *plugin);

  private:
     QMemArray<KTextEditor::Plugin *> m_plugins;

  public:
    bool readOnly () const { return m_bReadOnly; }
    bool browserView () const { return m_bBrowserView; }
    bool singleViewMode () const { return m_bSingleViewMode; }
    KateBrowserExtension *browserExtension () { return m_extension; }
    void textAsHtmlStream ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise, QTextStream *ts) const;

  private:
    // only to make part work, don't change it !
    bool m_bSingleViewMode;
    bool m_bBrowserView;
    bool m_bReadOnly;
    KateBrowserExtension *m_extension;

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

    /**
     * set the active view.
     *
     * If @p view is allready the active view, nothing is done.
     *
     * If the document is modified on disk, ask the user what to do.
     *
     * @since Kate 2.4
     */
    void setActiveView( KateView *view );

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

    QString textAsHtml ( uint startLine, uint startCol, uint endLine, uint endCol, bool blockwise) const;

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
    void charactersSemiInteractivelyInserted(int ,int ,const QString&);
    void backspacePressed();

  public:
//BEGIN editStart/editEnd (start, end, undo, cursor update, view update)
    /**
     * Enclose editor actions with @p editStart() and @p editEnd() to group
     * them.
     * @param withUndo if true, add undo history
     */
    void editStart (bool withUndo = true);
    /** @see Same as editStart() with undo */
    void editBegin () { editStart(); }
    /**
     * End a editor operation.
     * @see editStart()
     */
    void editEnd ();

//END editStart/editEnd

//BEGIN LINE BASED INSERT/REMOVE STUFF (editStart() and editEnd() included)
    /**
     * Add a string in the given line/column
     * @param line line number
     * @param col column
     * @param s string to be inserted
     * @return true on success
     */
    bool editInsertText ( uint line, uint col, const QString &s );
    /**
     * Remove a string in the given line/column
     * @param line line number
     * @param col column
     * @param len length of text to be removed
     * @return true on success
     */
    bool editRemoveText ( uint line, uint col, uint len );

    /**
     * Mark @p line as @p autowrapped. This is necessary if static word warp is
     * enabled, because we have to know whether to insert a new line or add the
     * wrapped words to the followin line.
     * @param line line number
     * @param autowrapped autowrapped?
     * @return true on success
     */
    bool editMarkLineAutoWrapped ( uint line, bool autowrapped );

    /**
     * Wrap @p line. If @p newLine is true, ignore the textline's flag
     * KateTextLine::flagAutoWrapped and force a new line. Whether a new line
     * was needed/added you can grab with @p newLineAdded.
     * @param line line number
     * @param col column
     * @param newLine if true, force a new line
     * @param newLineAdded return value is true, if new line was added (may be 0)
     * @return true on success
     */
    bool editWrapLine ( uint line, uint col, bool newLine = true, bool *newLineAdded = 0 );
    /**
     * Unwrap @p line. If @p removeLine is true, we force to join the lines. If
     * @p removeLine is true, @p length is ignored (eg not needed).
     * @param line line number
     * @param removeLine if true, force to remove the next line
     * @return true on success
     */
    bool editUnWrapLine ( uint line, bool removeLine = true, uint length = 0 );

    /**
     * Insert a string at the given line.
     * @param line line number
     * @param s string to insert
     * @return true on success
     */
    bool editInsertLine ( uint line, const QString &s );
    /**
     * Remove a line
     * @param line line number
     * @return true on success
     */
    bool editRemoveLine ( uint line );

    /**
     * Remove a line
     * @param startLine line to begin wrapping
     * @param endLine line to stop wrapping
     * @return true on success
     */
    bool wrapText (uint startLine, uint endLine);
//END LINE BASED INSERT/REMOVE STUFF

  signals:
    /**
     * Emitted each time text is inserted into a pre-existing line, including appends.
     * Does not include newly inserted lines at the moment. ### needed?
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
    void undoSafePoint();

  private slots:
    void undoCancel();

  private:
    void editAddUndo (KateUndoGroup::UndoType type, uint line, uint col, uint len, const QString &text);
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
    bool m_undoComplexMerge;
    KateUndoGroup* m_editCurrentUndo;

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
    friend class KateTemplateHandler;

  private:
    QPtrList<KateSuperCursor> m_superCursors;

    //
    // some internals for undo/redo
    //
    QPtrList<KateUndoGroup> undoItems;
    QPtrList<KateUndoGroup> redoItems;
    bool m_undoDontMerge; //create a setter later on and remove the friend declaration
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
    void textInserted(int line,int column);

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

  public:
    void bufferHlChanged ();

  private:
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
  // KTextEditor::DocumentInfoInterface ( ### unfinished )
  //
  public:
    /**
     * @return the name of the mimetype for the document.
     *
     * This method is using @see KMimeType::findByURL, and if the pointer
     * is then still the default MimeType for a nonlocal or unsaved file,
     * uses mimeTypeForContent().
     *
     * @since Kate 2.3
     */
    QString mimeType();

    /**
     * @return the calculated size in bytes that the document would have when saved to
     * disk.
     *
     * @since Kate 2.3
     * @todo implement this (it returns 0 right now)
     */
    long fileSize();

    /**
     * @return the calculated size the document would have when saved to disk
     * as a human readable string.
     *
     * @since Kate 2.3
     * @todo implement this (it returns "UNKNOWN")
     */
    QString niceFileSize();

    /**
     * @return a pointer to the KMimeType for this document, found by analyzing the
     * actual content.
     *
     * Note that this method is *not* part of the DocumentInfoInterface.
     *
     * @since Kate 2.3
     */
    KMimeType::Ptr mimeTypeForContent();

  //
  // KTextEditor::VariableInterface
  //
  public:
    QString variable( const QString &name ) const;

  signals:
    void variableChanged( const QString &, const QString & );

  private:
    QMap<QString, QString> m_storedVariables;

  //
  // KParts::ReadWrite stuff
  //
  public:
    bool openURL( const KURL &url );

    /* Anders:
      I reimplemented this, since i need to check if backup succeeded
      if requested */
    bool save();

    /* Anders: Reimplemented to do kate specific stuff */
    bool saveAs( const KURL &url );

    bool openFile (KIO::Job * job);
    bool openFile ();

    bool saveFile ();

    void setReadWrite ( bool rw = true );

    void setModified( bool m );

  private slots:
    void slotDataKate ( KIO::Job* kio_job, const QByteArray &data );
    void slotFinishedKate ( KIO::Job * job );

  private:
    void abortLoadKate();

    void activateDirWatch ();
    void deactivateDirWatch ();

    QString m_dirWatchFile;

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
    inline uint lastLine() const { return numLines()-1; }

    uint configFlags ();
    void setConfigFlags (uint flags);

    // Repaint all of all of the views
    void repaintViews(bool paintOnlyDirty = true);

    inline KateHighlighting *highlight () { return m_buffer->highlight(); }

    inline KateHighlighting *highlight () const { return m_buffer->highlight(); }

  public slots:    //please keep prototypes and implementations in same order
    void tagLines(int start, int end);
    void tagLines(KateTextCursor start, KateTextCursor end);

  //export feature
  public slots:
     void exportAs(const QString&);

  private: //the following things should become plugins
    bool exportDocumentToHTML (QTextStream *outputStream,const QString &name);
    QString HTMLEncode (QChar theChar);

  signals:
    void modifiedChanged ();
    void preHighlightChanged(uint);

  private slots:
    void internalHlChanged();

  public:
    void addView(KTextEditor::View *);
    void removeView(KTextEditor::View *);

    void addSuperCursor(class KateSuperCursor *, bool privateC);
    void removeSuperCursor(class KateSuperCursor *, bool privateC);

    bool ownedView(KateView *);
    bool isLastView(int numViews);

    uint currentColumn( const KateTextCursor& );
    void newLine(             KateTextCursor&, KateViewInternal * ); // Changes input
    void backspace(     KateView *view, const KateTextCursor& );
    void del(           KateView *view, const KateTextCursor& );
    void transpose(     const KateTextCursor& );

    void paste ( KateView* view );

  public:
    void insertIndentChars ( KateView *view );

    void indent ( KateView *view, uint line, int change );
    void comment ( KateView *view, uint line, uint column, int change );
    void align ( KateView *view, uint line );

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

    /**
      Find the position (line and col) of the next char
      that is not a space. If found line and col point to the found character.
      Otherwise they have both the value -1.
      @param line Line of the character which is examined first.
      @param col Column of the character which is examined first.
      @return True if the specified or a following character is not a space
               Otherwise false.
    */
    bool nextNonSpaceCharPos(int &line, int &col);

    /**
      Find the position (line and col) of the previous char
      that is not a space. If found line and col point to the found character.
      Otherwise they have both the value -1.
      @return True if the specified or a preceding character is not a space.
               Otherwise false.
    */
    bool previousNonSpaceCharPos(int &line, int &col);

    /**
    * Sets a comment marker as defined by the language providing the attribute
    * @p attrib on the line @p line
    */
    void addStartLineCommentToSingleLine(int line, int attrib=0);
    /**
    * Removes a comment marker as defined by the language providing the attribute
    * @p attrib on the line @p line
    */
    bool removeStartLineCommentFromSingleLine(int line, int attrib=0);

    /**
    * @see addStartLineCommentToSingleLine.
    */
    void addStartStopCommentToSingleLine(int line, int attrib=0);
    /**
    *@see removeStartLineCommentFromSingleLine.
    */
    bool removeStartStopCommentFromSingleLine(int line, int attrib=0);
    /**
    *@see removeStartLineCommentFromSingleLine.
    */
    bool removeStartStopCommentFromRegion(const KateTextCursor &start, const KateTextCursor &end, int attrib=0);

    /**
     * Add a comment marker as defined by the language providing the attribute
     * @p attrib to each line in the selection.
     */
           void addStartStopCommentToSelection( KateView *view, int attrib=0 );
    /**
    * @see addStartStopCommentToSelection.
    */
          void addStartLineCommentToSelection( KateView *view, int attrib=0 );

    /**
    * Removes comment markers relevant to the language providing
    * the attribuge @p attrib from each line in the selection.
    *
    * @return whether the operation succeded.
    */
    bool removeStartStopCommentFromSelection( KateView *view, int attrib=0 );
    /**
    * @see removeStartStopCommentFromSelection.
    */
    bool removeStartLineCommentFromSelection( KateView *view, int attrib=0 );

  public:
    QString getWord( const KateTextCursor& cursor );

  public:
    void tagAll();
    void updateViews();

    void newBracketMark( const KateTextCursor& start, KateTextRange& bm, int maxLines = -1 );
    bool findMatchingBracket( KateTextCursor& start, KateTextCursor& end, int maxLines = -1 );

  private:
    void guiActivateEvent( KParts::GUIActivateEvent *ev );

  public:

    QString docName () {return m_docName;};

    void setDocName (QString docName);

    void lineInfo (KateLineInfo *info, unsigned int line);

    KateCodeFoldingTree *foldingTree ();

  public:
    /**
     * @return wheather the document is modified on disc since last saved.
     *
     * @since 3.3
     */
    bool isModifiedOnDisc() { return m_modOnHd; };

    /** @deprecated */
    void isModOnHD( bool =false ) {};

    void setModifiedOnDisk( int reason );

  public slots:
    /**
     * Ask the user what to do, if the file has been modified on disc.
     * Reimplemented from Kate::Document.
     *
     * @since 3.3
     */
    void slotModifiedOnDisk( Kate::View *v=0 );

    /**
     * Reloads the current document from disc if possible
     */
    void reloadFile();

  private:
    int m_isasking; // don't reenter slotModifiedOnDisk when this is true
                    // -1: ignore once, 0: false, 1: true

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

  public slots:
     void applyWordWrap ();

   // code folding
  public:
    inline uint getRealLine(unsigned int virtualLine)
    {
      return m_buffer->lineNumber (virtualLine);
    }

    inline uint getVirtualLine(unsigned int realLine)
    {
      return m_buffer->lineVisibleNumber (realLine);
    }

    inline uint visibleLines ()
    {
      return m_buffer->countVisible ();
    }

    inline KateTextLine::Ptr kateTextLine(uint i)
    {
      return m_buffer->line (i);
    }

    inline KateTextLine::Ptr plainKateTextLine(uint i)
    {
      return m_buffer->plainLine (i);
    }

  signals:
    void codeFoldingUpdated();
    void aboutToRemoveText(const KateTextRange&);
    void textRemoved();
  public slots:
    void dumpRegionTree();

  private slots:
    void slotModOnHdDirty (const QString &path);
    void slotModOnHdCreated (const QString &path);
    void slotModOnHdDeleted (const QString &path);

  private:
    /**
     * create a MD5 digest of the file, if it is a local file,
     * and fill it into the string @p result.
     * This is using KMD5::hexDigest().
     *
     * @return wheather the operation was attempted and succeded.
     *
     * @since 3.3
     */
    bool createDigest ( QCString &result );

    /**
     * create a string for the modonhd warnings, giving the reason.
     *
     * @since 3.3
     */
    QString reasonedMOHString() const;

    /**
     * Removes all trailing whitespace form @p line, if
     * the cfRemoveTrailingDyn confg flag is set,
     * and the active view cursor is not on line and behind
     * the last nonspace character.
     *
     * @since 3.3
     */
    void removeTrailingSpace( uint line );

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
    KateBuffer *m_buffer;

    KateArbitraryHighlight* m_arbitraryHL;

    KateAutoIndent *m_indenter;

    bool hlSetByUser;

    bool m_modOnHd;
    unsigned char m_modOnHdReason;
    QCString m_digest; // MD5 digest, updated on load/save

    QString m_docName;
    int m_docNameNumber;

    // file type !!!
    int m_fileType;
    bool m_fileTypeSetByUser;

    /**
     * document is still reloading a file
     */
    bool m_reloading;

  public slots:
    void spellcheck();
    /**
     * Spellcheck a defined portion of the text.
     *
     * @param from Where to start the check
     * @param to Where to end. If this is (0,0), it will be set to the end of the document.
     */
    void spellcheck( const KateTextCursor &from, const KateTextCursor &to=KateTextCursor() );
    void ready(KSpell *);
    void misspelling( const QString&, const QStringList&, unsigned int );
    void corrected  ( const QString&, const QString&, unsigned int);
    void spellResult( const QString& );
    void spellCleanDone();


    void slotQueryClose_save(bool *handled, bool* abortClosing);

  private:
    void locatePosition( uint pos, uint& line, uint& col );
    KSpell *m_kspell;
    // define the part of the text to check
    KateTextCursor m_spellStart, m_spellEnd;
    // keep track of where we are.
    KateTextCursor m_spellPosCursor;
    uint m_spellLastPos;

  public:
    void makeAttribs (bool needInvalidate = true);

    static bool checkOverwrite( KURL u );

    static void setDefaultEncoding (const QString &encoding);

  /**
   * Configuration
   */
  public:
    inline KateDocumentConfig *config () { return m_config; };

    void updateConfig ();

  private:
    KateDocumentConfig *m_config;

  /**
   * Variable Reader
   * TODO add register functionality/ktexteditor interface
   */
  private:
    /**
     * read dir config file
     */
    void readDirConfig ();

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
    static bool checkBoolValue( QString value, bool *result );
    /**
      @return weather a string value could be converted
      to a integer value.
      The value is put in *result.
    */
    static bool checkIntValue( QString value, int *result );
    /**
      Feeds value into @p col using QColor::setNamedColor() and returns
      wheather the color is valid
    */
    static bool checkColorValue( QString value, QColor &col );

    static QRegExp kvLine;
    static QRegExp kvVar;

    KIO::TransferJob *m_job;
    KTempFile *m_tempFile;

  // TemplateInterface
  public:
      bool setTabInterceptor(KateKeyInterceptorFunctor *interceptor); /* perhaps make it moregeneral like an eventfilter*/
      bool removeTabInterceptor(KateKeyInterceptorFunctor *interceptor);
      bool invokeTabInterceptor(KKey);

  protected:
      virtual bool insertTemplateTextImplementation ( uint line, uint column, const QString &templateString, const QMap<QString,QString> &initialValues, QWidget *parentWindow=0 );
      KateKeyInterceptorFunctor *m_tabInterceptor;
  protected slots:
      void testTemplateCode();
  // IM input stuff
  //
  public:
    void setIMSelectionValue( uint imStartLine, uint imStart, uint imEnd,
                              uint imSelStart, uint imSelEnd, bool m_imComposeEvent );
    void getIMSelectionValue( uint *imStartLine, uint *imStart, uint *imEnd,
                              uint *imSelStart, uint *imSelEnd );
    bool isIMSelection( int _line, int _column );
    bool isIMEdit( int _line, int _column );

  private:
    uint m_imStartLine;
    uint m_imStart;
    uint m_imEnd;
    uint m_imSelStart;
    uint m_imSelEnd;
    bool m_imComposeEvent;

  //BEGIN DEPRECATED
  //
  // KTextEditor::SelectionInterface stuff
  // DEPRECATED, this will be removed for KDE 4.x !!!!!!!!!!!!!!!!!!!!
  //
  public slots:
    bool setSelection ( uint startLine, uint startCol, uint endLine, uint endCol );
    bool clearSelection ();
    bool hasSelection () const;
    QString selection () const;
    bool removeSelectedText ();
    bool selectAll();

    //
    // KTextEditor::SelectionInterfaceExt
    //
    int selStartLine();
    int selStartCol();
    int selEndLine();
    int selEndCol();


  // hack, only there to still support the deprecated stuff, will be removed for KDE 4.x
  #undef signals
  #define signals public
  signals:
  #undef signals
  #define signals protected
    void selectionChanged ();

  //
  // KTextEditor::BlockSelectionInterface stuff
  // DEPRECATED, this will be removed for KDE 4.x !!!!!!!!!!!!!!!!!!!!
  //
  public slots:
    bool blockSelectionMode ();
    bool setBlockSelectionMode (bool on);
    bool toggleBlockSelectionMode ();

  private:
//END DEPRECATED

  k_dcop:
    uint documentNumber () const;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

