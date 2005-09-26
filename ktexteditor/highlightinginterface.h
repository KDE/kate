/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

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

#ifndef KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACE_H
#define KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACE_H

#include <kdelibs_export.h>

class QString;

namespace KTextEditor
{

/**
 * Highlighting extension interface for the Document.
 *
 * <b>Introduction</b>\n
 *
 * The class HighlightingInterface provides only very general methods to get
 * information about syntax highlighting modes. Use hlMode() to get the
 * current highlighting mode and setHlMode() to set it. To get the number of
 * available syntax highlighting modes use hlModeCount(). The name of a mode
 * can be retrieved by using hlModeName(), to get the section to which a mode
 * belongs use hlModeSectionName(). The signal hlChanged() is emitted
 * whenever the syntax highlighting mode changed.
 *
 * <b>Accessing the HighlightingInterface</b>\n
 *
 * The HighlightingInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface @e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * @code
 *   // doc is of type KTextEditor::Document*
 *   KTextEditor::HighlightingInterface *iface =
 *       qobject_cast<KTextEditor::HighlightingInterface*>( doc );
 *
 *   if( iface ) {
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * @endcode
 *
 * @see KTextEditor::Document
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT HighlightingInterface
{
  public:
    /**
     * Virtual destructor.
     */
    virtual ~HighlightingInterface () {}

  //
  // SLOTS !!!
  //
  public:
    /**
     * Get the current active syntax highlighting mode.
     * @return current active mode
     * @see setHlMode()
     */
    virtual unsigned int hlMode () = 0;

    /**
     * Set the syntax highlighting mode to @p mode.
     * @param mode new highlighting mode
     * @see hlMode()
     */
    virtual bool setHlMode (unsigned int mode) = 0;

    /**
     * Get the number of available syntax highlighting modes.
     * @return number of available modes
     */
    virtual unsigned int hlModeCount () = 0;

    /**
     * Get the name of the syntax highlighting mode with number @p mode.
     * @return name of @p mode
     * @see hlModeSectionName()
     */
    virtual QString hlModeName (unsigned int mode) = 0;

    /**
     * Get the name of the section to which the syntax highlighting mode
     * with number @p mode belongs to.
     * @return section name
     * @see hlModeName()
     */
    virtual QString hlModeSectionName (unsigned int mode) = 0;

  //
  // SIGNALS !!!
  //
  public:
    /**
     * This signal is emitted whenever the document's syntax highlighting
     * mode changed.
     * @see hlMode(), setHlMode()
     */
    virtual void hlChanged () = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::HighlightingInterface, "org.kde.KTextEditor.HighlightingInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
