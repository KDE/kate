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

#ifndef __ktexteditor_document_h__
#define __ktexteditor_document_h__

// the very important KTextEditor::Cursor class
#include <ktexteditor/cursor.h>
#include <ktexteditor/range.h>

// our main baseclass of the KTextEditor::Document
#include <kparts/part.h>

// the list of views
#include <QList>

namespace KTextEditor
{

class Editor;
class View;

/**
 * The main class representing a text document.
 * This class provides access to the document content, allows
 * modifications and other stuff.
 */
class KTEXTEDITOR_EXPORT Document : public KParts::ReadWritePart
{
  Q_OBJECT

  public:
    /**
     * Document Constructor
     * @param parent parent object
     * @param name name
     */
    Document ( QObject *parent = 0, const char *name = 0 );

    /**
     * virtual destructor
     */
    virtual ~Document ();

    /**
     * Returns the global number of this document in your app.
     * @return document number
     */
    int documentNumber () const;

  /**
   * Stuff to create and manage the views of this document and access the global
   * editor object
   */
  public:
    /**
     * Create a view that will display the document data. You can create as many
     * views as you like. When the user modifies data in one view then all other
     * views will be updated as well.
     * @param parent parent widget
     * @return created KTextEditor::View
     */
    virtual View *createView ( QWidget *parent ) = 0;

    /**
     * Returns a list of all views of this document.
     * @return list of all existing views
     */
    virtual const QList<View*> &views () = 0;

    /**
     * Retrieve the global editor object, the editor part
     * implementation must ensure that this object lives as long
     * as any factory object exists or any document
     * @return global KTextEditor::Editor object
     */
    virtual Editor *editor () = 0;

  signals:
   /**
    * Should be emitted at appropriate times to help applications / plugins to
    * attach to a new view.
    * @attention This signal should be emitted after the view constructor is
    *            completed, e.g. in the createView() method.
    * @param document the document for which a new view is created
    * @param view the new created view
    */
    void viewCreated (Document *document, View *view);

  /**
   * General information about this document and it's content
   */
  public:
    /**
     * Returns this document's name.
     * The editor part should provide some meaningful name, like some unique
     * Untitled XYZ for document without url or basename for documents with
     * url.
     * @return readable document name
     */
    virtual const QString &documentName () const = 0;

    /**
     * Returns this document's mimetype.
     * @return mimetype
     */
    virtual QString mimeType() = 0;

  /**
   * SIGNALS
   * following signals should be emitted by the editor document
   */
  signals:
    /**
     * document name changed
     * @param document document which changed its name
     */
    void documentNameChanged ( Document *document );

    /**
     * document URL changed
     * @param document document which changed its URL
     */
    void documentUrlChanged ( Document *document );

    /**
     * the document's buffer changed from either state @e unmodified to
     * @e modified or vice versa.
     *
     * @see KParts::ReadWritePart::isModified().
     * @see KParts::ReadWritePart::setModified()
     * @param document document which changed its modified state
     */
    void modifiedChanged ( Document *document );

  /**
   * VERY IMPORTANT: Methodes to set and query the current encoding of the
   * document
   */
  public:
    /**
     * Set the encoding for this document. This encoding will be used
     * while loading and saving files, it won't affect the already existing
     * content of the document, e.g. if the file has already be opened without
     * the correct encoding, this won't fix it, you would for example need to
     * trigger reload for this.
     * @param encoding new encoding for the document, name must be accepted
     * by the QTextCodec, if empty encoding name given, the part should fallback
     * to it's own default encoding, e.g. the system encoding or the global user
     * settings
     * @return success, return @e false, if the encoding could not be set
     */
    virtual bool setEncoding (const QString &encoding) = 0;

    /**
     * Returns the current chosen encoding
     * @return current encoding of the document, will return empty string if the
     * document uses the default encoding of the editor and no own special encoding
     */
    virtual const QString &encoding () const = 0;

  /**
   * General file related actions
   * All this actions cause user interaction in some cases
   */
  public:
    /**
     * Reload the current file
     * The user will get prompted by the part on changes and more
     * and can cancel this action if it can harm
     * @return success, has the reload been done? if the document
     * has no url set, it will just return @e false
     */
    virtual bool documentReload () = 0;

    /**
     * save the current file
     * The user will get prompted by the part on, asked for filename if
     * needed and more
     * @return success, has the save been done?
     */
    virtual bool documentSave () = 0;

    /**
     * save the current file with a different name
     * The user will get prompted by the part on, asked for filename
     * and more
     * @return success, has the save as been done?
     */
    virtual bool documentSaveAs () = 0;

