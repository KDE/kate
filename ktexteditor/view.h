/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __ktexteditor_view_h__
#define __ktexteditor_view_h__

// the very important KTextEditor::Cursor class
#include <ktexteditor/cursor.h>

// gui merging
#include <kxmlguiclient.h>

#include <kdocument/view.h>

// widget
#include <qwidget.h>

class QMenu;

namespace KTextEditor
{

class Document;

/**
 * The View class represents a single view of a KTextEditor::Document
 * The view should provide both the graphical representation of the text
 * and the xmlgui for the actions
 */
class KTEXTEDITOR_EXPORT View : public KDocument::View
{
  Q_OBJECT

  public:
    /**
     * View Constructor.
     * @param parent parent widget
     */
    View ( QWidget *parent ) : KDocument::View( parent ) {}

    /**
     * virtual destructor
     */
    virtual ~View () {}

  /**
   * Accessor for the document
   */
  public:
    /**
     * Get the @e document on which the view operates, i.e. the view is
     * a view of the returned document.
     * @return the view's document
     */
    virtual Document *document () = 0;

  /**
   * General information about this view
   */
  public:
    /**
     * Get the current view mode/state.
     * This can be used to visually indicate the view's current mode, for
     * example that this view's mode is now in @e INSERT mode,
     * or @e OVERWRITE mode, or @e COMMAND mode, or whatever other edit modes
     * are supported. The string should be translated (i18n), as this is a
     * user aimed representation of the view state, which should be shown in
     * the GUI (e.g. in the status bar()).
     * @see viewModeChanged()
     */
    virtual QString viewMode () const = 0;

    /**
     * Possible edit modes. These correspond to various modes the text
     * editor might be in.
     */
    enum EditMode {
      EditInsert = 0,   /**< Insert mode. Characters will be added. */
      EditOverwrite = 1 /**< Overwrite mode. Characters will be replaced. */
    };

    /**
     * Get the view's current edit mode - it should return the current mode,
     * if there is e.g. some vim like command mode active, it should return
     * the edit mode, which is the most possible one after leaving the
     * command mode. If in doubt return EditInsert.
     *
     * @return the current edit mode of this view
     */
    virtual enum EditMode viewEditMode() const = 0;

  /**
   * SIGNALS
   * following signals should be emitted by the editor view
   */
  signals:
    /**
     * This signal is emitted whenever the @e view gets the focus.
     * @param view view which gets focus
     */
    void focusIn ( KTextEditor::View *view );

    /**
     * This signal is emitted whenever the @e view looses the focus.
     * @param view view which looses focus
     */
    void focusOut ( KTextEditor::View *view );

    /**
     * This signal is emitted whenever the view mode of @e view changes.
     * @param view the view which changed its mode
     * @see viewMode()
     */
    void viewModeChanged ( KTextEditor::View *view );

    /**
     * This signal is emitted whenever the @e view's edit @e mode changed from
     * either @c EditInsert to @c EditOverwrite or vice versa.
     * @param view view which changed its edit mode
     * @param mode new edit mode
     */
    void viewEditModeChanged ( KTextEditor::View *view, enum KTextEditor::View::EditMode mode );

    /**
     * information message
     * @param view view which sends out information
     * @param message information message
     */
    void informationMessage ( KTextEditor::View *view, const QString &message );

    /**
     * This signal is emitted from @e view whenever the users inserts @e text
     * at @e position, i.e. the user typed/pasted text.
     * @param view view in which the text was inserted
     * @param position position where the text was inserted
     * @param text the text the user has typed into the editor
     */
    void textInserted ( KTextEditor::View *view, const KTextEditor::Cursor &position, const QString &text );

  /**
   * Context menu handling
   */
  public:
    /**
     * Set a context menu for this view to @e menu.
     * @param menu new context menu object for this view
     */
    virtual void setContextMenu ( QMenu *menu ) = 0;

    /**
     * Get the context menu for this view. The return value can be NULL
     * if no context menu object was set.
     * @return context menu object
     */
    virtual QMenu *contextMenu () = 0;

  /**
   * Cursor handling
   */
  public:
    /**
     * Set the view's new cursor to @e position. A @e TAB character
     * is handeled as only on character.
     * @param position new cursor position
     * @return @e true on success, otherwise @e false
     */
    virtual bool setCursorPosition (const Cursor &position) = 0;

