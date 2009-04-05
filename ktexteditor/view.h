/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

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

#ifndef KDELIBS_KTEXTEDITOR_VIEW_H
#define KDELIBS_KTEXTEDITOR_VIEW_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/range.h>

// gui merging
#include <kxmlguiclient.h>

// widget
#include <QtGui/QWidget>

class QMenu;

namespace KTextEditor
{

class Document;

/**
 * \brief A text widget with KXMLGUIClient that represents a Document.
 *
 * Topics:
 *  - \ref view_intro
 *  - \ref view_hook_into_gui
 *  - \ref view_selection
 *  - \ref view_cursors
 *  - \ref view_mouse_tracking
 *  - \ref view_modes
 *  - \ref view_extensions
 *
 * \section view_intro Introduction
 *
 * The View class represents a single view of a KTextEditor::Document,
 * get the document on which the view operates with document().
 * A view provides both the graphical representation of the text and the
 * KXMLGUIClient for the actions. The view itself does not provide
 * text manipulation, use the methods from the Document instead. The only
 * method to insert text is insertText(), which inserts the given text
 * at the current cursor position and emits the signal textInserted().
 *
 * Usually a view is created by using Document::createView().
 * Furthermore a view can have a context menu. Set it with setContextMenu()
 * and get it with contextMenu().
 *
 * \section view_hook_into_gui Merging the View's GUI
 *
 * A View is derived from the class KXMLGUIClient, so its GUI elements (like
 * menu entries and toolbar items) can be merged into the application's GUI
 * (or into a KXMLGUIFactory) by calling
 * \code
 * // view is of type KTextEditor::View*
 * mainWindow()->guiFactory()->addClient( view );
 * \endcode
 * You can add only one view as client, so if you have several views, you first
 * have to remove the current view, and then add the new one, like this
 * \code
 * mainWindow()->guiFactory()->removeClient( currentView );
 * mainWindow()->guiFactory()->addClient( newView );
 * \endcode
 *
 * \section view_selection Text Selection
 *
 * As the view is a graphical text editor it provides \e normal and \e block
 * text selection. You can check with selection() whether a selection exists.
 * removeSelection() will remove the selection without removing the text,
 * whereas removeSelectionText() also removes both, the selection and the
 * selected text. Use selectionText() to get the selected text and
 * setSelection() to specify the selected textrange. The signal
 * selectionChanged() is emitted whenever the selecteion changed.
 *
 * \section view_cursors Cursor Positions
 *
 * A view has one Cursor which represents a line/column tuple. Two different
 * kinds of cursor positions are supported: first is the \e real cursor
 * position where a \e tab character only counts one character. Second is the
 * \e virtual cursor position, where a \e tab character counts as many
 * spaces as defined. Get the real position with cursorPosition() and the
 * virtual position with cursorPositionVirtual(). Set the real cursor
 * position with setCursorPosition(). You can even get the screen coordinates
 * of the current cursor position in pixel by using
 * cursorPositionCoordinates(). The signal cursorPositionChanged() is emitted
 * whenever the cursor position changed.
 *
 * \section view_mouse_tracking Mouse Tracking
 *
 * It is possible to get notified via the signal mousePositionChanged() for
 * mouse move events, if mouseTrackingEnabled() returns \e true. Mouse tracking
 * can be turned on/off by calling setMouseTrackingEnabled(). If an editor
 * implementation does not support mouse tracking, mouseTrackingEnabled() will
 * always return \e false.
 *
 * \section view_modes Edit Modes
 *
 * A view supports several edit modes (EditMode). Common edit modes are
 * \e insert-mode (INS) and \e overwrite-mode (OVR). Which edit modes the
 * editor supports depends on the implementation, another well-known mode is
 * the \e command-mode for example in vim and yzis. The getter viewMode()
 * returns a string like \p INS or \p OVR and is represented in the user
 * interface for example in the status bar. Further you can get the edit
 * mode as enum by using viewEditMode(). Whenever the edit mode changed the
 * signals viewModeChanged() and viewEditModeChanged() are emitted.
 *
 * \section view_extensions View Extension Interfaces
 *
 * A simple view represents the text of a Document and provides a text cursor,
 * text selection, edit modes etc.
 * Advanced concepts like code completion and text hints are defined in the
 * extension interfaces. An KTextEditor implementation does not need to
 * support all the extensions. To implement the interfaces multiple
 * inheritance is used.
 *
 * More information about interfaces for the view can be found in
 * \ref kte_group_view_extensions.
 *
 * \see KTextEditor::Document, KTextEditor::TemplateInterface,
 *      KTextEditor::CodeCompletionInterface,
 *      KTextEditor::SessionConfigInterface, KTextEditor::TemplateInterface,
 *      KXMLGUIClient
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT View :  public QWidget, public KXMLGUIClient
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a view attached to the widget \p parent.
     * \param parent parent widget
     * \see Document::createView()
     */
    View ( QWidget *parent );

