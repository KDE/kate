/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ktexteditor_selectioninterface_h__
#define __ktexteditor_selectioninterface_h__

#include <qstring.h>

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class SelectionInterface
{
  friend class PrivateSelectionInterface;

  public:
    SelectionInterface();
    virtual ~SelectionInterface();
        
    unsigned int selectionInterfaceNumber () const;
    
  protected:  
    void setSelectionInterfaceDCOPSuffix (const QCString &suffix);  
    
  /*
  *  slots !!!
  */
  public:
    /**
    *  @return set the selection from line_start,col_start to line_end,col_end
    */
    virtual bool setSelection ( unsigned int startLine, unsigned int startCol, unsigned int endLine, unsigned int endCol ) = 0;

    /**
    *  removes the current Selection (not Text)
    */
    virtual bool clearSelection () = 0;

    /**
    *  @return true if there is a selection
    */
    virtual bool hasSelection () const = 0;

    /**
    *  @return a QString for the selected text
    */
    virtual QString selection () const = 0;

    /**
    *  removes the selected Text
    */
    virtual bool removeSelectedText () = 0;

    /**
    *  select the whole text
    */
    virtual bool selectAll () = 0;

	//
	// signals !!!
	//
	public:
	  virtual void selectionChanged () = 0;
  
  private:
    class PrivateSelectionInterface *d;
    static unsigned int globalSelectionInterfaceNumber;
    unsigned int mySelectionInterfaceNumber;
};

class Document;
class View;

SelectionInterface *selectionInterface (Document *doc);
SelectionInterface *selectionInterface (View *view);

};

#endif
