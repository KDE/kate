/* This file is part of the KDE libraries
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_DOCUMENT_H_
#define _KATE_DOCUMENT_H_

#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <QtCore/QDate>
#include <QtGui/QClipboard>
#include <QtCore/QStack>

#include <kmimetype.h>

#include <ktexteditor/document.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/variableinterface.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/annotationinterface.h>
#include <ktexteditor/highlightinterface.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/recoveryinterface.h>

#include "katepartprivate_export.h"
#include "katetextline.h"
#include "katetextcursor.h"
#include "katetextrange.h"
#include "katecodefolding.h"
#include "messageinterface.h"

namespace KTextEditor {
  class Plugin;
  class Attribute;
  class TemplateScript;
}

namespace KIO { class TransferJob; }

namespace Kate { class SwapFile; }

class KateCodeFoldingTree;
class KateBuffer;
class KateView;
class KateLineInfo;
class KateDocumentConfig;
class KateHighlighting;
class KateUndoManager;
class KateOnTheFlyChecker;

class KateAutoIndent;


//
// Kate KTextEditor::Document class (and even KTextEditor::Editor ;)
//
class KATEPART_TESTS_EXPORT KateDocument : public KTextEditor::Document,
                     public KTextEditor::SessionConfigInterface,
                     public KTextEditor::ParameterizedSessionConfigInterface,
                     public KTextEditor::SearchInterface,
                     public KTextEditor::MarkInterface,
                     public KTextEditor::VariableInterface,
                     public KTextEditor::ModificationInterface,
                     public KTextEditor::ConfigInterface,
                     public KTextEditor::AnnotationInterface,
                     public KTextEditor::HighlightInterface,
                     public KTextEditor::MovingInterface,
                     public KTextEditor::RecoveryInterface,
                     public KTextEditor::MessageInterface,
                     private KTextEditor::MovingRangeFeedback
{
  Q_OBJECT
  Q_INTERFACES(KTextEditor::SessionConfigInterface)
  Q_INTERFACES(KTextEditor::ParameterizedSessionConfigInterface)
  Q_INTERFACES(KTextEditor::SearchInterface)
  Q_INTERFACES(KTextEditor::MarkInterface)
  Q_INTERFACES(KTextEditor::VariableInterface)
  Q_INTERFACES(KTextEditor::ModificationInterface)
  Q_INTERFACES(KTextEditor::AnnotationInterface)
  Q_INTERFACES(KTextEditor::ConfigInterface)
  Q_INTERFACES(KTextEditor::HighlightInterface)
  Q_INTERFACES(KTextEditor::MovingInterface)
  Q_INTERFACES(KTextEditor::RecoveryInterface)
  Q_INTERFACES(KTextEditor::MessageInterface)

  friend class KateDocumentTest;

  public:
    explicit KateDocument (bool bSingleViewMode=false, bool bBrowserView=false, bool bReadOnly=false,
                  QWidget *parentWidget = 0, QObject * = 0);
    ~KateDocument ();
    
    using ReadWritePart::closeUrl;
    virtual bool closeUrl();

    virtual KTextEditor::Editor *editor ();

    KTextEditor::Range rangeOnLine(KTextEditor::Range range, int line) const;

    
  /*
   * Overload this to have on-demand view creation
   */
  public:
    /**
     * @return The widget defined by this part, set by setWidget().
     */
    virtual QWidget *widget();