    /**
     * Virtual destructor.
     */
    virtual ~View ();

  /*
   * Accessor for the document
   */
  public:
    /**
     * Get the view's \e document, that means the view is a view of the
     * returned document.
     * \return the view's document
     */
    virtual Document *document () const = 0;

    /**
     * Check whether this view is the document's active view.
     * This is equal to the code:
     * \code
     * document()->activeView() == view
     * \endcode
     */
    bool isActiveView() const;

  /*
   * General information about this view
   */
  public:
    /**
     * Get the current view mode/state.
     * This can be used to visually indicate the view's current mode, for
     * example \e INSERT mode, \e OVERWRITE mode or \e COMMAND mode - or
     * whatever other edit modes are supported. The string should be
     * translated (i18n), as this is a user aimed representation of the view
     * state, which should be shown in the GUI, for example in the status bar.
     * \return
     * \see viewModeChanged()
     */
    virtual QString viewMode () const = 0;

    /**
     * Possible edit modes.
     * These correspond to various modes the text editor might be in.
     */
    enum EditMode {
      EditInsert = 0,   /**< Insert mode. Characters will be added. */
      EditOverwrite = 1 /**< Overwrite mode. Characters will be replaced. */
    };

    /**
     * Get the view's current edit mode.
     * The current mode can be \e insert mode, \e replace mode or any other
     * the editor supports, e.g. a vim like \e command mode. If in doubt
     * return EditInsert.
     *
     * \return the current edit mode of this view
     * \see viewEditModeChanged()
     */
    virtual enum EditMode viewEditMode() const = 0;

  /*
   * SIGNALS
   * following signals should be emitted by the editor view
   */
  Q_SIGNALS:
    /**
     * This signal is emitted whenever the \p view gets the focus.
     * \param view view which gets focus
     * \see focusOut()
     */
    void focusIn ( KTextEditor::View *view );

    /**
     * This signal is emitted whenever the \p view loses the focus.
     * \param view view which lost focus
     * \see focusIn()
     */
    void focusOut ( KTextEditor::View *view );

    /**
     * This signal is emitted whenever the view mode of \p view changes.
     * \param view the view which changed its mode
     * \see viewMode()
     */
    void viewModeChanged ( KTextEditor::View *view );

    /**
     * This signal is emitted whenever the \p view's edit \p mode changed from
     * either EditInsert to EditOverwrite or vice versa.
     * \param view view which changed its edit mode
     * \param mode new edit mode
     * \see viewEditMode()
     */
    void viewEditModeChanged ( KTextEditor::View *view,
                               enum KTextEditor::View::EditMode mode );

    /**
     * This signal is emitted whenever the \p view wants to display a
     * information \p message. The \p message can be displayed in the status bar
     * for example.
     * \param view view which sends out information
     * \param message information message
     */
    void informationMessage ( KTextEditor::View *view, const QString &message );

    /**
     * This signal is emitted from \p view whenever the users inserts \p text
     * at \p position, that means the user typed/pasted text.
     * \param view view in which the text was inserted
     * \param position position where the text was inserted
     * \param text the text the user has typed into the editor
     * \see insertText()
     */
    void textInserted ( KTextEditor::View *view,
                        const KTextEditor::Cursor &position,
                        const QString &text );

  /*
   * Context menu handling
   */
  public:
    /**
     * Set a context menu for this view to \p menu.
     *
     * \note any previously assigned menu is not deleted.  If you are finished
     *       with the previous menu, you may delete it.
     *
     * \param menu new context menu object for this view
     * \see contextMenu()
     */
    virtual void setContextMenu ( QMenu *menu ) = 0;

    /**
     * Get the context menu for this view. The return value can be NULL
     * if no context menu object was set.
     * \return context menu object
     * \see setContextMenu()
     */
    virtual QMenu *contextMenu () const = 0;

    /**
     * Populate \a menu with default text editor actions.  If \a menu is
     * null, a menu will be created with the view as its parent.
     *
     * \note to use this menu, you will next need to call setContextMenu(),
     *       as this does not assign the new context menu.
     *
     * \param menu the menu to be populated, or null to create a new menu
     * \return the menu, whether created or passed initially
     */
    virtual QMenu* defaultContextMenu(QMenu* menu = 0L) const = 0;

  Q_SIGNALS:
    /**
     * Signal which is emitted immediately prior to showing the current
     * context \a menu.
     */
    void contextMenuAboutToShow(KTextEditor::View* view, QMenu* menu);

