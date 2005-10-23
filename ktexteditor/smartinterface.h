/* This file is part of the KDE project
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_SMARTINTERFACE_H
#define KDELIBS_KTEXTEDITOR_SMARTINTERFACE_H

#include <ktexteditor/smartrange.h>

namespace KTextEditor
{
class Document;
class View;
class SmartCursor;

/**
 * \brief A Document extension interface for handling SmartCursors and SmartRanges.
 *
 * \ingroup kte_group_doc_extensions
 *
 * Topics:
 *  - \ref intro
 *  - \ref highlight
 *  - \ref action
 *  - \ref access
 *
 * \section intro Introduction
 * Use this interface to:
 * \li create new SmartCursors and SmartRanges;
 * \li create arbitrary highlighting; and
 * \li associate KActions to ranges of text
 *
 * \section creation Creation of SmartCursors and SmartRanges
 * These functions must be used to create SmartCursors and SmartRanges.  This
 * means that these objects cannot be derived from by third party applications.
 *
 * You then own these objects; upon deletion they de-register themselves from
 * the Document with which they were associated.  Alternatively, they are all
 * deleted with the deletion of the owning Document.
 *
 * \section highlight Arbitrary Highlighting Interface
 * Arbitrary highlighting of text can be achieved by creating SmartRanges in a
 * tree structure, and assigning appropriate Attributes to these ranges.
 *
 * To highlight all views, use addHighlightToDocument(); to highlight one or some
 * of the views, use addHighlightToView().  You only need to call this function once
 * per tree; just supply the top range you want to have highlighted.  Calling
 * this function more than once with ranges from the same tree may give undefined results.
 *
 * \section action Action Binding Interface
 * Action binding can be used to associate KActions with specific ranges of text.
 * These bound actions are automatically enabled and disabled when the caret enters
 * their associated ranges, and context menus are automatically populated with the
 * relevant actions.
 *
 * As with the arbitrary highlighting interface, to enable bound actions to be active,
 * call addActionsToDocument() or addActionsToView() on the top SmartRange of a tree.
 * If only small branches of a tree contain actions, it may be more efficient to simply add
 * each of these branches instead (but this is unlikely unless the tree is complex).
 *
 * Note that actions can be bound either directly to the range via
 * SmartRange::attachAction(), or indirectly via Attribute::attachAction().  Using
 * attributes may be more convenient when you want all ranges of a specific type to have
 * the same action associated with them.
 *
 * \todo extend this to provide a signal from the action indicating which range was
 *       used to activate it (if possible)
 *
 * \section access Accessing the Smart Interface
 *
 * The SmartInterface is supposed to be an extension interface for a Document,
 * i.e. the Document inherits the interface @e provided that the 
 * KTextEditor library in use implements the interface. Use dynamic_cast to access
 * the interface:
 * \code
 *   // doc is of type KTextEditor::Document*
 *   KTextEditor::SmartInterface *iface =
 *       dynamic_cast<KTextEditor::SmartInterface*>( doc );
 *
 *   if( iface ) {
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * \endcode
 *
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT SmartInterface
{
  friend class Attribute;

  public:
    SmartInterface();
    virtual ~SmartInterface();

    /**
     * Clears or deletes all instances of smart objects, ie:
     * \li deletes all SmartCursors
     * \li deletes all SmartRanges
     * \li clears all arbitrary highlight ranges
     * \li clears all action binding
     *
     * This method is likely to be noticably faster than deleting and clearing each instance yourself.
     *
     * Deletion occurs without modification to the underlying text.
     */
    virtual void clearSmartInterface() = 0;

    /**
     * Returns whether the smart interface will be cleared on reload of the document.
     *
     * Defaults to true.
     */
    bool clearOnDocumentReload() const;

    /**
     * Specify whether the smart interface should be cleared on reload of the document.
     *
     * \param clearOnReload set to true to enable clearing of the smart interface on reload (the default).
     */
    void setClearOnDocumentReload(bool clearOnReload);

    // BEGIN New cursor methods
    /**
     * \name Smart Cursors
     *
     * The following functions allow for creation and deletion of SmartCursors.
     * \{
     */
    /**
     * Creates a new SmartCursor.
     *
     * You own this object, and may delete it when you are finished with it.
     * Alternatively, you may call the various clear methods, or wait for the Document
     * to be destroyed.
     *
     * \param position The initial cursor position assumed by the new cursor.
     *                 If not specified, it will start at position (0, 0).
     * \param moveOnInsert Define whether the cursor should move when text is inserted at the cursor position.
     */
    virtual SmartCursor* newSmartCursor(const Cursor& position = Cursor::start(), bool moveOnInsert = true) = 0;

    /**
     * \overload
     * \n \n
     * Creates a new SmartCursor.
     *
     * You own this object, and may delete it when you are finished with it.
     * Alternatively, you may call the various clear methods, or wait for the Document
     * to be destroyed.
     *
     * \param moveOnInsert Define whether the cursor should move when text is inserted at the cursor position.
     */
    inline SmartCursor* newSmartCursor(bool moveOnInsert = true)
      { return newSmartCursor(Cursor::start(), moveOnInsert); }

    /**
     * \overload
     * \n \n
     * Creates a new SmartCursor.
     *
     * You own this object, and may delete it when you are finished with it.
     * Alternatively, you may call the various clear methods, or wait for the Document
     * to be destroyed.
     *
     * \param line the line number of the cursor's initial position
     * \param column the line number of the cursor's initial position
     * \param moveOnInsert Define whether the cursor should move when text is inserted at the cursor position.
     */
    inline SmartCursor* newSmartCursor(int line, int column, bool moveOnInsert = true)
      { return newSmartCursor(Cursor(line, column), moveOnInsert); }

    /**
     * Delete all SmartCursors from this document, with the exception of those
     * cursors currently bound to ranges.
     */
    virtual void deleteCursors() = 0;
    // END

    // BEGIN New range methods
    /**
     * \}
     *
     * \name Smart Ranges
     *
     * The following functions allow for creation of new SmartRanges.
     * \{
     */
    /**
     * Creates a new SmartRange.
     * \param range The initial text range assumed by the new range.
     * \param parent The parent SmartRange, if this is to be the child of an existing range.
     * \param insertBehaviour Define whether the range should expand when text is inserted adjacent to the range.
     */
    virtual SmartRange* newSmartRange(const Range& range = Range(), SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand) = 0;

    /**
     * \overload
     * \n \n
     * Creates a new SmartRange.
     * \param startPosition The start position assumed by the new range.
     * \param endPosition The end position assumed by the new range.
     * \param parent The parent SmartRange, if this is to be the child of an existing range.
     * \param insertBehaviour Define whether the range should expand when text is inserted adjacent to the range.
     */
    inline SmartRange* newSmartRange(const Cursor& startPosition, const Cursor& endPosition, SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand)
      { return newSmartRange(Range(startPosition, endPosition), parent, insertBehaviour); }

    /**
     * \overload
     * \n \n
     * Creates a new SmartRange.
     * \param startLine The start line assumed by the new range.
     * \param startColumn The start column assumed by the new range.
     * \param endLine The end line assumed by the new range.
     * \param endColumn The end column assumed by the new range.
     * \param parent The parent SmartRange, if this is to be the child of an existing range.
     * \param insertBehaviour Define whether the range should expand when text is inserted adjacent to the range.
     */
    inline SmartRange* newSmartRange(int startLine, int startColumn, int endLine, int endColumn, SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand)
      { return newSmartRange(Range(startLine, startColumn, endLine, endColumn), parent, insertBehaviour); }

    /**
     * Creates a new SmartRange from pre-existing SmartCursors.  The cursors must not be part of any other range.
     *
     * \param start Start SmartCursor
     * \param end End SmartCursor
     * \param parent The parent SmartRange, if this is to be the child of an existing range.
     * \param insertBehaviour Define whether the range should expand when text is inserted at ends of the range.
     */
    virtual SmartRange* newSmartRange(SmartCursor* start, SmartCursor* end, SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand) = 0;

    /**
     * Delete a SmartRange without deleting the SmartCursors which make up its start() and end().
     *
     * First, extract the cursors yourself using:
     * \code
     *  SmartCursor* start = &range->smartStart();
     *  SmartCursor* end = &range->smartEnd();
     * \endcode
     *
     * Then, call this function to delete the SmartRange instance.  The underlying text will not be affected.
     *
     * \param range the range to dissociate from its smart cursors, and delete
     */
    virtual void unbindSmartRange(SmartRange* range) = 0;

    /**
     * Delete all SmartRanges from this document. This will also delete all
     * cursors currently bound to ranges.
     *
     * This will not affect any underlying text.
     */
    virtual void deleteRanges() = 0;
    // END

    // BEGIN Syntax highlighting extension
    /**
     * \}
     *
     * \name Arbitrary Highlighting
     *
     * The following functions enable highlighting processing for SmartRanges with arbitrary
     * highlighting information.
     * \{
     */
    /**
     * Register a SmartRange tree as providing arbitrary highlighting information,
     * and that it should be rendered on all of the views of a document.
     *
     * \param topRange the top range of the tree to add
     */
    virtual void addHighlightToDocument(SmartRange* topRange) = 0;

    /**
     * Remove a SmartRange tree from providing arbitrary highlighting information
     * to all of the views of a document.
     *
     * \param topRange the top range of the tree to remove
     */
    virtual void removeHighlightFromDocument(SmartRange* topRange) = 0;

    /**
     * Return a list of SmartRanges which are currently registered as
     * providing arbitrary highlighting information to all of the views of a
     * document.
     */
    virtual const QList<SmartRange*> documentHighlights() const = 0;

    /**
     * Clear the highlight ranges from a Document.
     */
    virtual void clearDocumentHighlights() = 0;

    /**
     * Register a SmartRange tree as providing arbitrary highlighting information,
     * and that it should be rendered on the specified \p view.
     *
     * \param view view on which to render the highlight
     * \param topRange the top range of the tree to add
     */
    virtual void addHighlightToView(View* view, SmartRange* topRange) = 0;

    /**
     * Remove a SmartRange tree from providing arbitrary highlighting information
     * to a specific view of a document.
     *
     * \note implementations should not take into account document-bound
     *       highlighting ranges when calling this function; it is intended solely
     *       to be the counter of addHighlightToView()
     *
     * \param view view on which the highlight was previously rendered
     * \param topRange the top range of the tree to remove
     */
    virtual void removeHighlightFromView(View* view, SmartRange* topRange) = 0;

    /**
     * Return a list of SmartRanges which are currently registered as
     * providing arbitrary highlighting information to a specific view of a
     * document.
     *
     * \note implementations should not take into account document-bound
     *       highlighting ranges when returning the list; it is intended solely
     *       to show highlights added via addHighlightToView()
     *
     * \param view view to query for the highlight list
     */
    virtual const QList<SmartRange*> viewHighlights(View* view) const = 0;

    /**
     * Clear the highlight ranges from a View.
     *
     * \param view view to clear highlights from
     */
    virtual void clearViewHighlights(View* view) = 0;
    // END

    // BEGIN Action binding extension
    /**
     * \}
     *
     * \name Action Binding
     *
     * The following functions allow for the processing of KActions bound to SmartRanges.
     * \{
     */
    /**
     * Register a SmartRange tree as providing bound actions,
     * and that they should interact with all of the views of a document.
     *
     * \param topRange the top range of the tree to add
     */
    virtual void addActionsToDocument(SmartRange* topRange) = 0;

    /**
     * Remove a SmartRange tree from providing bound actions
     * to all of the views of a document.
     *
     * \param topRange the top range of the tree to remove
     */
    virtual void removeActionsFromDocument(SmartRange* topRange) = 0;

    /**
     * Return a list of SmartRanges which are currently registered as
     * providing bound actions to all of the views of a document.
     */
    virtual const QList<SmartRange*> documentActions() const = 0;

    /**
     * Remove all bound SmartRanges which provide actions to the document.
     */
    virtual void clearDocumentActions() = 0;

    /**
     * Register a SmartRange tree as providing bound actions,
     * and that they should interact with the specified \p view.
     *
     * \param view view on which to use the actions
     * \param topRange the top range of the tree to add
     */
    virtual void addActionsToView(View* view, SmartRange* topRange) = 0;

    /**
     * Remove a SmartRange tree from providing bound actions
     * to the specified \p view.
     *
     * \note implementations should not take into account document-bound
     *       action ranges when calling this function; it is intended solely
     *       to be the counter of addActionsToView()
     *
     * \param view view on which the actions were previously used
     * \param topRange the top range of the tree to remove
     */
    virtual void removeActionsFromView(View* view, SmartRange* topRange) = 0;

    /**
     * Return a list of SmartRanges which are currently registered as
     * providing bound actions to the specified \p view.
     *
     * \note implementations should not take into account document-bound
     *       action ranges when returning the list; it is intended solely
     *       to show actions added via addActionsToView()
     *
     * \param view view to query for the action list
     */
    virtual const QList<SmartRange*> viewActions(View* view) const = 0;

    /**
     * Remove all bound SmartRanges which provide actions to the specified \p view.
     *
     * \param view view from which to remove actions
     */
    virtual void clearViewActions(View* view) = 0;
    //!\}
    // END

  protected:
    /**
     * \internal
     * Used to notify implementations that an Attribute has gained
     * a dynamic component and needs to be included in mouse and/or cursor
     * tracking.
     */
    virtual void attributeDynamic(Attribute* a) = 0;
    /**
     * \internal
     * Used to notify implementations that an Attribute has lost
     * all dynamic components and no longer needs to be included in mouse and cursor
     * tracking.
     */
    virtual void attributeNotDynamic(Attribute* a) = 0;

  private:
    bool m_clearOnDocumentReload;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SmartInterface, "org.kde.KTextEditor.SmartInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
