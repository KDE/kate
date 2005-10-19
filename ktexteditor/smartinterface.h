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
 * \short A Document extension interface for handling SmartCursors and SmartRanges.
 * \ingroup kte_group_doc_extensions
 *
 * Use this interface to:
 * \li create new SmartCursors and SmartRanges;
 * \li use them to create arbitrary highlighting; and
 * \li use them to associate KActions to ranges of text
 *
 * \section Creation of SmartCursors and SmartRanges
 * These functions must be used to create SmartCursors and SmartRanges.  This
 * means that these objects cannot be derived from by third party applications.
 *
 * You then own these objects; upon deletion they de-register themselves from
 * the Document with which they were associated.  Alternatively, they are all
 * deleted with the deletion of the owning Document.
 *
 * \section Arbitrary Highlighting Interface
 * Arbitrary highlighting of text can be achieved by creating SmartRanges in a
 * tree structure, and assigning appropriate Attributes to these ranges.
 *
 * To highlight all views, use addHighlightToDocument(); to highlight one or some
 * of the views, use addHighlightToView().  You only need to call this function once
 * per tree; just supply the top range you want to have highlighted.  Calling
 * this function more than once with ranges from the same tree may give undefined results.
 *
 * \section Action Binding Interface
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
 * \todo add clearCursors() and clearRanges() functions to delete all cursors + ranges associated with a specific document
 * \author Hamish Rodda \<rodda@kde.org\>
 */
class KTEXTEDITOR_EXPORT SmartInterface
{
  friend class Attribute;

  public:
    virtual ~SmartInterface();

    // BEGIN New cursor methods
    /**
     * Creates a new SmartCursor.
     * \param position The initial cursor position assumed by the new cursor.
     * \param moveOnInsert Define whether the cursor should move when text is inserted at the cursor position.
     */
    virtual SmartCursor* newSmartCursor(const Cursor& position = Cursor::invalid(), bool moveOnInsert = true) = 0;
    /// \overload
    inline SmartCursor* newSmartCursor(bool moveOnInsert = true)
      { return newSmartCursor(Cursor(), moveOnInsert); }
    /// \overload
    inline SmartCursor* newSmartCursor(int line, int column, bool moveOnInsert = true)
      { return newSmartCursor(Cursor(line, column), moveOnInsert); }
    // END

    // BEGIN New range methods
    /**
     * Creates a new SmartRange.
     * \param range The initial text range assumed by the new range.
     * \param parent The parent SmartRange, if this is to be the child of an existing range.
     * \param insertBehaviour Define whether the range should expand when text is inserted at ends of the range.
     */
    virtual SmartRange* newSmartRange(const Range& range = Range::invalid(), SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand) = 0;
    /// \overload
    inline SmartRange* newSmartRange(const Cursor& startPosition, const Cursor& endPosition, SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand)
      { return newSmartRange(Range(startPosition, endPosition), parent, insertBehaviour); }
    /// \overload
    inline SmartRange* newSmartRange(int startLine, int startColumn, int endLine, int endColumn, SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand)
      { return newSmartRange(Range(startLine, startColumn, endLine, endColumn), parent, insertBehaviour); }

    /**
     * Creates a new SmartRange from pre-existing SmartCursors.  The cursors must not be part of any other range.
     * \param start Start SmartCursor
     * \param end End SmartCursor
     * \param parent The parent SmartRange, if this is to be the child of an existing range.
     * \param insertBehaviour Define whether the range should expand when text is inserted at ends of the range.
     */
    virtual SmartRange* newSmartRange(SmartCursor* start, SmartCursor* end, SmartRange* parent = 0L, SmartRange::InsertBehaviours insertBehaviour = SmartRange::DoNotExpand) = 0;
    // END

    // BEGIN Syntax highlighting extension
    virtual void addHighlightToDocument(SmartRange* topRange) = 0;
    virtual void removeHighlightFromDocument(SmartRange* topRange) = 0;
    virtual const QList<SmartRange*>& documentHighlights() const = 0;

    virtual void addHighlightToView(View* view, SmartRange* topRange) = 0;
    virtual void removeHighlightFromView(View* view, SmartRange* topRange) = 0;
    virtual const QList<SmartRange*>& viewHighlights(View* view) const = 0;
    // END

    // BEGIN Action binding extension
    virtual void addActionsToDocument(SmartRange* topRange) = 0;
    virtual void removeActionsFromDocument(SmartRange* topRange) = 0;
    virtual const QList<SmartRange*>& documentActions() const = 0;

    virtual void addActionsToView(View* view, SmartRange* topRange) = 0;
    virtual void removeActionsFromView(View* view, SmartRange* topRange) = 0;
    virtual const QList<SmartRange*>& viewActions(View* view) const = 0;
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
};

}

Q_DECLARE_INTERFACE(KTextEditor::SmartInterface, "org.kde.KTextEditor.SmartInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