  /*
   * Cursor handling
   */
  public:
    /**
     * Set the view's new cursor to \p position. A \e TAB character
     * is handeled as only on character.
     * \param position new cursor position
     * \return \e true on success, otherwise \e false
     * \see cursorPosition()
     */
    virtual bool setCursorPosition (Cursor position) = 0;

    /**
     * Get the view's current cursor position. A \e TAB character is
     * handeled as only one character.
     * \return current cursor position
     * \see setCursorPosition()
     */
    virtual Cursor cursorPosition () const = 0;

    /**
     * Get the current \e virtual cursor position, \e virtual means the
     * tabulator character (TAB) counts \e multiple characters, as configured
     * by the user (e.g. one TAB is 8 spaces). The virtual cursor
     * position provides access to the user visible values of the current
     * cursor position.
     *
     * \return virtual cursor position
     * \see cursorPosition()
     */
    virtual Cursor cursorPositionVirtual () const = 0;

    /**
     * Get the screen coordinates (x, y) of the supplied \a cursor relative
     * to the view widget in pixels. Thus, 0,0 represents the top left hand of
     * the view widget.
     *
     * \param cursor cursor to determine coordinate for.
     * \return cursor screen coordinates relative to the view widget
     */
    virtual QPoint cursorToCoordinate(const KTextEditor::Cursor& cursor) const = 0;

    /**
     * Get the screen coordinates (x/y) of the cursor position in pixels.
     * \return cursor screen coordinates
     */
    virtual QPoint cursorPositionCoordinates () const = 0;

  /*
   * SIGNALS
   * following signals should be emitted by the editor view
   * if the cursor position changes
   */
  Q_SIGNALS:
    /**
     * This signal is emitted whenever the \p view's cursor position changed.
     * \param view view which emitted the signal
     * \param newPosition new position of the cursor (Kate will pass the real
     *        cursor potition, not the virtual)
     * \see cursorPosition(), cursorPositionVirtual()
     */
    void cursorPositionChanged (KTextEditor::View *view,
                                const KTextEditor::Cursor& newPosition);

    /**
     * This signal should be emitted whenever the \p view is scrolled vertically.
     * \param view view which emitted the signal
     * \param newPos the new scroll position
     */
    void verticalScrollPositionChanged (KTextEditor::View *view, const KTextEditor::Cursor& newPos);
  
    /**
     * This signal should be emitted whenever the \p view is scrolled horizontally.
     * \param view view which emitted the signal
     */
    void horizontalScrollPositionChanged (KTextEditor::View *view);
  /*
   * Mouse position
   */
  public:
    /**
     * Check, whether mouse tracking is enabled.
     *
     * Mouse tracking is required to have the signal mousePositionChanged()
     * emitted.
     * \return \e true, if mouse tracking is enabled, otherwise \e false
     * \see setMouseTrackingEnabled(), mousePositionChanged()
     */
    virtual bool mouseTrackingEnabled() const = 0;

    /**
     * Try to enable or disable mouse tracking according to \p enable.
     * The return value contains the state of mouse tracking \e after the
     * request. Mouse tracking is required to have the mousePositionChanged()
     * signal emitted.
     *
     * \note Implementation Notes: An implementation is not forced to support
     *       this, and should always return \e false if it does not have
     *       support.
     *
     * \param enable if \e true, try to enable mouse tracking, otherwise disable
     *        it.
     * \return the current state of mouse tracking
     * \see mouseTrackingEnabled(), mousePositionChanged()
     */
    virtual bool setMouseTrackingEnabled(bool enable) = 0;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the position of the mouse changes over
     * this \a view. If the mouse moves off the view, an invalid cursor position
     * should be emitted, i.e. Cursor::invalid().
     * \note If mouseTrackingEnabled() returns \e false, this signal is never
     *       emitted.
     * \param view view which emitted the signal
     * \param newPosition new position of the mouse or Cursor::invalid(), if the
     *        mouse moved out of the \p view.
     * \see mouseTrackingEnabled()
     */
    void mousePositionChanged (KTextEditor::View *view,
                               const KTextEditor::Cursor& newPosition);

  /*
   * Selection methodes.
   * This deals with text selection and copy&paste
   */
  public:
    /**
     * Set the view's selection to the range \p selection.
     * The old selection will be discarded.
     * \param range the range of the new selection
     * \return \e true on success, otherwise \e false (e.g. when the cursor
     *         range is invalid)
     * \see selectionRange(), selection()
     */
    virtual bool setSelection ( const Range &range ) = 0;

