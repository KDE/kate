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

#ifndef __ktexteditor_dynwordwrapinterface_h__
#define __ktexteditor_dynwordwrapinterface_h__

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::View class !!!
*/
class DynWordWrapInterface
{
  friend class PrivateDynWordWrapInterface;
  
  public:
    DynWordWrapInterface ();
    virtual ~DynWordWrapInterface ();

    unsigned int dynWordWrapInterfaceNumber () const;

  //
  // slots !!!
  //
  public:
    virtual void setDynWordWrap (bool) = 0;
    virtual bool dynWordWrap () const = 0;

  private:
    class PrivateDynWordWrapInterface *d;
    static unsigned int globalDynWordWrapInterfaceNumber;
    unsigned int myDynWordWrapInterfaceNumber;
};

DynWordWrapInterface *dynWordWrapInterface (class View *view);

};

#endif
