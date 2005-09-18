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

#ifndef KDELIBS_KTEXTEDITOR_DOCUMENT_H
#define KDELIBS_KTEXTEDITOR_DOCUMENT_H

// the very important KTextEditor::Cursor class
#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>
#include <kdocument/document.h>

// our main baseclass of the KTextEditor::Document
#include <kparts/part.h>

// the list of views
#include <QList>

namespace KTextEditor
{

class Editor;
class View;

/**
 * A KParts derived class representing a text document.
 *
 * <b>Introduction</b>\n
 *
 * The Document class represents a pure text document providing methods to
 * modify the content and create views. A document can have any number
 * of views, each view representing the same content, i.e. all views are
 * synchronized. Support for text selection is handeled by a View and text
 * format attribues by the Attribute interface.
 *
 * To load a document call KParts::ReadOnlyPart::openURL().
 * To reload a document from a file call documentReload(), to save the
 * document call documentSave() or documentSaveAs(). Whenever the modified
 * state of the document changes the signal modifiedChanged() is emitted.
 * Check the modified state with KParts::ReadWritePart::isModified().
 * Further signals are documentUrlChanged(). The encoding can be specified
 * with setEncoding(), however this will only take effect on file reload and
 * file save.
 *
 * Every document has a application wide unique document number, accessible
 * with documentNumber().
 *
 * <b>Text Manipulation</b>\n
 *
 * Get the whole content with text() and set new content with setText().
 * Call insertText() or insertLine() to insert new text or removeText()
 * and removeLine() to remove content. Whenever the document's content
 * changed the signal textChanged() is emitted. Additional signals are
 * textInserted() and textRemoved().
 *
 * If the editor part supports it a document provides full undo/redo history.
 * Text manipulation actions can be grouped together using startEditing()
 * and endEditing(). All actions inbetween are grouped together to only one
 * undo/redo action. Due to internal reference counting you can call
 * startEditing() and endEditing() as often as you wish, but make sure you
 * call endEditing() exactly as often as you call startEditing(), otherwise
 * the reference counter gets confused.
 *
 * <b>Document Views</b>\n
 *
 * A View displays the document's content. As already mentioned a document
 * can have any number of views, all synchronized. Get a list of all views
 * with views(). Only one of the views can be active (i.e. has focus), get
 * it by using activeView(). Create a new view with createView(). Everytime
 * a new view is created the signal viewCreated() is emitted.
 *
 * @see KTextEditor::Editor, KTextEditor::View, KTextEditor::Attribute,
 *      KParts::ReadWritePart
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Document : public KDocument::Document
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create a new document with @p parent.
     * @param parent parent object
     * @see Editor::createDocument()
     */
    Document ( QObject *parent = 0);

    /**
     * Virtual destructor.
     */
    virtual ~Document ();

  /*
   * Methods to create and manage the views of this document and access the
   * global editor object.
   */
  public:
    /**
     * Get the global editor object. The editor part implementation must
     * ensure that this object exists as long as any factory or document
     * object exists.
     * @return global KTextEditor::Editor object
     * @see KTextEditor::Editor
     */
    virtual Editor *editor () = 0;

    /**
     * Return the view which is currently has user focus, if any.
     */
    virtual View* activeView() const = 0;

  signals:
   /**
    * This signal is emitted whenever the @p document creates a new @p view.
    * It should be called for every view to help applications / plugins to
    * attach to the @p view.
    * @attention This signal should be emitted after the view constructor is
    *            completed, e.g. in the createView() method.
    * @param document the document for which a new view is created
    * @param view the new view
    * @see createView()
    */
    void viewCreated (KTextEditor::Document *document, KTextEditor::View *view);

  /*
   * General information about this document and its content.
   */
  public:
    /**
     * Get this document's name.
     * The editor part should provide some meaningful name, like some unique
     * "Untitled XYZ" for the document - @e without URL or basename for
     * documents with url.
     * @return readable document name
     */
    virtual const QString &documentName () const = 0;

    /**
     * Get this document's mimetype.
     * @return mimetype
     */
    virtual QString mimeType() = 0;

  /*
   * SIGNALS
   * following signals should be emitted by the editor document.
   */
  signals:
    /**
     * This signal is emitted whenever the @p document name changes.
     * @param document document which changed its name
     * @see documentName()
     */
    void documentNameChanged ( KTextEditor::Document *document );