Q_SIGNALS:
// TODO for KDE5: move to KTE::Document
  void readWriteChanged (KTextEditor::Document *document);

    
    
  public:
    bool readOnly () const { return m_bReadOnly; }
    bool browserView () const { return m_bBrowserView; }
    bool singleViewMode () const { return m_bSingleViewMode; }
    static bool simpleMode ();

  private:
    // only to make part work, don't change it !
    const bool m_bSingleViewMode;
    const bool m_bBrowserView;
    const bool m_bReadOnly;

  //
  // KTextEditor::Document stuff
  //
  public:
    virtual KTextEditor::View *createView( QWidget *parent );
    virtual const QList<KTextEditor::View*> &views () const;

    virtual KTextEditor::View* activeView() const { return m_activeView; }
    // Invalid covariant returns my a$$... for some reason gcc won't let me return a KateView above!
    KateView* activeKateView() const;

  private:
    QLinkedList<KateView*> m_views;
    QList<KTextEditor::View*> m_textEditViews;
    KTextEditor::View *m_activeView;

  //
  // KTextEditor::EditInterface stuff
  //
  public Q_SLOTS:
    virtual bool setText(const QString &);
    virtual bool setText(const QStringList& text);
    virtual bool clear ();

    virtual bool insertText ( const KTextEditor::Cursor &position, const QString &s, bool block = false );
    virtual bool insertText ( const KTextEditor::Cursor &position, const QStringList &text, bool block = false );

    virtual bool insertLine ( int line, const QString &s );
    virtual bool insertLines ( int line, const QStringList &s );

    virtual bool removeText ( const KTextEditor::Range &range, bool block = false );
    virtual bool removeLine ( int line );

    virtual bool replaceText ( const KTextEditor::Range &range, const QString &s, bool block = false );

    // unhide method...
    virtual bool replaceText (const KTextEditor::Range &r, const QStringList &l, bool b)
    { return KTextEditor::Document::replaceText (r, l, b); }

  public:
    virtual QString text ( const KTextEditor::Range &range, bool blockwise = false ) const;
    virtual QStringList textLines ( const KTextEditor::Range& range, bool block = false ) const;
    virtual QString text() const;
    virtual QString line(int line) const;
    virtual QChar character(const KTextEditor::Cursor& position) const;
    virtual int lines() const;
    virtual KTextEditor::Cursor documentEnd() const;
    int numVisLines() const;
    virtual int totalCharacters() const;
    virtual int lineLength(int line) const;

  Q_SIGNALS:
    void charactersSemiInteractivelyInserted(const KTextEditor::Cursor& position, const QString& text);

  public:
//BEGIN editStart/editEnd (start, end, undo, cursor update, view update)
    /**
     * Enclose editor actions with @p editStart() and @p editEnd() to group
     * them.
     */
    void editStart ();

    /**
     * Alias for @p editStart()
     */
    void editBegin () { editStart(); }

    /**
     * End a editor operation.
     * @see editStart()
     */
    void editEnd ();

    void pushEditState();
    void popEditState();

    virtual bool startEditing () { editStart (); return true; }
    virtual bool endEditing () { editEnd (); return true; }

//END editStart/editEnd

    void inputMethodStart();
    void inputMethodEnd();

//BEGIN LINE BASED INSERT/REMOVE STUFF (editStart() and editEnd() included)
    /**
     * Add a string in the given line/column
     * @param line line number
     * @param col column
     * @param s string to be inserted
     * @return true on success
     */
    bool editInsertText ( int line, int col, const QString &s );
    /**
     * Remove a string in the given line/column
     * @param line line number
     * @param col column
     * @param len length of text to be removed
     * @return true on success
     */
    bool editRemoveText ( int line, int col, int len );

    /**
     * Mark @p line as @p autowrapped. This is necessary if static word warp is
     * enabled, because we have to know whether to insert a new line or add the
     * wrapped words to the followin line.
     * @param line line number
     * @param autowrapped autowrapped?
     * @return true on success
     */
    bool editMarkLineAutoWrapped ( int line, bool autowrapped );

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
    bool editWrapLine ( int line, int col, bool newLine = true, bool *newLineAdded = 0 );
    /**
     * Unwrap @p line. If @p removeLine is true, we force to join the lines. If
     * @p removeLine is true, @p length is ignored (eg not needed).
     * @param line line number
     * @param removeLine if true, force to remove the next line
     * @return true on success
     */
    bool editUnWrapLine ( int line, bool removeLine = true, int length = 0 );

    /**
     * Insert a string at the given line.
     * @param line line number
     * @param s string to insert
     * @return true on success
     */
    bool editInsertLine ( int line, const QString &s );
    /**
     * Remove a line
     * @param line line number
     * @return true on success
     */
    bool editRemoveLine ( int line );

    bool editRemoveLines ( int from, int to );

    /**
     * Remove a line
     * @param startLine line to begin wrapping
     * @param endLine line to stop wrapping
     * @return true on success
     */
    bool wrapText (int startLine, int endLine);
