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

#ifndef __ktexteditor_highlightinginterface_h__
#define __ktexteditor_highlightinginterface_h__

class QString;

namespace KTextEditor
{

/*
*  This is an interface for the KTextEditor::Document class !!!
*/
class HighlightingInterface
{
  friend class PrivateHighlightingInterface;

  public:
    HighlightingInterface ();
    virtual ~HighlightingInterface ();

    unsigned int highlightingInterfaceNumber () const;

  //
	// slots !!!
	//
  public:
	/**
  * returns the current active highlighting mode
  */
	virtual unsigned int hlMode () = 0;

  /**
	* set the current active highlighting mode
	*/
	virtual bool setHlMode (unsigned int mode) = 0;
  
	/**
	* returns the number of available highlightings
	*/
  virtual unsigned int hlModeCount () = 0;
	
	/**
	* returns the name of the highlighting with number "mode"
	*/
	virtual QString hlModeName (unsigned int mode) = 0;
	
	/**
	* returns the sectionname of the highlighting with number "mode"
	*/
  virtual QString hlModeSectionName (unsigned int mode) = 0;

	//
	// signals !!!
	//
	public:
	  virtual void hlChanged () = 0;

  private:
    class PrivateHighlightingInterface *d;
    static unsigned int globalHighlightingInterfaceNumber;
    unsigned int myHighlightingInterfaceNumber;
};

HighlightingInterface *highlightingInterface (class Document *doc);

};

#endif