    /**
     * This is an overloaded member function, provided for convenience, it
     * differs from the above function only in what argument(s) it accepts.
     * An existing old selection will be discarded. If possible you should
     * reimplement the default implementation with a more efficient one.
     * \param position start or end position of the selection, depending
     *        on the \p length parameter
     * \param length if >0 \p position defines the start of the selection,
     *        if <0 \p position specifies the end
     * \param wrap if \e false the selection does not wrap lines and reaches
     *        only to start/end of the cursors line. Default: \e true
     * \see selectionRange(), selection()
     *
     * \todo rodda - is this really needed? it can now be accomplished with
     *       SmartCursor::advance()
     */
    virtual bool setSelection ( const Cursor &position,
                                int length,
                                bool wrap = true );

    /**
     * Query the view whether it has selected text, i.e. whether a selection
     * exists.
     * \return \e true if a text selection exists, otherwise \e false
     * \see setSelection(), selectionRange()
     */
    virtual bool selection() const = 0;

    /**
     * Get the range occupied by the current selection.
     * \return selection range, valid only if a selection currently exists.
     * \see setSelection()
     */
    virtual const Range &selectionRange() const = 0;

    /**
     * Get the view's selected text.
     * \return the selected text
     * \see setSelection()
     */
    virtual QString selectionText () const = 0;

    /**
     * Remove the view's current selection, \e without deleting the selected
     * text.
     * \return \e true on success, otherwise \e false
     * \see removeSelectionText()
     */
    virtual bool removeSelection () = 0;

    /**
     * Remove the view's current selection \e including the selected text.
     * \return \e true on success, otherwise \e false
     * \see removeSelection()
     */
    virtual bool removeSelectionText () = 0;

  /*
   * Blockselection stuff
   */
  public:
   /**
    * Set block selection mode to state \p on.
    * \param on if \e true, block selection mode is turned on, otherwise off
    * \return \e true on success, otherwise \e false
    * \see blockSelection()
    */
    virtual bool setBlockSelection (bool on) = 0;

   /**
    * Get the status of the selection mode. \e true indicates that block
    * selection mode is on. If this is \e true, selections applied via the
    * SelectionInterface are handled as block selections and the Copy&Paste
    * functions work on rectangular blocks of text rather than normal.
    * \return \e true, if block selection mode is enabled, otherwise \e false
    * \see setBlockSelection()
    */
    virtual bool blockSelection () const = 0;

  /*
   * SIGNALS
   * following signals should be emitted by the editor view for selection
   * handling.
   */
  Q_SIGNALS:
    /**
     * This signal is emitted whenever the \p view's selection changes.
     * \note If the mode switches from block selection to normal selection
     *       or vice versa this signal should also be emitted.
     * \param view view in which the selection changed
     * \see selection(), selectionRange(), selectionText()
     */
    void selectionChanged (KTextEditor::View *view);

  public:
    /**
     * This is a convenience function which inserts \p text at the view's
     * current cursor position. You do not necessarily need to reimplement
     * it, except you want to do some special things.
     * \param text Text to be inserted
     * \return \e true on success of insertion, otherwise \e false
     * \see textInserted()
     */
    virtual bool insertText (const QString &text);

  private:
    class ViewPrivate* const d;
};

/**
 * \brief Pixel coordinate to Cursor extension interface for the View.
 *
 * \ingroup kte_group_view_extensions
 *
 * \section ctc_intro Introduction
 *
 * The CoordinatesToCursorInterface makes it possible to map a
 * pixel coordinate to a cursor position. To map a cursor position
 * to pixel coordinates use one of
 * - KTextEditor::View::cursorToCoordinate()
 * - KTextEditor::View::cursorPositionCoordinates()
 *
 * \section ctc_access Accessing the CoordinatesToCursorInterface
 *
 * The CoordinatesToCursorInterface is an extension interface for a
 * View, i.e. the View inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // view is of type KTextEditor::View*
 * KTextEditor::CoordinatesToCursorInterface *iface =
 *     qobject_cast<KTextEditor::CoordinatesToCursorInterface*>( view );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::View
 * \since 4.2
 * \note KDE5: merge into KTextEditor::View
 */
class KTEXTEDITOR_EXPORT CoordinatesToCursorInterface
{
  public:
    /** Virtual destructor. */
    virtual ~CoordinatesToCursorInterface();

    /**
     * Get the text-cursor in the document from the screen coordinates,
     * relative to the view widget.
     *
     * To map a cursor to pixel coordinates (the reverse transformation)
     * use KTextEditor::View::cursorToCoordinate().
     *
     * \param coord coordinates relative to the view widget
     * \return cursor in the View, that points onto the character under
     *         the given coordinate. May be KTextEditor::Cursor::invalid().
     */
    virtual KTextEditor::Cursor coordinatesToCursor(const QPoint& coord) const = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::CoordinatesToCursorInterface, "org.kde.KTextEditor.CoordinatesToCursorInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
