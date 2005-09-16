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

#include <kdelibs_export.h>

#include <qhash.h>
#include <qobject.h>

class QPixmap;

namespace KTextEditor
{

class Document;

class Mark
{
  public:
    int line;
    uint type;
};

/**
 * The MarkInterface provides the possibility to enable and disable marks in
 * the iconborder of a @p Document. There are several pre-defined mark types
 * but it is possible to add custom marks and set custom pixmaps.
 */
class KTEXTEDITOR_EXPORT MarkInterface
{
  public:
    /**
     * virtual destructor
     */
    virtual ~MarkInterface () {}

  //
  // slots !!!
  //
  public:
    /**
     * Get all marks set on the @e line.
     * @return a @e uint representing of the marks set in @e line concatenated
     *         by logical OR 
     */
    virtual uint mark (int line) = 0;
    
    /** 
     * Set a mark of type @e markType to @e line.
     * If @e line already contains a mark of that type it has no effect. 
     * All other marks are deleted before the mark is set.
     * @param line line to set the mark
     * @param markType mark type
     */
    virtual void setMark (int line, uint markType) = 0;
    /**
     * Clear all marks set in @e line.
     */
    virtual void clearMark (int line) = 0;

    /**
     * Add mark of type @e markType to @e line. Existing marks on this line
     * are preserved. If the mark @e markType already is set, nothing happens.
     * @param line line to set the mark
     * @param markType mark type
     */
    virtual void addMark (int line, uint markType) = 0;
    /**
     * Removes a mark of type @e markType from @e line.
     * @param line line to remove the mark
     * @param markType mark type to be removed
     */
    virtual void removeMark (int line, uint markType) = 0;

    /**
     * Get a list of all marks in the document.
     * @return a list of all marks in the document
     */
    virtual const QHash<int, KTextEditor::Mark*> &marks () = 0;

    /**
     * Clear all marks in the document.
     */ 
    virtual void clearMarks () = 0;

    /**
     * Get the number of pre-defined marker types we have so far.
     * @note FIXME: If you change this you have to make sure katepart supports
     *              the new size!
     * @return number of reserved marker types
     */
    static int reservedMarkersCount() { return 7; }

    /**
     * Pre-defined mark types.
     *
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
    /**
     * The @e document emits this signal whenever a mark changed.
     * @param document document which emitted this signal
     */
    virtual void marksChanged (Document* document) = 0;

    /**
     * Methods to modify mark properties.
     */
  public:
    /**
     * Set the @e mark's pixmap to @e pixmap.
     * @param mark mark to change
     * @param pixmap new pixmap
     */
    virtual void setMarkPixmap( MarkTypes mark, const QPixmap &pixmap ) = 0;
    /**
     * Set the @e mark's description to @e text.
     * @param mark mark to set the description
     * @param text new descriptive text
     */
    virtual void setMarkDescription( MarkTypes mark, const QString &text ) = 0;
    /**
     * Set the marks user changable pattern to @e markMask, i.e. concatenate
     * all changable marks with a logical OR.
     * @param markMask bitmap pattern
     */
    virtual void setMarksUserChangable( uint markMask ) = 0;

    /**
     * Possible actions on a mark.
     */
    enum MarkChangeAction {
		MarkAdded=0,    /**< action: a mark was added.  */
		MarkRemoved=1   /**< action: a mark was removed. */
	};

  //
  // signals !!!
  //
  public:
    /**
     * The @e document emits this signal whenever the @e mark changes.
     * @param document the document which emitted the signal
     * @param mark changed makr
     * @param action action, either removed or added
     */
    virtual void markChanged ( Document* document, KTextEditor::Mark mark, 
                              KTextEditor::MarkInterface::MarkChangeAction action) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::MarkInterface, "org.kde.KTextEditor.MarkInterface")

#endif