//END LINE BASED INSERT/REMOVE STUFF

  Q_SIGNALS:
    /**
     * Emmitted when text from @p line was wrapped at position pos onto line @p nextLine.
     */
    void editLineWrapped ( int line, int col, int len );

    /**
     * Emitted each time text from @p nextLine was upwrapped onto @p line.
     */
    void editLineUnWrapped ( int line, int col );

  public:
    bool isEditRunning() const;

    void setUndoMergeAllEdits(bool merge);

  private:
    int editSessionNumber;
    QStack<int> editStateStack;
    bool editIsRunning;
    bool m_undoMergeAllEdits;

  //
  // KTextEditor::UndoInterface stuff
  //
  public Q_SLOTS:
    void undo ();
    void redo ();

  public:
    uint undoCount () const;
    uint redoCount () const;

    KateUndoManager* undoManager()
    {
        return m_undoManager;
    }

  protected:
    KateUndoManager* const m_undoManager;

  Q_SIGNALS:
    void undoChanged ();

  //
  // KTextEditor::SearchInterface stuff
  //
  public:
    virtual QVector<KTextEditor::Range> searchText(
        const KTextEditor::Range & range,
        const QString & pattern,
        const KTextEditor::Search::SearchOptions options);

    virtual KTextEditor::Search::SearchOptions supportedSearchOptions() const;

  private:
    /**
     * Return a widget suitable to be used as a dialog parent.
     */
    QWidget * dialogParent();

  /*
   * Access to the mode/highlighting subsystem
   */
  public:
    /**
     * Return the name of the currently used mode
     * \return name of the used mode
     *
     */
    virtual QString mode() const;

    /**
     * Return the name of the currently used mode
     * \return name of the used mode
     *
     */
    virtual QString highlightingMode() const;

    /**
     * Return a list of the names of all possible modes
     * \return list of mode names
     */
    virtual QStringList modes() const;

    /**
     * Return a list of the names of all possible modes
     * \return list of mode names
     */
    virtual QStringList highlightingModes() const;

    /**
     * Set the current mode of the document by giving its name
     * \param name name of the mode to use for this document
     * \return \e true on success, otherwise \e false
     */
    virtual bool setMode(const QString &name);

    /**
     * Set the current mode of the document by giving its name
     * \param name name of the mode to use for this document
     * \return \e true on success, otherwise \e false
     */
    virtual bool setHighlightingMode(const QString &name);
    /**
     * Returns the name of the section for a highlight given its index in the highlight
     * list (as returned by highlightModes()).
     * You can use this function to build a tree of the highlight names, organized in sections.
     * \param name the name of the highlight for which to find the section name.
     */
    virtual QString highlightingModeSection( int index ) const;

    /**
     * Returns the name of the section for a mode given its index in the highlight
     * list (as returned by modes()).
     * You can use this function to build a tree of the mode names, organized in sections.
     * \param name the name of the highlight for which to find the section name.
     */
    virtual QString modeSection( int index ) const;

  /*
   * Helpers....
   */
  public:
    void bufferHlChanged();

    /**
     * allow to mark, that we changed hl on user wish and should not reset it
     * atm used for the user visible menu to select highlightings
     */
    void setDontChangeHlOnSave();

    /**
     * Set that the BOM marker is forced via the tool menu
     */
    void bomSetByUser();

  //
  // KTextEditor::SessionConfigInterface and KTextEditor::ParameterizedSessionConfigInterface stuff
  //
  public:
    virtual void readSessionConfig (const KConfigGroup&);
    virtual void writeSessionConfig (KConfigGroup&);
    virtual void readParameterizedSessionConfig (const KConfigGroup&, unsigned long configParameters);
    virtual void writeParameterizedSessionConfig (KConfigGroup&, unsigned long configParameters);

  Q_SIGNALS:
    void configChanged();

  //
  // KTextEditor::MarkInterface
  //
  public Q_SLOTS:
    virtual void setMark( int line, uint markType );
    virtual void clearMark( int line );

    virtual void addMark( int line, uint markType );
    virtual void removeMark( int line, uint markType );

    virtual void clearMarks();

    void requestMarkTooltip( int line, QPoint position );

    ///Returns true if the click on the mark should not be further processed
    bool handleMarkClick( int line );

    ///Returns true if the context-menu event should not further be processed
    bool handleMarkContextMenu( int line, QPoint position );

    virtual void setMarkPixmap( MarkInterface::MarkTypes, const QPixmap& );

    virtual void setMarkDescription( MarkInterface::MarkTypes, const QString& );

    virtual void setEditableMarks( uint markMask );

  public:
    virtual uint mark( int line );
    virtual const QHash<int, KTextEditor::Mark*> &marks ();
    virtual QPixmap markPixmap( MarkInterface::MarkTypes ) const;
    virtual QString markDescription( MarkInterface::MarkTypes ) const;
    virtual QColor markColor( MarkInterface::MarkTypes ) const;
    virtual uint editableMarks() const;

  Q_SIGNALS:
    void markToolTipRequested( KTextEditor::Document* document, KTextEditor::Mark mark, QPoint position, bool& handled );

    void markContextMenuRequested( KTextEditor::Document* document, KTextEditor::Mark mark, QPoint pos, bool& handled );

    void markClicked( KTextEditor::Document* document, KTextEditor::Mark mark, bool& handled );

    void marksChanged( KTextEditor::Document* );
    void markChanged( KTextEditor::Document*, KTextEditor::Mark, KTextEditor::MarkInterface::MarkChangeAction );

  private:
    QHash<int, KTextEditor::Mark*> m_marks;
    QHash<int,QPixmap>           m_markPixmaps;
    QHash<int,QString>           m_markDescriptions;
    uint                        m_editableMarks;

  //
  // KTextEditor::PrintInterface
  //
  public Q_SLOTS:
    bool printDialog ();
    Q_SCRIPTABLE bool print ();

  //
  // KTextEditor::DocumentInfoInterface ( ### unfinished )
  //
  public:
    /**
     * @return the name of the mimetype for the document.
     *
     * This method is using KMimeType::findByUrl, and if the pointer
     * is then still the default MimeType for a nonlocal or unsaved file,
     * uses mimeTypeForContent().
     */
    virtual QString mimeType();

    /**
     * @return a pointer to the KMimeType for this document, found by analyzing the
     * actual content.
     *
     * Note that this method is *not* part of the DocumentInfoInterface.
     */
    KMimeType::Ptr mimeTypeForContent();

  //
  // KTextEditor::VariableInterface
  //
  public:
    virtual QString variable( const QString &name ) const;
    // ### TODO KDE5: add to KTextEditor::VaribaleInterface
    virtual QString setVariable( const QString &name, const QString &value);

  Q_SIGNALS:
    void variableChanged( KTextEditor::Document*, const QString &, const QString & );

  private:
    QMap<QString, QString> m_storedVariables;

  //
  // MovingInterface API
  //
  public:
    /**
     * Create a new moving cursor for this document.
     * @param position position of the moving cursor to create
     * @param insertBehavior insertion behavior
     * @return new moving cursor for the document
     */
    virtual KTextEditor::MovingCursor *newMovingCursor (const KTextEditor::Cursor &position, KTextEditor::MovingCursor::InsertBehavior insertBehavior = KTextEditor::MovingCursor::MoveOnInsert);

    /**
     * Create a new moving range for this document.
     * @param range range of the moving range to create
     * @param insertBehaviors insertion behaviors
     * @param emptyBehavior behavior on becoming empty
     * @return new moving range for the document
     */
    virtual KTextEditor::MovingRange *newMovingRange (const KTextEditor::Range &range, KTextEditor::MovingRange::InsertBehaviors insertBehaviors = KTextEditor::MovingRange::DoNotExpand
      , KTextEditor::MovingRange::EmptyBehavior emptyBehavior = KTextEditor::MovingRange::AllowEmpty);

    /**
     * Current revision
     * @return current revision
     */
    virtual qint64 revision () const;

    /**
     * Last revision the buffer got successful saved
     * @return last revision buffer got saved, -1 if none
     */
    virtual qint64 lastSavedRevision () const;

    /**
     * Lock a revision, this will keep it around until released again.
     * But all revisions will always be cleared on buffer clear() (and therefor load())
     * @param revision revision to lock
     */
    virtual void lockRevision (qint64 revision);

    /**
     * Release a revision.
     * @param revision revision to release
     */
    virtual void unlockRevision (qint64 revision);

    /**
     * Transform a cursor from one revision to an other.
     * @param cursor cursor to transform
     * @param insertBehavior behavior of this cursor on insert of text at its position
     * @param fromRevision from this revision we want to transform
     * @param toRevision to this revision we want to transform, default of -1 is current revision
     */
    virtual void transformCursor (KTextEditor::Cursor &cursor, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision = -1);

    /**
     * Transform a cursor from one revision to an other.
     * @param line line number of the cursor to transform
     * @param column column number of the cursor to transform
     * @param insertBehavior behavior of this cursor on insert of text at its position
     * @param fromRevision from this revision we want to transform
     * @param toRevision to this revision we want to transform, default of -1 is current revision
     */
    virtual void transformCursor (int& line, int& column, KTextEditor::MovingCursor::InsertBehavior insertBehavior, qint64 fromRevision, qint64 toRevision = -1);

    /**
     * Transform a range from one revision to an other.
     * @param range range to transform
     * @param insertBehaviors behavior of this range on insert of text at its position
     * @param emptyBehavior behavior on becoming empty
     * @param fromRevision from this revision we want to transform
     * @param toRevision to this revision we want to transform, default of -1 is current revision
     */
    virtual void transformRange (KTextEditor::Range &range, KTextEditor::MovingRange::InsertBehaviors insertBehaviors, KTextEditor::MovingRange::EmptyBehavior emptyBehavior, qint64 fromRevision, qint64 toRevision = -1);

  //
  // MovingInterface Signals
  //
  Q_SIGNALS:
    /**
     * This signal is emitted before the cursors/ranges/revisions of a document are destroyed as the document is deleted.
     * @param document the document which the interface belongs too which is in the process of being deleted
     */
    void aboutToDeleteMovingInterfaceContent (KTextEditor::Document *document);

    /**
     * This signal is emitted before the ranges of a document are invalidated and the revisions are deleted as the document is cleared (for example on load/reload).
     * While this signal is emitted, still the old document content is around before the clear.
     * @param document the document which the interface belongs too which will invalidate its data
     */
    void aboutToInvalidateMovingInterfaceContent (KTextEditor::Document *document);

  //
  // Annotation Interface
  //
  public:

    virtual void setAnnotationModel( KTextEditor::AnnotationModel* model );
    virtual KTextEditor::AnnotationModel* annotationModel() const;

  Q_SIGNALS:
    void annotationModelChanged( KTextEditor::AnnotationModel*, KTextEditor::AnnotationModel* );

  private:
    KTextEditor::AnnotationModel* m_annotationModel;

  //
  // KParts::ReadWrite stuff
  //
  public:
    /**
     * open the file obtained by the kparts framework
     * the framework abstracts the loading of remote files
     * @return success
     */
    virtual bool openFile ();

    /**
     * save the file obtained by the kparts framework
     * the framework abstracts the uploading of remote files
     * @return success
     */
    virtual bool saveFile ();

    virtual void setReadWrite ( bool rw = true );

    virtual void setModified( bool m );

  private:
    void activateDirWatch (const QString &useFileName = QString());
    void deactivateDirWatch ();

    QString m_dirWatchFile;

  public:
    /**
     * Type chars in a view.
     * Will filter out non-printable chars from the realChars array before inserting.
     */
    bool typeChars ( KateView *type, const QString &realChars );

    /**
     * gets the last line number (lines() - 1)
     */
    inline int lastLine() const { return lines()-1; }

    // Repaint all of all of the views
    void repaintViews(bool paintOnlyDirty = true);

    KateHighlighting *highlight () const;

  public Q_SLOTS:    //please keep prototypes and implementations in same order
    void tagLines(int start, int end);
    void tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end);

  //export feature, obsolute
  public Q_SLOTS:
     void exportAs(const QString&) { }

  Q_SIGNALS:
    void preHighlightChanged(uint);

  private Q_SLOTS:
    void internalHlChanged();

  public:
    void addView(KTextEditor::View *);
    /** removes the view from the list of views. The view is *not* deleted.
     * That's your job. Or, easier, just delete the view in the first place.
     * It will remove itself. TODO: this could be converted to a private slot
     * connected to the view's destroyed() signal. It is not currently called
     * anywhere except from the KateView destructor.
     */
    void removeView(KTextEditor::View *);
    void setActiveView(KTextEditor::View*);

    bool ownedView(KateView *);

    uint toVirtualColumn( const KTextEditor::Cursor& );
    void newLine( KateView*view ); // Changes input
    void backspace(     KateView *view, const KTextEditor::Cursor& );
    void del(           KateView *view, const KTextEditor::Cursor& );
    void transpose(     const KTextEditor::Cursor& );

    void paste ( KateView* view, const QString &s );

  public:
    void indent ( KTextEditor::Range range, int change );
    void comment ( KateView *view, uint line, uint column, int change );
    void align ( KateView *view, const KTextEditor::Range &range );
    void insertTab( KateView *view, const KTextEditor::Cursor& );

    enum TextTransform { Uppercase, Lowercase, Capitalize };

    /**
      Handling uppercase, lowercase and capitalize for the view.

      If there is a selection, that is transformed, otherwise for uppercase or
      lowercase the character right of the cursor is transformed, for capitalize
      the word under the cursor is transformed.
    */
    void transform ( KateView *view, const KTextEditor::Cursor &, TextTransform );
    /**
      Unwrap a range of lines.
    */
    void joinLines( uint first, uint last );

  private:
    bool removeStringFromBeginning(int line, const QString &str);
    bool removeStringFromEnd(int line, const QString &str);

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
    bool removeStartStopCommentFromRegion(const KTextEditor::Cursor &start, const KTextEditor::Cursor &end, int attrib=0);

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
     * @return whether the operation succeeded.
     */
    bool removeStartStopCommentFromSelection( KateView *view, int attrib=0 );
    /**
     * @see removeStartStopCommentFromSelection.
     */
    bool removeStartLineCommentFromSelection( KateView *view, int attrib=0 );

  public:
    QString getWord( const KTextEditor::Cursor& cursor );

  public:
    void newBracketMark( const KTextEditor::Cursor& start, KTextEditor::Range& bm, int maxLines = -1 );
    bool findMatchingBracket( KTextEditor::Range& range, int maxLines = -1 );

  public:
    virtual const QString &documentName () const { return m_docName; }

  private:
    void updateDocName ();

  public:
    void lineInfo (KateLineInfo *info, int line) const;

    KateCodeFoldingTree *foldingTree ();

  public:
    /**
     * @return whether the document is modified on disk since last saved
     */
    bool isModifiedOnDisc() { return m_modOnHd; }

    virtual void setModifiedOnDisk( ModifiedOnDiskReason reason );

    virtual void setModifiedOnDiskWarning ( bool on );

  public Q_SLOTS:
    /**
     * Ask the user what to do, if the file has been modified on disk.
     * Reimplemented from KTextEditor::Document.
     */
    virtual void slotModifiedOnDisk( KTextEditor::View *v = 0 );

    /**
     * Reloads the current document from disk if possible
     */
    virtual bool documentReload ();

    virtual bool documentSave ();
    virtual bool documentSaveAs ();

    virtual bool save();
  public:
    virtual bool saveAs( const KUrl &url );

  Q_SIGNALS:
    /**
     * Indicate this file is modified on disk
     * @param doc the KTextEditor::Document object that represents the file on disk
     * @param isModified indicates the file was modified rather than created or deleted
     * @param reason the reason we are emitting the signal.
     */
    void modifiedOnDisk (KTextEditor::Document *doc, bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);

  public:
    void ignoreModifiedOnDiskOnce();

  private:
    int m_isasking; // don't reenter slotModifiedOnDisk when this is true
                    // -1: ignore once, 0: false, 1: true

  public:
    virtual bool setEncoding (const QString &e);
    virtual const QString &encoding() const;


  public Q_SLOTS:
    void setWordWrap (bool on);
    void setWordWrapAt (uint col);

  public:
    bool wordWrap() const;
    uint wordWrapAt() const;

  public Q_SLOTS:
    void setPageUpDownMovesCursor(bool on);

  public:
    bool pageUpDownMovesCursor() const;

   // code folding
  public:
    uint getRealLine(unsigned int virtualLine);
    uint getVirtualLine(unsigned int realLine);
    uint visibleLines ();
    Kate::TextLine kateTextLine(uint i);
    Kate::TextLine plainKateTextLine(uint i);

  Q_SIGNALS:
    void codeFoldingUpdated();
    void aboutToRemoveText(const KTextEditor::Range&);

  private Q_SLOTS:
    void slotModOnHdDirty (const QString &path);
    void slotModOnHdCreated (const QString &path);
    void slotModOnHdDeleted (const QString &path);

  private:
    /**
     * create a MD5 digest of the file, if it is a local file.
     * The result can be accessed through KateBuffer::digest().
     * This is using KMD5::hexDigest().
     *
     * @return wheather the operation was attempted and succeeded.
     */
    bool createDigest ();
    
    /**
     * create a string for the modonhd warnings, giving the reason.
     */
    QString reasonedMOHString() const;

    /**
     * Removes all trailing whitespace in the document.
     */
    void removeTrailingSpaces();

  public:
    /**
     * md5 digest of this document
     * @return md5 digest for this document
     */
    const QByteArray &digest () const;

    void updateFileType (const QString &newType, bool user = false);

    QString fileType () const { return m_fileType; }

    /**
     * Get access to buffer of this document.
     * Is needed to create cursors and ranges for example.
     * @return document buffer
     */
    KateBuffer &buffer () { return *m_buffer; }
    
    /**
     * set indentation mode by user
     * this will remember that a user did set it and will avoid reset on save
     */
    void rememberUserDidSetIndentationMode ()
    {
      m_indenterSetByUser = true;
    }
    
    /**
     * User did set encoding for next reload => enforce it!
     */
    void userSetEncodingForNextReload ()
    {
      m_userSetEncodingForNextReload = true;
    }

  //
  // REALLY internal data ;)
  //
  private:
    // text buffer
    KateBuffer *const m_buffer;

    // indenter
    KateAutoIndent *const m_indenter;

    bool m_hlSetByUser;
    bool m_bomSetByUser;
    bool m_indenterSetByUser;
    bool m_userSetEncodingForNextReload;

    bool m_modOnHd;
    ModifiedOnDiskReason m_modOnHdReason;

    QString m_docName;
    int m_docNameNumber;

    // file type !!!
    QString m_fileType;
    bool m_fileTypeSetByUser;

    /**
     * document is still reloading a file
     */
    bool m_reloading;

  public Q_SLOTS:
    void slotQueryClose_save(bool *handled, bool* abortClosing);

  public:
    virtual bool queryClose();

    void makeAttribs (bool needInvalidate = true);

    static bool checkOverwrite( KUrl u, QWidget *parent );

  /**
   * Configuration
   */
  public:
    KateDocumentConfig *config() { return m_config; }
    KateDocumentConfig *config() const { return m_config; }

    void updateConfig ();

  private:
    KateDocumentConfig *const m_config;

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

    /**
     * helper regex to capture the document variables
     */
    static QRegExp kvLine;
    static QRegExp kvLineWildcard;
    static QRegExp kvLineMime;
    static QRegExp kvVar;

    bool m_fileChangedDialogsActivated;

  //
  // KTextEditor::ConfigInterface
  //
  public:
     virtual QStringList configKeys() const;
     virtual QVariant configValue(const QString &key);
     virtual void setConfigValue(const QString &key, const QVariant &value);

  //
  // KTextEditor::RecoveryInterface
  //
  public:
    virtual bool isDataRecoveryAvailable() const;
    virtual void recoverData();
    virtual void discardDataRecovery();

  //
  // KTextEditor::HighlightInterface
  //
  public:
    virtual KTextEditor::Attribute::Ptr defaultStyle(const KTextEditor::HighlightInterface::DefaultStyle ds) const;
    virtual QList< KTextEditor::HighlightInterface::AttributeBlock > lineAttributes(const unsigned int line);
    virtual QStringList embeddedHighlightingModes() const;
    virtual QString highlightingModeAt(const KTextEditor::Cursor& position);

  //
  //BEGIN: KTextEditor::MessageInterface
  //
  public:
    virtual bool postMessage(KTextEditor::Message* message);

  public Q_SLOTS:
    void messageDestroyed(KTextEditor::Message* message);

  private:
    QHash<KTextEditor::Message *, QList<QSharedPointer<QAction> > > m_messageHash;
  //END KTextEditor::MessageInterface

  protected Q_SLOTS:
      void dumpRegionTree();

  public:
      QString defaultDictionary() const;
      QList<QPair<KTextEditor::MovingRange*, QString> > dictionaryRanges() const;
      bool isOnTheFlySpellCheckingEnabled() const;

      QString dictionaryForMisspelledRange(const KTextEditor::Range& range) const;
      void clearMisspellingForWord(const QString& word);

  public Q_SLOTS:
      void clearDictionaryRanges();
      void setDictionary(const QString& dict, const KTextEditor::Range &range);
      void revertToDefaultDictionary(const KTextEditor::Range &range);
      void setDefaultDictionary(const QString& dict);
      void onTheFlySpellCheckingEnabled(bool enable);
      void respellCheckBlock(int start, int end) {respellCheckBlock(this,start,end);}
      void refreshOnTheFlyCheck(const KTextEditor::Range &range = KTextEditor::Range::invalid());

  Q_SIGNALS:
      void respellCheckBlock(KateDocument *document,int start, int end);
      void dictionaryRangesPresent(bool yesNo);
      void defaultDictionaryChanged(KateDocument *document);

  public:
      bool containsCharacterEncoding(const KTextEditor::Range& range);

      typedef QList<QPair<int, int> > OffsetList;

      int computePositionWrtOffsets(const OffsetList& offsetList, int pos);

      /**
       * The first OffsetList is from decoded to encoded, and the second OffsetList from
       * encoded to decoded.
       **/
      QString decodeCharacters(const KTextEditor::Range& range,
                               KateDocument::OffsetList& decToEncOffsetList,
                               KateDocument::OffsetList& encToDecOffsetList);
      void replaceCharactersByEncoding(const KTextEditor::Range& range);

      enum EncodedCharaterInsertionPolicy {EncodeAlways, EncodeWhenPresent, EncodeNever};

  protected:
      KateOnTheFlyChecker *m_onTheFlyChecker;
      QString m_defaultDictionary;
      QList<QPair<KTextEditor::MovingRange*, QString> > m_dictionaryRanges;

      // from KTextEditor::MovingRangeFeedback
      void rangeInvalid(KTextEditor::MovingRange *movingRange);
      void rangeEmpty(KTextEditor::MovingRange *movingRange);

      void deleteDictionaryRange(KTextEditor::MovingRange *movingRange);

  private:
    Kate::SwapFile *m_swapfile;
    
  public:
    Kate::SwapFile* swapFile();

  //helpers for scripting and codefolding
    int defStyleNum(int line, int column);
    bool isComment(int line, int column);

  private Q_SLOTS:
    /**
     * watch for all started io jobs to remember if file is perhaps loading atm
     * @param job started job
     */
    void slotStarted(KIO::Job *job);
    void slotCompleted();
    void slotCanceled();
    
    /**
     * trigger display of loading message, after 1000 ms
     */
    void slotTriggerLoadingMessage ();
    
    /**
     * Abort loading
     */
    void slotAbortLoading ();
    
  private:
    /**
     * different possible states
     */
    enum DocumentStates {
      /**
       * Idle
       */
      DocumentIdle,
      
      /**
       * Loading
       */
      DocumentLoading,
      
      /**
       * Saving
       */
      DocumentSaving,
      
      /**
       * Pre Saving As, this is between ::saveAs is called and ::save
       */
      DocumentPreSavingAs,
      
      /**
       * Saving As
       */
      DocumentSavingAs
    };
    
    /**
     * current state
     */
    DocumentStates m_documentState;
    
    /**
     * read-write state before loading started
     */
    bool m_readWriteStateBeforeLoading;
    
    /**
     * loading job, we want to cancel with cancel in the loading message
     */
    QPointer<KJob> m_loadingJob;
    
    /**
     * message to show during loading
     */
    QPointer<KTextEditor::Message> m_loadingMessage;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