    /**
     * This signal is emitted whenever the @p document URL changes.
     * @param document document which changed its URL
     * @see KParts::ReadOnlyPart::url()
     */
    void documentUrlChanged ( KTextEditor::Document *document );

    /**
     * This signal is emitted whenever the @p document's buffer changed from
     * either state @e unmodified to @e modified or vice versa.
     *
     * @param document document which changed its modified state
     * @see KParts::ReadWritePart::isModified().
     * @see KParts::ReadWritePart::setModified()
     */
    void modifiedChanged ( KTextEditor::Document *document );

  /*
   * VERY IMPORTANT: Methods to set and query the current encoding of the
   * document
   */
  public:
    /**
     * Set the encoding for this document. This encoding will be used
     * while loading and saving files, it will @e not affect the already
     * existing content of the document, e.g. if the file has already been
     * opened without the correct encoding, this will @e not fix it, you
     * would for example need to trigger a reload for this.
     * @param encoding new encoding for the document, the name must be
     *        accepted by QTextCodec, if an empty encoding name is given, the
     *        part should fallback to its own default encoding, e.g. the
     *        system encoding or the global user settings
     * @return @e true on success, or @e false, if the encoding could not be set.
     * @see encoding()
     */
    virtual bool setEncoding (const QString &encoding) = 0;

    /**
     * Get the current chosen encoding. The return value is an empty string,
     * if the document uses the default encoding of the editor and no own
     * special encoding.
     * @return current encoding of the document
     * @see setEncoding()
     */
    virtual const QString &encoding () const = 0;

  /*
   * General file related actions.
   * All this actions cause user interaction in some cases.
   */
  public:
    /**
     * Reload the current file.
     * The user will be prompted by the part on changes and more and can
     * cancel this action if it can harm.
     * @return @e true if the reload has been done, otherwise @e false. If
     *         the document has no url set, it will just return @e false.
     */
    virtual bool documentReload () = 0;

    /**
     * Save the current file.
     * The user will be asked for a filename if needed and more.
     * @return @e true on success, i.e. the save has been done, otherwise
     *         @e false
     */
    virtual bool documentSave () = 0;

    /**
     * Save the current file to another location.
     * The user will be asked for a filename and more.
     * @return @e true on success, i.e. the save has been done, otherwise
     *         @e false
     */
    virtual bool documentSaveAs () = 0;

 /*
  * Methodes to create/end editing sequences.
  */
 public:
    /**
     * Begin an editing sequence.
     * Edit commands during this sequence will be bunched together so that
     * they represent a single undo command in the editor, and so that
     * repaint events do not occur inbetween.
     *
     * Your application should @e not return control to the event loop while
     * it has an unterminated (i.e. no matching endEditing() call) editing
     * sequence (result undefined) - so do all of your work in one go!
     *
     * This call stacks, like the endEditing() calls, this means you can
     * safely call it three times in a row for example if you call
     * endEditing() three times, too, it internaly just does counting the
     * running editing sessions.
     *
     * If the texteditor part does not support this kind of transactions,
     * both calls just do nothing.
     *
     * @param view here you can optional give a view which does the editing
     *        this can cause the editor part implementation to do some
     *        special cursor handling in this view. Important: this only will
     *        work if you pass here a view which parent document is this
     *        document, otherwise the view is just ignored.
     * @return @e true on success, otherwise @e false. Parts not supporting
     *         it should return @e false
     * @see endEditing()
     */
    virtual bool startEditing (View *view = 0) = 0;

    /**
     * End an editing sequence.
     * @return @e true on success, otherwise @e false. Parts not supporting
     *         it should return @e false.
     * @see startEditing() for more details
     */
    virtual bool endEditing () = 0;

  /*
   * General access to the document's text content.
   */
  public:
    /**
     * Get the document content.
     * @return the complete document content
     * @see setText()
     */
    virtual QString text () const = 0;

    /**
     * Get the document content within the given @p range.
     * @param range the range of text to retrieve
     * @param block Set this to @e true to receive text in a visual block,
     *        rather than everything inside @p range.
     * @return the requested text part, or QString() for invalid ranges.
     * @see setText()
     */
    virtual QString text ( const Range& range, bool block = false ) const = 0;

    /**
     * Get a single text line.
     * @param line the wanted line
     * @return the requested line, or "" for invalid line numbers
     * @see text(), lineLength()
     */
    virtual QString line ( int line ) const = 0;

    /**
     * Get the count of lines of the document.
     * @return the current number of lines in the document
     * @see length()
     */
    virtual int lines () const = 0;

