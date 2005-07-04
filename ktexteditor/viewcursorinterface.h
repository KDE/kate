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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __ktexteditor_viewcursorinterface_h__
#define __ktexteditor_viewcursorinterface_h__

#include <kdelibs_export.h>

class QCString;

namespace KTextEditor
{

/**
*  This is an interface to access the text cursor of a View class.
*/
class KTEXTEDITOR_EXPORT ViewCursorInterface
{
  friend class PrivateViewCursorInterface;

  public:
    ViewCursorInterface ();
    virtual ~ViewCursorInterface ();

    unsigned int viewCursorInterfaceNumber () const;
    
  protected:  
    void setViewCursorInterfaceDCOPSuffix (const QCString &suffix); 

  //
  // slots !!!
  //
  public:
    /**
     * Get the current cursor coordinates in pixels.
     */
    virtual class QPoint cursorCoordinates () = 0;

    /**
     * Get the cursor position
     */
    virtual void cursorPosition (unsigned int *line, unsigned int *col) = 0;

    /**
     * Get the cursor position, calculated with 1 character per tab
     */
    virtual void cursorPositionReal (unsigned int *line, unsigned int *col) = 0;

    /**
     * Set the cursor position
     */
    virtual bool setCursorPosition (unsigned int line, unsigned int col) = 0;

    /**
     * Set the cursor position, use 1 character per tab
     */
    virtual bool setCursorPositionReal (unsigned int line, unsigned int col) = 0;

    virtual unsigned int cursorLine () = 0;
    virtual unsigned int cursorColumn () = 0;
    virtual unsigned int cursorColumnReal () = 0;

  //
  // signals !!!
  //
  public:
    virtual void cursorPositionChanged () = 0;
  
  private:
    class PrivateViewCursorInterface *d;
    static unsigned int globalViewCursorInterfaceNumber;
    unsigned int myViewCursorInterfaceNumber;
};

KTEXTEDITOR_EXPORT ViewCursorInterface *viewCursorInterface (class View *view);

}

#endif
