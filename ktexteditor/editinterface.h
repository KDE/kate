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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ktexteditor_editinterface_h__
#define __ktexteditor_editinterface_h__

#include <qstring.h>

namespace KTextEditor
{

/**
*  This is an interface for the KTextEditor::Document class !!!
*/
class EditInterface
{
  friend class PrivateEditInterface;

  public:
    EditInterface();
    virtual ~EditInterface();

    unsigned int editInterfaceNumber () const;

  public:
  /**
  * slots !!!
  */
    /**
    * @return the complete document as a single QString
    */
    virtual QString text () const = 0;

    /**
    * @return a QString
    */
    virtual QString text ( unsigned int startLine, unsigned int startCol, unsigned int endLine, unsigned int endCol ) const = 0;

    /**
    * @return All the text from the requested line.
    */
    virtual QString textLine ( unsigned int line ) const = 0;

    /**
    * @return The current number of lines in the document
    */
    virtual unsigned int numLines () const = 0;

    /**
    * @return the number of characters in the document
    */
    virtual unsigned int length () const = 0;

    /**
    * @return the number of characters in the line (-1 if no line "line")
    */
    virtual int lineLength ( unsigned int line ) const = 0;

    /**
    * Set the given text into the view.
    * Warning: This will overwrite any data currently held in this view.
    */
    virtual bool setText ( const QString &text ) = 0;

    /**
    *  clears the document
    * Warning: This will overwrite any data currently held in this view.
    */
    virtual bool clear () = 0;

    /**
    *  Inserts text at line "line", column "col"
    *  returns true if success
    */
    virtual bool insertText ( unsigned int line, unsigned int col, const QString &text ) = 0;

    /**
    *  remove text at line "line", column "col"
    *  returns true if success
    */
    virtual bool removeText ( unsigned int startLine, unsigned int startCol, unsigned int endLine, unsigned int endCol ) = 0;

    /**
    * Insert line(s) at the given line number. If the line number is -1
    * (the default) then the line is added to end of the document
    */
    virtual bool insertLine ( unsigned int line, const QString &text ) = 0;

    /**
    * Insert line(s) at the given line number. If the line number is -1
    * (the default) then the line is added to end of the document
    */
    virtual bool removeLine ( unsigned int line ) = 0;

  /**
  * signals !!!
  */
  public:
    virtual void textChanged () = 0;

    virtual void charactersInteractivelyInserted(int ,int ,const QString&)=0; //line, col, characters if you don't support this, don't create a signal, just overload it.

  /**
  * only for the interface itself - REAL PRIVATE
  */
  private:
    class PrivateEditInterface *d;
    static unsigned int globalEditInterfaceNumber;
    unsigned int myEditInterfaceNumber;
};

EditInterface *editInterface (class Document *doc);

};

#endif
