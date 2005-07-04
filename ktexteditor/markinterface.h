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

#ifndef __ktexteditor_markinterface_h__
#define __ktexteditor_markinterface_h__

#include <qptrlist.h>

#include <kdelibs_export.h>

class QCString;

namespace KTextEditor
{

class Mark
{
  public:
    uint line;
    uint type;
};

/**
*  This is an interface to enable marks to be made in the iconborder of the Document class.
*/
class KTEXTEDITOR_EXPORT MarkInterface
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
    /** 
    * @return a uint representing the marks set in @p line OR'ed togeather. 
    */
    virtual uint mark (uint line) = 0;
    
    /** 
    * Adds a mark of type @p markType to @p line.
    * Has no effect if the line allready contains a mark of that type. 
    */
    virtual void setMark (uint line, uint markType) = 0;
    /**
    * Clears all marks set in @p line.
    */
    virtual void clearMark (uint line) = 0;

    virtual void addMark (uint line, uint markType) = 0;
    /**
    *  Removes any mark of type @p markType from @p line.
    */
    virtual void removeMark (uint line, uint markType) = 0;

    /**
    * @return a list of all marks in the document
    */
    virtual QPtrList<KTextEditor::Mark> marks () = 0;
    /**
    * Clears all marks in the document.
    */ 
    virtual void clearMarks () = 0;

    /**
     * get the number of predefined marker types we have so far.
     * @note If you change this you have to make sure katepart supports the new size!
     * @return number of reserved marker types
     * @since 3.3
     */
    static int reservedMarkersCount();

    /**
     * Pre-defined mark types.
     *
     * To create a non-standard mark type, use MarkInterfaceExtension.
     * To add a new standard mark type, edit this interface to document the type.
     */
    enum MarkTypes
    {
      /** Bookmark */
      markType01= 0x1,
      /** Breakpoint active */
      markType02= 0x2,
      /** Breakpoint reached */
      markType03= 0x4,
      /** Breakpoint disabled */
      markType04= 0x8,
      /** Execution mark */
      markType05= 0x10,
      /** Warning */
      markType06= 0x20,
      /** Error */
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
      markType32= 0x80000000,
      /* reserved marks */
      Bookmark = markType01,
      BreakpointActive = markType02,
      BreakpointReached = markType03,
      BreakpointDisabled = markType04,
      Execution = markType05,
      Warning = markType06,
      Error = markType07
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

KTEXTEDITOR_EXPORT MarkInterface *markInterface (class Document *doc);

}

#endif
