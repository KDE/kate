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

#ifndef __ktexteditor_viewconfiginterface_h__
#define __ktexteditor_viewconfiginterface_h__

class QObject;
class KConfig;

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class ViewConfigInterface
{
  friend class PrivateViewConfigInterface;

  public:
    ViewConfigInterface();
    virtual ~ViewConfigInterface();

    unsigned int viewConfigInterfaceNumber () const;

  //
  // slots !!!
  //
  public:    
    /**
      Read/Write session config of only this view
    */
    virtual void readSessionConfig (KConfig *) = 0;
    virtual void writeSessionConfig (KConfig *) = 0;

  private:
    class PrivateViewConfigInterface *d;
    static unsigned int globalViewConfigInterfaceNumber;
    unsigned int myViewConfigInterfaceNumber;
};

ViewConfigInterface *viewConfigInterface (QObject *obj);

};

#endif
