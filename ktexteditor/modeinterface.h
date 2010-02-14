/* This file is part of the KDE project
   Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_MODEINTERFACE_H
#define KDELIBS_KTEXTEDITOR_MODEINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>


namespace KTextEditor
{

class Document;

/**
 * \brief Mode information interface for the Document.
 *
 * \ingroup kte_group_doc_extensions
 *
 * \section modeinterface_intro Introduction
 *
 * The ModeInterface provides access to the modes of a document
 *
 * \section modeinterface_access Accessing the ModeInterface
 *
 * The ModeInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // doc is of type KTextEditor::Document*
 * KTextEditor::ModeInterface *iface =
 *     qobject_cast<KTextEditor::ModeInterface*>( doc );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Document
 * \author Joseph Wenninger \<jowenn@kde.org\>
 */
class KTEXTEDITOR_EXPORT ModeInterface
{
  public:    
    /**
     * Constructor.
     */
    ModeInterface();

    /**
     * Virtual destructor.
     */
    virtual ~ModeInterface();
    /**
     * \brief Get all available file modes for the current document.
     *
     * \return Returns a list of all possible modes within a document.
     *
     * \see KTextEditor::Document::mode()
     */
    virtual QStringList allPossibleModes() const = 0;

    /**
     * \brief Get the possible modes for a given position
     *
     * The mode of a certain position might be ambiguos. eg. mode C++ could stand for
     * the source or the header file. In most cases this mode will be equal to the highlighting name
     * but it does not have to be. Modes should not be mixed with HighlightingModes.
     * For instance for the toplevel in a document this might defer. For sub modes it might in the future
     *
     *
     * \see modes()
     *
     * TODO: I intended to make this const but Kate's implementation needs to
     * call kateTextline which is non-const. Solution?
     */
    virtual QString modeAt(const Cursor &position) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::ModeInterface, "org.kde.KTextEditor.ModeInterface")

#endif // KDELIBS_KTEXTEDITOR_MODEINTERFACE_H

// kate: space-indent on; indent-width 2; replace-tabs on;
