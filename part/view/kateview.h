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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef kate_view_h
#define kate_view_h

#include "katetextline.h"

#include <ktexteditor/view.h>
#include <ktexteditor/texthintinterface.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/templateinterface.h>
#include <ktexteditor/rangefeedback.h>
#include <ktexteditor/configinterface.h>

#include <QtCore/QPointer>
#include <QtGui/QMenu>
#include <QtCore/QLinkedList>
#include <QtCore/QHash>

#include <kdebug.h>

class KateDocument;
class KateBookmarks;
class KateCmdLine;
class KateViewConfig;
class KateRenderer;
class KateSpell;
class KateCompletionWidget;
class KateSmartRange;
class KateViewInternal;
class KateSearchBar;
class KateViewBar;
class KateGotoBar;

class KToggleAction;
class KAction;
class KRecentFilesAction;
class KSelectAction;

class QVBoxLayout;


//
// Kate KTextEditor::View class ;)
//
class KateView : public KTextEditor::View,
                 public KTextEditor::TextHintInterface,
                 public KTextEditor::SessionConfigInterface,
                 public KTextEditor::TemplateInterface,
                 public KTextEditor::CodeCompletionInterface,
                 public KTextEditor::ConfigInterface,
		 private KTextEditor::SmartRangeWatcher
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::TextHintInterface)
    Q_INTERFACES(KTextEditor::SessionConfigInterface)
    Q_INTERFACES(KTextEditor::TemplateInterface)
    Q_INTERFACES(KTextEditor::ConfigInterface)
    Q_INTERFACES(KTextEditor::CodeCompletionInterface)
    friend class KateViewInternal;
    friend class KateIconBorder;
    friend class KateSearchBar;

  public:
    KateView( KateDocument* doc, QWidget* parent );
    ~KateView ();

    KTextEditor::Document *document () const;

    QString viewMode () const;

  //
  // KTextEditor::ClipboardInterface
  //
  public Q_SLOTS:
    // TODO: Factor out of m_viewInternal
    void paste();
    void cut();
    void copy() const;

  private Q_SLOTS:
    /**
     * internal use, copy text as HTML to clipboard
     */
    void copyHTML();

    /**
     * internal use, apply word wrap
     */
    void applyWordWrap ();

  // helper to export text as html stuff
  private:
    QString selectionAsHtml ();
    QString textAsHtml ( KTextEditor::Range range, bool blockwise);
    void textAsHtmlStream ( const KTextEditor::Range& range, bool blockwise, QTextStream *ts);

    /**
     * Gets a substring in valid-xml html.
     * Example:  "<b>const</b> b = <i>34</i>"
     * It won't contain <p> or <body> or <html> or anything like that.
     *
     * @param startCol start column of substring
     * @param length length of substring
     * @param renderer The katerenderer.  This will have the schema
     *                 information that describes how to render the
     *                 attributes.
     * @param outputStream A stream to write the html to
     */
    void lineAsHTML (KateTextLine::Ptr line, int startCol, int length, QTextStream *outputStream);

  public Q_SLOTS:
    void exportAsHTML ();

  //
  // KTextEditor::PopupMenuInterface
  //
  public:
    void setContextMenu( QMenu* menu );
    QMenu* contextMenu() const;
    QMenu* defaultContextMenu(QMenu* menu = 0L) const;

  private Q_SLOTS:
    void aboutToShowContextMenu();

  private:
    QPointer<QMenu> m_contextMenu;

  //
  // KTextEditor::ViewCursorInterface
  //
  public:
    bool setCursorPosition (KTextEditor::Cursor position);

    KTextEditor::Cursor cursorPosition () const;

    KTextEditor::Cursor cursorPositionVirtual () const;

    QPoint cursorToCoordinate(const KTextEditor::Cursor& cursor) const;

    QPoint cursorPositionCoordinates() const;

    bool setCursorPositionVisual( const KTextEditor::Cursor& position );

    /**
     * Return the virtual cursor column, each tab is expanded into the
     * document's tabWidth characters. If word wrap is off, the cursor may be
     * behind the line's length.
     */
    int virtualCursorColumn() const;

    virtual bool mouseTrackingEnabled() const;
    virtual bool setMouseTrackingEnabled(bool enable);

  private:
    void notifyMousePositionChanged(const KTextEditor::Cursor& newPosition);

  // Internal
  public:
    bool setCursorPositionInternal( const KTextEditor::Cursor& position, uint tabwidth = 1, bool calledExternally = false );

  //
  // KTextEditor::ConfigInterface
  //
  public:
     QStringList configKeys() const;
     QVariant configValue(const QString &key);
     void setConfigValue(const QString &key, const QVariant &value);

  //
  // KTextEditor::CodeCompletionInterface2
  //
  public:
    virtual bool isCompletionActive() const;
    virtual void startCompletion(const KTextEditor::Range& word, KTextEditor::CodeCompletionModel* model);
    virtual void abortCompletion();
    virtual void forceCompletion();
    virtual void registerCompletionModel(KTextEditor::CodeCompletionModel* model);
    virtual void unregisterCompletionModel(KTextEditor::CodeCompletionModel* model);
    virtual bool isAutomaticInvocationEnabled() const;
    virtual void setAutomaticInvocationEnabled(bool enabled = true);

  Q_SIGNALS:
    void completionExecuted(KTextEditor::View* view, const KTextEditor::Cursor& position, KTextEditor::CodeCompletionModel* model, int row);
    void completionAborted(KTextEditor::View* view);

  public Q_SLOTS:
    void userInvokedCompletion();

  public:
    KateCompletionWidget* completionWidget() const;
    mutable KateCompletionWidget* m_completionWidget;
    void sendCompletionExecuted(const KTextEditor::Cursor& position, KTextEditor::CodeCompletionModel* model, int row);
    void sendCompletionAborted();

  //
  // KTextEditor::TextHintInterface
  //
  public:
    void enableTextHints(int timeout);
    void disableTextHints();

  Q_SIGNALS:
    void needTextHint(const KTextEditor::Cursor& position, QString &text);

  public:
    bool dynWordWrap() const      { return m_hasWrap; }

  //
  // KTextEditor::SelectionInterface stuff
  //
  public Q_SLOTS:
    virtual bool setSelection ( const KTextEditor::Range &selection );

    virtual bool removeSelection () { return clearSelection(); }

    virtual bool removeSelectionText () { return removeSelectedText(); }

    virtual bool setBlockSelection (bool on) { return setBlockSelectionMode (on); }

    bool clearSelection ();
    bool clearSelection (bool redraw, bool finishedChangingSelection = true);

    bool removeSelectedText ();

    bool selectAll();

  public:
    virtual bool selection() const;
    virtual QString selectionText() const;
    virtual bool blockSelection() const { return blockSelectionMode(); }
    virtual const KTextEditor::Range &selectionRange() const;

  //
  // Arbitrary Syntax HL + Action extensions
  //
  public:
    // Syntax highlighting extension
    void addExternalHighlight(KTextEditor::SmartRange* topRange, bool supportDynamic);
    const QList<KTextEditor::SmartRange*>& externalHighlights() const;
    void clearExternalHighlights();

    void addInternalHighlight(KTextEditor::SmartRange* topRange);
    void removeInternalHighlight(KTextEditor::SmartRange* topRange);
    const QList<KTextEditor::SmartRange*>& internalHighlights() const;

    // Action association extension
    void addActions(KTextEditor::SmartRange* topRange);
    const QList<KTextEditor::SmartRange*>& actions() const;
    void clearActions();

  Q_SIGNALS:
    void dynamicHighlightAdded(KateSmartRange* range);
    void dynamicHighlightRemoved(KateSmartRange* range);

  public Q_SLOTS:
    void removeExternalHighlight(KTextEditor::SmartRange* topRange);
    void removeActions(KTextEditor::SmartRange* topRange);

  private:
    // Smart range watcher overrides
    virtual void rangeDeleted(KTextEditor::SmartRange* range);

    QList<KTextEditor::SmartRange*> m_externalHighlights;
    QList<KTextEditor::SmartRange*> m_externalHighlightsDynamic;
    QList<KTextEditor::SmartRange*> m_internalHighlights;
    QList<KTextEditor::SmartRange*> m_actions;

  //
  // internal helper stuff, for katerenderer and so on
  //
  public:
    // should cursor be wrapped ? take config + blockselection state in account
    bool wrapCursor ();

    // some internal functions to get selection state of a line/col
    bool cursorSelected(const KTextEditor::Cursor& cursor);
    bool lineSelected (int line);
    bool lineEndSelected (const KTextEditor::Cursor& lineEndPos);
    bool lineHasSelected (int line);
    bool lineIsSelection (int line);

    void tagSelection (const KTextEditor::Range &oldSelection);

    void selectWord(   const KTextEditor::Cursor& cursor );
    void selectLine(   const KTextEditor::Cursor& cursor );

  //
  // KTextEditor::BlockSelectionInterface stuff
  //
  public Q_SLOTS:
    bool setBlockSelectionMode (bool on);
    bool toggleBlockSelectionMode ();

  public:
    bool blockSelectionMode() const;


  //BEGIN EDIT STUFF
  public:
    void editStart ();
    void editEnd (int editTagLineStart, int editTagLineEnd, bool tagFrom);

    void editSetCursor (const KTextEditor::Cursor &cursor);
  //END

  //BEGIN TAG & CLEAR
  public:
    bool tagLine (const KTextEditor::Cursor& virtualCursor);

    bool tagRange (const KTextEditor::Range& range, bool realLines = false);
    bool tagLines (int start, int end, bool realLines = false );
    bool tagLines (KTextEditor::Cursor start, KTextEditor::Cursor end, bool realCursors = false);
    bool tagLines (KTextEditor::Range range, bool realRange = false);

    void tagAll ();

    void relayoutRange(const KTextEditor::Range& range, bool realLines = false);

    void clear ();

    void repaintText (bool paintOnlyDirty = false);

    void updateView (bool changed = false);
  //END

  //
  // KTextEditor::View
  //
  public:
    /**
     Return values for "save" related commands.
    */
    bool isOverwriteMode() const;
    enum KTextEditor::View::EditMode viewEditMode() const {return isOverwriteMode() ? KTextEditor::View::EditOverwrite : KTextEditor::View::EditInsert;}
    QString currentTextLine();
    QString currentWord();

  public Q_SLOTS:
    void indent();
    void unIndent();
    void cleanIndent();
    void align();
    void comment();
    void uncomment();
    void killLine();

    /**
      Uppercases selected text, or an alphabetic character next to the cursor.
    */
    void uppercase();
    /**
      Lowercases selected text, or an alphabetic character next to the cursor.
    */
    void lowercase();
    /**
      Capitalizes the selection (makes each word start with an uppercase) or
      the word under the cursor.
    */
    void capitalize();
    /**
      Joins lines touched by the selection
    */
    void joinLines();

    // Note - the following functions simply forward to KateViewInternal
    void keyReturn();
    void backspace();
    void deleteWordLeft();
    void keyDelete();
    void deleteWordRight();
    void transpose();
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
    void shiftTopOfView();
    void bottomOfView();
    void shiftBottomOfView();
    void pageUp();
    void shiftPageUp();
    void pageDown();
    void shiftPageDown();
    void top();
    void shiftTop();
    void bottom();
    void shiftBottom();
    void toMatchingBracket();
    void shiftToMatchingBracket();

    void gotoLine();

  // config file / session management functions
  public:
    void readSessionConfig(const KConfigGroup&);
    void writeSessionConfig(KConfigGroup&);

  public Q_SLOTS:
    void setEol( int eol );
    void find();
    void findSelectedForwards();
    void findSelectedBackwards();
    void replace();
    void findNext();
    void findPrevious();

    void setFoldingMarkersOn( bool enable ); // Not in KTextEditor::View, but should be
    void setIconBorder( bool enable );
    void setLineNumbersOn( bool enable );
    void setScrollBarMarks( bool enable );
    void toggleFoldingMarkers();
    void toggleIconBorder();
    void toggleLineNumbersOn();
    void toggleScrollBarMarks();
    void toggleDynWordWrap ();
    void setDynWrapIndicators(int mode);

  public:
    int getEol() const;

  public:
    KateRenderer *renderer ();

    bool iconBorder();
    bool lineNumbersOn();
    bool scrollBarMarks();
    int dynWrapIndicators();
    bool foldingMarkersOn();

  private Q_SLOTS:
    /**
     * used to update actions after selection changed
     */
    void slotSelectionChanged ();

  public:
    /**
     * accessor to katedocument pointer
     * @return pointer to document
     */
    inline KateDocument*  doc() { return m_doc; }

    KActionCollection* editActionCollection() const { return m_editActions; }

  public Q_SLOTS:
    void slotNewUndo();
    void slotUpdate();
    void toggleInsert();
    void reloadFile();
    void toggleWWMarker();
    void toggleWriteLock();
    void switchToCmdLine ();
    void slotReadWriteChanged ();

  Q_SIGNALS:
    void dropEventPass(QDropEvent*);

  public:
    void slotTextInserted ( KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text);

  protected:
    void contextMenuEvent( QContextMenuEvent* );

  private Q_SLOTS:
    void slotGotFocus();
    void slotLostFocus();
    void slotDropEventPass( QDropEvent* ev );
    void slotSaveCanceled( const QString& error );
    void slotExpandToplevel();
    void slotCollapseLocal();
    void slotExpandLocal();
    void slotConfigDialog ();

  private:
    void setupConnections();
    void setupActions();
    void setupEditActions();
    void setupCodeFolding();

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
    KAction*               m_switchCmdLine;

    KSelectAction*         m_setEndOfLine;

    QAction *m_cut;
    QAction *m_copy;
    QAction *m_copyHTML;
    QAction *m_paste;
    QAction *m_selectAll;
    QAction *m_deSelect;

    KToggleAction *m_toggleBlockSelection;
    KToggleAction *m_toggleInsert;
    KToggleAction *m_toggleWriteLock;

    KateDocument*          m_doc;
    KateViewInternal*      m_viewInternal;
    KateRenderer*          m_renderer;
    KateSpell             *m_spell;
    KateBookmarks*         m_bookmarks;

    QVBoxLayout *m_vBox;

    bool       m_hasWrap;

  private Q_SLOTS:
    void slotNeedTextHint(int line, int col, QString &text);
    void slotHlChanged();

  /**
   * Configuration
   */
  public:
    inline KateViewConfig *config () { return m_config; }

    void updateConfig ();

    void updateDocumentConfig();

    void updateRendererConfig();

  private Q_SLOTS:
    void updateFoldingConfig ();

  private:
    KateViewConfig *m_config;
    bool m_startingUp;
    bool m_updatingDocumentConfig;

    // stores the current selection
    KateSmartRange* m_selection;

    // do we select normal or blockwise ?
    bool blockSelect;

  /**
   * IM input stuff
   */
  public:
    void setImComposeEvent( bool imComposeEvent ) { m_imComposeEvent = imComposeEvent; }
    bool imComposeEvent () const { return m_imComposeEvent; }

  private:
    bool m_imComposeEvent;

  ///Template stuff
  public:
    virtual bool insertTemplateTextImplementation ( const KTextEditor::Cursor&, const QString &templateString, const QMap<QString,QString> &initialValues);


  /**
   * Accessors to the bars...
   */
  public:
    KateCmdLine *cmdLine ();
    KateSearchBar *searchBar (bool initHintAsPower = false);
    KateGotoBar *gotoBar ();

  /**
   * viewbar + it's widgets
   * they are created on demand...
   */
  private:
    // created in constructor of the view
    KateViewBar *m_viewBar;

    // created on demand..., only access them through the above accessors....
    KateCmdLine *m_cmdLine;
    KateSearchBar *m_searchBar;
    KateGotoBar *m_gotoBar;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
