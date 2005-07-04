/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
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

#ifndef __ktexteditor_editinterfaceext_h__
#define __ktexteditor_editinterfaceext_h__

#include <qstring.h>

#include <kdelibs_export.h>

namespace KTextEditor
{

/**
* This is the main interface for accessing and modifying
* text of the Document class.
*/
class KTEXTEDITOR_EXPORT EditInterfaceExt
{
  friend class PrivateEditInterfaceExt;

  public:
    EditInterfaceExt();
    virtual ~EditInterfaceExt();

    uint editInterfaceExtNumber() const;

    /**
	 * Begin an editing sequence.  Edit commands during this sequence will be
	 * bunched together such that they represent a single undo command in the
	 * editor, and so that repaint events do not occur inbetween.
	 *
	 * Your application should not return control to the event loop while it
	 * has an unterminated (no matching editEnd() call) editing sequence
	 * (result undefined) - so do all of your work in one go...
	 */
    virtual void editBegin() = 0;

	/**
	 * End and editing sequence.
	 */
    virtual void editEnd() = 0;

  public:
  /**
  * only for the interface itself - REAL PRIVATE
  */
  private:
    class PrivateEditInterfaceExt *d;
    static uint globalEditInterfaceExtNumber;
    uint myEditInterfaceExtNumber;
};

KTEXTEDITOR_EXPORT EditInterfaceExt *editInterfaceExt (class Document *doc);

}

#endif
