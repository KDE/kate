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

#ifndef __ktexteditor_encodinginterface_h__
#define __ktexteditor_encodinginterface_h__

class QObject;

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class EncodingInterface
{
  friend class PrivateEncodingInterface;

  public:
    EncodingInterface();
    virtual ~EncodingInterface();

    unsigned int encodingInterfaceNumber () const;

  //
  // slots !!!
  //  
  public:
    virtual void setEncoding (const class QString &e) = 0;
    virtual class QString encoding() const = 0;

  private:
    class PrivateEncodingInterface *d;
    static unsigned int globalEncodingInterfaceNumber;
    unsigned int myEncodingInterfaceNumber;
};

EncodingInterface *encodingInterface (QObject *obj);

};

#endif
