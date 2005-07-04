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

#ifndef __ktexteditor_clipboardinterface_h__
#define __ktexteditor_clipboardinterface_h__

#include <kdelibs_export.h>

class QCString;

namespace KTextEditor
{

/**
* This is an interface for accessing the clipboard through the View class.
*/
class KTEXTEDITOR_EXPORT ClipboardInterface
{
  friend class PrivateClipboardInterface;

  public:
    ClipboardInterface();
    virtual ~ClipboardInterface();
    
    unsigned int clipboardInterfaceNumber () const;
    
  protected:  
    void setClipboardInterfaceDCOPSuffix (const QCString &suffix); 
        
  /**
  * slots !!!
  */
  public:
    /**
    * copies selected text
    */
    virtual void copy ( ) const = 0;

    /**
    * copies selected text
    */
    virtual void cut ( ) = 0;

    /**
    * copies selected text
    */
    virtual void paste ( ) = 0;

  private:
    class PrivateClipboardInterface *d;
    static unsigned int globalClipboardInterfaceNumber;
    unsigned int myClipboardInterfaceNumber;
};

KTEXTEDITOR_EXPORT ClipboardInterface *clipboardInterface (class View *view);

}

#endif
