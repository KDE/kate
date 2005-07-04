/* This file is part of the KDE project
   Copyright (C) 2002 Anders Lund <anders@alweb.dk>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
   
   $Id$
*/

#ifndef __ktexteditor_selectioninterface_ext_h__
#define __ktexteditor_selectioninterface_ext_h__

#include <kdelibs_export.h>

class QCString;

namespace KTextEditor
{

/**
    Provide access to seleciton positions.
    This is an interface for the Document class.
    Mainly here for dcop and the sake of scripting.
    @author Anders Lund <anders@alweb.dk>
*/
class KTEXTEDITOR_EXPORT SelectionInterfaceExt
{
  friend class PrivateSelectionInterfaceExt;

  public:
    SelectionInterfaceExt();
    virtual ~SelectionInterfaceExt();
        
    unsigned int selectionInterfaceExtNumber () const;
    
  protected:  
    void setSelectionInterfaceExtDCOPSuffix (const QCString &suffix);  
    
  public:
    /** The selection start line number */
    virtual int selStartLine()=0;
    /** The selection start col */
    virtual int selStartCol()=0;
    /** The selection end line */
    virtual int selEndLine()=0;
    /** The selection end col */
    virtual int selEndCol()=0;
  
  private:
    class PrivateSelectionInterfaceExt *d;
    static unsigned int globalSelectionInterfaceExtNumber;
    unsigned int mySelectionInterfaceExtNumber;
};

class Document;
class View;

KTEXTEDITOR_EXPORT SelectionInterfaceExt *selectionInterfaceExt (Document *doc);
KTEXTEDITOR_EXPORT SelectionInterfaceExt *selectionInterfaceExt (View *view);

}  // namespace KTextEditor
#endif