    /**
     * End position of the document.
     * @return The last column on the last line of the document
     * @see all()
     */
    virtual Cursor end() const = 0;

    /**
     * A Range which encompasses the whole document.
     * @return A range from the start to the end of the document
     */
    inline Range all() const { return Range(Cursor(), end()); }

    /**
     * Get the count of characters in the document. A TAB character counts as
     * only one character.
     * @return the number of characters in the document
     * @see lines()
     */
    virtual int length () const = 0;

    /**
     * Get the length of a given line in characters.
     * @param line line to get length from
     * @return the number of characters in the line or -1 if the line was
     *         invalid
     * @see line()
     */
    virtual int lineLength ( int line ) const = 0;

    /**
     * Get the end cursor position of line @p line.
     * @param line line
     * @see lineLength(), line()
     */
    inline Cursor endOfLine(int line) const { return Cursor(line, lineLength(line)); }

    /**
     * Set the given text as new document content.
     * @param text new content for the document
     * @return @e true on success, otherwise @e false
     * @see text()
     */
    virtual bool setText ( const QString &text ) = 0;

    /**
     * Remove the whole content of the document.
     * @return @e true on success, otherwise @e false
     * @see removeText(), removeLine()
     */
    virtual bool clear () = 0;

    /**
     * Insert @p text at @p position.
     * @param position position to insert the text
     * @param text text to insert
     * @param block insert this text as a visual block of text rather than a linear sequence
     * @return @e true on success, otherwise @e false
     * @see setText(), removeText()
     */
    virtual bool insertText ( const Cursor &position, const QString &text, bool block = false ) = 0;

    /**
     * Remove the text specified in @p range.
     * @param range range of text to remove
     * @param block set this to true to remove a text block on the basis of columns, rather than everything inside \p range
     * @return @e true on success, otherwise @e false
     * @see setText(), insertText()
     */
    virtual bool removeText ( const Range &range, bool block = false ) = 0;

    /**
     * Checks whether the @p cursor specifies a valid position in a document.
     * It can optionally be overridden by an implementation.
     * @param cursor which should be checked
     * @return @e true, if the cursor is valid, otherwise @e false
     * @see SmartCursor::isValid()
     */
    virtual bool cursorInText(const Cursor &cursor);

    /**
     * Insert line(s) at the given line number. The newline character '\\n'
     * is treated as line delimiter, so it is possible to insert multiple
     * lines. To append lines at the end of the document, use
     * @code
     *   insertLine( numLines(), text )
     * @endcode
     * @param line line where to insert the text
     * @param text text which should be inserted
     * @return @e true on success, otherwise @e false
     * @see insertText()
     */
    virtual bool insertLine ( int line, const QString &text ) = 0;

    /**
     * Remove @p line from the document.
     * @param line line to remove
     * @return @e true on success, otherwise @e false
     * @see removeText(), clear()
     */
    virtual bool removeLine ( int line ) = 0;

  /*
   * SIGNALS
   * Following signals should be emitted by the document if the text content
   * is changed.
   */
  signals:
    /**
     * The @p document emits this signal whenever its text changes.
     * @param document document which emitted this signal
     * @see text(), textLine()
     */
    void textChanged(KTextEditor::Document *document);

    /**
     * The @p document emits this signal whenever text was inserted.  The
     * insertion occurred at range.start(), and new text now occupies up to
     * range.end().
     * @param document document which emitted this signal
     * @param range range that the newly inserted text occupies
     * @see insertText(), insertLine()
     */
    void textInserted(KTextEditor::Document *document, const KTextEditor::Range& range);

    /**
     * The @p document emits this signal whenever @p range was removed, i.e.
     * text was removed.
     * @param document document which emitted this signal
     * @param range range that the removed text previously occupied
     * @see removeText(), removeLine(), clear()
     */
    void textRemoved(KTextEditor::Document *document, const KTextEditor::Range& range);

    /**
     * The @p document emits this signal whenever the text in range
     * @p oldRange was removed and replaced with the text now in @e newRange,
     * e.g. the user selects text and pastes new text to replace the selection.
     * @note @p oldRange.start() is guaranteed to equal @p newRange.start().
     * @param document document which emitted this signal
     * @param oldRange range that the text previously occupied
     * @param newRange range that the changed text now occupies
     * @see insertText(), insertLine(), removeText(), removeLine(), clear()
     */
    void textChanged(KTextEditor::Document *document, const KTextEditor::Range& oldRange, const KTextEditor::Range& newRange);
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

