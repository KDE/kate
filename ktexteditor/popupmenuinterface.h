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

#ifndef __ktexteditor_popupmenuinterface_h__
#define __ktexteditor_popupmenuinterface_h__

class QCString;

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::View class !!!
*/
class PopupMenuInterface
{
  friend class PrivatePopupMenuInterface;

  public:
    PopupMenuInterface ();
    virtual ~PopupMenuInterface ();

    unsigned int popupMenuInterfaceNumber () const;
    
  protected:  
    void setPopupMenuInterfaceDCOPSuffix (const QCString &suffix);  

  //
  // normal methodes
  //
  public:
     /**
      Install a Popup Menu. The Popup Menu will be activated on
      a right mouse button press event.
    */
    virtual void installPopup (class QPopupMenu *rmb_Menu) = 0;

  private:
    class PrivatePopupMenuInterface *d;    
    static unsigned int globalPopupMenuInterfaceNumber;
    unsigned int myPopupMenuInterfaceNumber;
};

PopupMenuInterface *popupMenuInterface (class View *view);

};

#endif
