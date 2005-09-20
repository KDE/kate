/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann (cullmann@kde.org)

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

#ifndef KDELIBS_KTEXTEDITOR_MODIFICATIONINTERFACE_H
#define KDELIBS_KTEXTEDITOR_MODIFICATIONINTERFACE_H

#include <kdelibs_export.h>

namespace KTextEditor
{

class Document;
class View;

/**
 * External modification extension interface for the Document.
 *
 * <b>Introduction</b>\n
 *
 * The class ModificationInterface provides methods to handle modifications
 * of all opened files caused by external programs. Whenever the
 * modified-on-disk state changes the signal modifiedOnDisk() is emitted
 * along with a ModifiedOnDiskReason. Set the state by calling
 * setModifiedOnDisk(). Whether the Editor should show warning dialogs to
 * inform the user about external modified files can be controlled with
 * setModifiedOnDiskWarning(). The slot modifiedOnDisk() is called to ask
 * the user what to do whenever a file was modified.
 *
 * <b>Accessing the ModificationInterface</b>\n
 *
 * The ModificationInterface is supposed to be an extension interface for a
 * Document, i.e. the Document inherits the MarkInterface @e provided that
 * the used KTextEditor library implements the interface. To access the
 * ModificationInterface do the following:
 * @code
 *   // doc is of type KTextEditor::Document*
 *   KTextEditor::ModificationInterface *modificationInterface =
 *       qobject_cast<KTextEditor::ModificationInterface*>( doc );
 *
 *   if( modificationInterface ) {
 *       // the implementation supports the ModificationInterface
 *       // do stuff
 *   }
 *   else {
 *       // the implementation does not support the ModificationInterface
 *   }
 * @endcode
 *
 * @see KTextEditor::Document
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT ModificationInterface
{
  public:
    /**
     * virtual destructor
     */
    virtual ~ModificationInterface () {}

  public:
    /**
     * Reasons why a document is modified on disk.
     */
    enum ModifiedOnDiskReason {
      OnDiskUnmodified = 0, ///< Not modified
      OnDiskModified = 1,   ///< The file was modified by another program
      OnDiskCreated = 2,    ///< The file was created by another program
      OnDiskDeleted = 3     ///< The file was deleted
    };

  public:
    /**
     * For client apps that want to deal with files modified on disk, it is
     * nessecary to reset this property.
     * @p reason is a ModifiedOnDiskReason.
     */
    virtual void setModifiedOnDisk( ModifiedOnDiskReason reason ) = 0;

   /**
    * Turn on/off any warning dialogs about the modification for this editor
    * @param on should the editor show dialogs?
    */
   virtual void setModifiedOnDiskWarning ( bool on ) = 0;

  /**
   * These stuff is implemented as slots in the real document
   */
  public:
    /**
     * Ask the user what to do, if the file is modified on disk.
     * The @p v argument is used to avoid asking again, when the
     * editor regains focus after the dialog is hidden.
     */
    virtual void slotModifiedOnDisk( View *view = 0 ) = 0;

  /**
   * These stuff is implemented as signals in the real document
   */
  public:
    /**
     * Indicate this file is modified on disk
     * @param doc the KTextEditor::Document object that represents the file on disk
     * @param isModified indicates the file was modified rather than created or deleted
     * @param reason the reason we are emitting the signal.
     */
    virtual void modifiedOnDisk (KTextEditor::Document *doc, bool isModified,
                                 KTextEditor::ModificationInterface::ModifiedOnDiskReason reason) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::ModificationInterface, "org.kde.KTextEditor.ModificationInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

