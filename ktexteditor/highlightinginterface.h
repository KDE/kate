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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACE_H
#define KDELIBS_KTEXTEDITOR_HIGHLIGHTINGINTERFACE_H

#include <kdelibs_export.h>

class QString;

namespace KTextEditor
{

/**
 * \brief Highlighting extension interface for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section hliface_intro Introduction
 *
 * The class HighlightingInterface provides only very general methods to get
 * information about syntax highlighting modes. Use hlMode() to get the
 * current highlighting mode and setHlMode() to set it. To get the number of
 * available syntax highlighting modes use hlModeCount(). The name of a mode
 * can be retrieved by using hlModeName(), to get the section to which a mode
 * belongs use hlModeSectionName(). The signal hlChanged() is emitted
 * whenever the syntax highlighting mode changed.
 *
 * \section hliface_access Accessing the HighlightingInterface
 *
 * The HighlightingInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 *   // doc is of type KTextEditor::Document*
 *   KTextEditor::HighlightingInterface *iface =
 *       qobject_cast<KTextEditor::HighlightingInterface*>( doc );
 *
 *   if( iface ) {
 *       // the implementation supports the interface
 *       // do stuff
 *   }
 * \endcode
 *
 * \see KTextEditor::Document
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT HighlightingInterface
{
  public:
    HighlightingInterface (QObject *myself);

    /**
     * Virtual destructor.
     */
    virtual ~HighlightingInterface ();

  /*
   * Access to the highlighting subsystem
   */
  public:
    /**
     * Return the name of the currently used highlighting
     * \return name of the used highlighting
     * 
     */
    virtual QString highlighting() const = 0;
    
    /**
     * Return a list of the names of all possible highlighting
     * \return list of highlighting names
     */
    virtual QStringList highlightings() const = 0;
    
    /**
     * Set the current highlighting of the document by giving it's name
     * \param name name of the highlighting to use for this document
     * \return \e true on success, otherwise \e false
     */
    virtual bool setHighlighting(const QString &name) = 0;

  /*
   * Important signals which are emited from the highlighting system
   */
  public:
    /**
     * Warn anyone listening that the current document's highlighting has
     * changed.
     * 
     * \param document the document which's highlighting has changed
     */
    virtual void highlightingChanged(KTextEditor::Document *document) = 0;

  private:
    class HighlightingInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::HighlightingInterface, "org.kde.KTextEditor.HighlightingInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
