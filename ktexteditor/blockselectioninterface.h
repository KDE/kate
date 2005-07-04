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

#ifndef __ktexteditor_blockselectioninterface_h__
#define __ktexteditor_blockselectioninterface_h__

#include <kdelibs_export.h>

class QCString;

namespace KTextEditor
{

/**
 * An interface for the Document class which allows the selection
 * method to be changed between selecting rectangular blocks of text and normal mode
 * (all text between the start cursor and the current cursor).
 */
class KTEXTEDITOR_EXPORT BlockSelectionInterface
{
  friend class PrivateBlockSelectionInterface;

  public:
    BlockSelectionInterface ();
    virtual ~BlockSelectionInterface ();

    unsigned int blockSelectionInterfaceNumber () const;
    
  protected:  
    void setBlockSelectionInterfaceDCOPSuffix (const QCString &suffix);  

  /**
  *  slots !!!
  */
  public:
    /**
    * Returns the status of the selection mode - true indicates block selection mode is on.
    * If this is true, selections applied via the SelectionInterface are handled as
    * blockselections and the paste functions of the ClipboardInterface works on
    * rectangular blocks of text rather than normal. (copy too, but thats clear I hope ;)
    */
    virtual bool blockSelectionMode () = 0;

    /**
    * Set block selection mode to state "on"
    */
    virtual bool setBlockSelectionMode (bool on) = 0;

    /**
    * toggle block seletion mode
    */
    virtual bool toggleBlockSelectionMode () = 0;

    private:
      class PrivateBlockSelectionInterface *d;
      static unsigned int globalBlockSelectionInterfaceNumber;
      unsigned int myBlockSelectionInterfaceNumber;
};

/**
 * Access the block selection interface of document @param doc
 */
KTEXTEDITOR_EXPORT BlockSelectionInterface *blockSelectionInterface (class Document *doc);

}

#endif