 /**
  * Methodes to create/end editing sequences
  */
 public:
    /**
     * Begin an editing sequence.  Edit commands during this sequence will be
     * bunched together such that they represent a single undo command in the
     * editor, and so that repaint events do not occur inbetween.
     *
     * Your application should not return control to the event loop while it
     * has an unterminated (no matching editEnd() call) editing sequence
     * (result undefined) - so do all of your work in one go...
     *
     * This call stacks, like the endEditing calls, this means you can safely
     * call it three times in a row for example if you call editEnd three times, too,
     * it internaly just does counting the running editing sessions.
     *
     * If the texteditor part doesn't support this kind of transactions, both calls
     * just do nothing.
     *
     * @param view here you can optional give a view which does the editing
     *             this can cause the editor part implementation to do some special
     *             cursor handling in this view, important: this only will work
     *             if you pass here a view which parent document is this document,
     *             otherwise, the view is just ignored
     * @return success, parts not supporting it should return @e false
     */
    virtual bool startEditing (View *view = 0) = 0;

    /**
     * End an editing sequence.
     * @return success, parts not supporting it should return @e false
     */
    virtual bool endEditing () = 0;

  /**
   * General access to the document's text content
   */
  public:
    /**
     * retrieve the document content
     * @return the complete document content
     */
    virtual QString text () const = 0;

    /**
     * retrieve part of the document content
     * @param startPosition start position of text to retrieve
     * @param endPosition end position of text to retrieve
     * @return the requested text part, "" for invalid areas
     */
    virtual QString text ( const Cursor &startPosition, const Cursor &endPosition ) const = 0;

    /**
     * retrieve a single text line
     * @param line the wanted line
     * @return the requested line, "" for invalid line numbers
     */
    virtual QString line ( int line ) const = 0;

    /**
     * count of lines in document
     * @return The current number of lines in the document
     */
    virtual int lines () const = 0;

    /**
     * count of characters in document
     * @return the number of characters in the document
     */
    virtual int length () const = 0;

    /**
     * retrieve the length of a given line in characters
     * @param line line to get length from
     * @return the number of characters in the line (-1 if no line "line")
     */
    virtual int lineLength ( int line ) const = 0;

    /**
     * Set the given text as new document content
     * @param text new content for the document
     * @return success
     */
    virtual bool setText ( const QString &text ) = 0;

    /**
     * Removes the whole content of the document
     * @return success
     */
    virtual bool clear () = 0;

    /**
     * Inserts text at given position
     * @param position position to insert the text
     * @param text text to insert
     * @return success
     */
    virtual bool insertText ( const Cursor &position, const QString &text ) = 0;

    /**
     * remove part of the document content
     * @param startPosition start position of text to remove
     * @param endPosition end position of text to remove
     * @return success
     */
    virtual bool removeText ( const Cursor &startPosition, const Cursor &endPosition ) = 0;

    /**
     * checks if a cursor specifies a valid position in a document
     * could be overridden by an implementor, but does not have to
     * @param cursor which should be checked
     * @return check result
     */
    virtual bool cursorInText(const Cursor &cursor);

    /**
     * Insert line(s) at the given line number.
     * Use insertLine(numLines(), text) to append line at end of document
     * @param line line where to insert
     * @param text text which should be inserted
     * @return success
     */
    virtual bool insertLine ( int line, const QString &text ) = 0;

    /**
     * Remove line at the given line number.
     * @param line line to remove
     * @return success
     */
    virtual bool removeLine ( int line ) = 0;

  /**
   * SIGNALS
   * following signals should be emitted by the document
   * if the text content is changed
   */
  signals:
    /**
     * Text changed!
     * @param document document which emited this signal
     */
    void textChanged(Document *document);

    /**
     * Text was inserted at range.start() up to range.end().
     * @param document document which emited this signal
     * @param range range that the newly inserted text occupies
     */
    void textInserted(KTextEditor::Document *document, const KTextEditor::Range& range);

    /**
     * Text was removed from range.start() to range.end().
     * @param document document which emited this signal
     * @param range range that the removed text previously occupied
     */
    void textRemoved(KTextEditor::Document *document, const KTextEditor::Range& range);

    /**
     * Text previously within oldRange was removed and replaced with the text now in newRange.
     * oldRange.start() is guaranteed to equal newRange.start().
     * @param document document which emited this signal
     * @param oldRange range that the text previously occupied
     * @param newRange range that the changed text now occupies
     */
    void textChanged(KTextEditor::Document *document, const KTextEditor::Range& oldRange, const KTextEditor::Range& newRange);

  private:
    /**
     * Private d-pointer
     */
    class PrivateDocument *m_d;

    /**
     * document number
     */
    int m_documentNumber;
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

