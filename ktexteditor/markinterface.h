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

#ifndef __ktexteditor_markinterface_h__
#define __ktexteditor_markinterface_h__

#include <qptrlist.h>

class QCString;

namespace KTextEditor
{

class Mark
{
  public:
    uint line;
    uint type;
};

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class MarkInterface
{
  friend class PrivateMarkInterface;
  
  public:
    MarkInterface ();
    virtual ~MarkInterface ();

    unsigned int markInterfaceNumber () const;
    
  protected:  
    void setMarkInterfaceDCOPSuffix (const QCString &suffix);  

  //
  // slots !!!
  //
  public:
    virtual uint mark (uint line) = 0;

    virtual void setMark (uint line, uint markType) = 0;
    virtual void clearMark (uint line) = 0;

    virtual void addMark (uint line, uint markType) = 0;
    virtual void removeMark (uint line, uint markType) = 0;

    virtual QPtrList<KTextEditor::Mark> marks () = 0;
    virtual void clearMarks () = 0;
    
    enum MarkTypes
    {
      markType01= 0x1, // Bookmark !!!
      markType02= 0x2, // Breakpoint active !!!
      markType03= 0x4, // Breakpoint reached !!!
      markType04= 0x8, // Breakpoint disabled !!!
      markType05= 0x10,// Execution mark !!!
      markType06= 0x20,
      markType07= 0x40,
      markType08= 0x80,
      markType09= 0x100,
      markType10= 0x200,
      markType11= 0x400,
      markType12= 0x800,
      markType13= 0x1000,
      markType14= 0x2000,
      markType15= 0x4000,
      markType16= 0x8000,
      markType17= 0x10000,
      markType18= 0x20000,
      markType19= 0x40000,
      markType20= 0x80000,
      markType21= 0x100000,
      markType22= 0x200000,
      markType23= 0x400000,
      markType24= 0x800000,
      markType25= 0x1000000,
      markType26= 0x2000000,
      markType27= 0x4000000,
      markType28= 0x8000000,
      markType29= 0x10000000,
      markType30= 0x20000000,
      markType31= 0x40000000,
      markType32= 0x80000000
    };

  //
  // signals !!!
  //
  public:
    virtual void marksChanged () = 0;
  
  private:
    class PrivateMarkInterface *d;
    static unsigned int globalMarkInterfaceNumber;
    unsigned int myMarkInterfaceNumber;
};

MarkInterface *markInterface (class Document *doc);

};

#endif