    /**
     * Get the view's current cursor position. A @e TAB character is
     * handeled as only one character.
     * @return current cursor position
     */
    virtual const Cursor &cursorPosition () const = 0;

    /**
     * Get the current @e virtual cursor position, i.e. the tabulator
     * character (TAB) counts @e multiple characters, as configured
     * by the user (e.g. one TAB is 8 spaces). The virtual cursor
     * position provides access to the user visible values of the current
     * cursor position.
     * 
     * @return virtual cursor position
     */
    virtual Cursor cursorPositionVirtual () const = 0;

    /**
     * Get the screen coordinates (x/y) of the cursor position.
     * @return cursor screen coordinates
     */
    virtual QPoint cursorPositionCoordinates () const = 0;

  /**
   * SIGNALS
   * following signals should be emitted by the editor view
   * if the cursor position changes
   */
  signals:
    /**
     * This signal is emitted whenever the @e view's cursor position changed.
     * @param view view which emitted the signal
     * @see cursorPosition()
     * @see cursorPositionVirtual()
     */
    void cursorPositionChanged (KTextEditor::View *view);

  /**
   * Selection methodes
   * This deals with text selection & copy'n'paste
   */
  public:
    /**
     * Set the view's selection to the range specified from @e startPosition
     * to @e endPosition. The old selection will be discarded.
     * @param startPosition start of the new selection
     * @param endPosition end of the new selection
     * @return @e true on success, otherwise @e false (e.g. when the cursor
     *         range is invalid)
     */
    virtual bool setSelection ( const Cursor &startPosition, const Cursor &endPosition ) = 0;

    /**
     * This is an overloaded member function, provided for convenience. It
     * differs from the above function only in what argument(s) it accepts.
     * An existing old selection will be discarded. If possible you should
     * reimplement the default implementation with a more efficient one.
     * @param position start or end position of the selection, depending
     *        on the @e length parameter
     * @param length if >0 @e position defines the start of the selection,
     *        if <0 @e position specifies the end
     * @param wrap if @e false the selection does not wrap lines and reaches
     *        only to start/end of the cursors line. Default: @e true
     */
    virtual bool setSelection ( const Cursor &position, int length, bool wrap = true );

    /**
     * Query the view whether it has selected text, i.e. whether a selection
     * exists.
     * @return @e true if a text selection exists
     */
    virtual bool selection () const = 0;

    /**
     * Get the view's selected text.
     * @return the selected text
     */
    virtual QString selectionText () const = 0;

    /**
     * Remove the view's current selection, @e without deleting the selected
     * text.
     * @return @e true on success, otherwise @e false
     */
    virtual bool removeSelection () = 0;

    /**
     * Remove the view's current selection @e including the selected text.
     * @return @e true on success, otherwise @e flase
     */
    virtual bool removeSelectionText () = 0;

    /**
     * Get the start position of the selection.
     * @return selection start
     */
    virtual const Cursor &selectionStart () const = 0;

    /**
     * Get the end position of the selection.
     * @return selection end
     */
    virtual const Cursor &selectionEnd () const = 0;

  /**
   * Blockselection stuff
   */
  public:
   /**
    * Set block selection mode to state @e on.
    * @param on if @e true, block selection mode is turned on, otherwise off
    * @return @e true on success, otherwise @e false
    */
    virtual bool setBlockSelection (bool on) = 0;

   /**
    * Get the status of the selection mode - @e true indicates that block
    * selection mode is on. If this is @e true, selections applied via the
    * SelectionInterface are handled as block selections and the Copy&Paste
    * functions work on rectangular blocks of text rather than normal.
    * @return @e true, if block selection mode is enabled, otherwise @e false
    */
    virtual bool blockSelection () const = 0;

  /**
   * SIGNALS
   * following signals should be emitted by the editor view for selection
   * handling.
   */
  signals:
    /**
     * This signal is emitted whenever the @e view's selection changes.
     * @note If the mode switches from block selection to normal selection
     *       or vice versa this signal should also be emitted.
     * @param view view in which the selection changed
     */
    void selectionChanged (KTextEditor::View *view);

  public:
    /**
     * This is a convenience function which inserts @e text at the view's
     * current cursor position. You do not necessarily need to reimplement
     * it, except you want to do some special things.
     * @param text Text to be inserted
     * @return success of insertion
     */
    virtual bool insertText (const QString &text);
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
