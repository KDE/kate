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

#ifndef __ktexteditor_blockselectioninterface_h__
#define __ktexteditor_blockselectioninterface_h__

#include <qstring.h>

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class BlockSelectionInterface
{
  friend class PrivateBlockSelectionInterface;

  public:
    BlockSelectionInterface ();
    virtual ~BlockSelectionInterface ();

    unsigned int blockSelectionInterfaceNumber () const;
    
  protected:  
    void setBlockSelectionInterfaceDCOPSuffix (const QCString &suffix);  

  /*
  *  slots !!!
  */
  public:
    /**
    * is blockselection mode on ?
    * if the blockselection mode is on, the selections
    * applied via the SelectionInterface are handled as
    * blockselections and the paste functions of the
    * ClipboardInterface works blockwise (copy too, but
    * thats clear I hope ;)
    */
    virtual bool blockSelectionMode () = 0;

    /**
    * set blockselection mode to state "on"
    */
    virtual bool setBlockSelectionMode (bool on) = 0;

    /**
    * toggle blockseletion mode
    */
    virtual bool toggleBlockSelectionMode () = 0;

    private:
      class PrivateBlockSelectionInterface *d;
      static unsigned int globalBlockSelectionInterfaceNumber;
      unsigned int myBlockSelectionInterfaceNumber;
};

BlockSelectionInterface *blockSelectionInterface (class Document *doc);

};

#endif
