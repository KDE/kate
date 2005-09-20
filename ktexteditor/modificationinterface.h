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
 * Document, i.e. the Document inherits the interface @e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * @code
 *   // doc is of type KTextEditor::Document*
 *   KTextEditor::ModificationInterface *iface =
 *       qobject_cast<KTextEditor::ModificationInterface*>( doc );
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
class KTEXTEDITOR_EXPORT ModificationInterface
{
  public:
    /**
     * Virtual destructor.
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
     * Set the document's modified-on-disk state to @p reason.
     * KTextEditor implementations should emit the signal modifiedOnDisk()
     * along with the reason. When the document is in a clean state again the
     * reason should be ModifiedOnDiskReason::OnDiskModified.
     *
     * @param reason the modified-on-disk reason.
     * @see ModifiedOnDiskReason, modifiedOnDisk()
     */
    virtual void setModifiedOnDisk( ModifiedOnDiskReason reason ) = 0;

   /**
    * Control, whether the editor should show a warning dialog whenever a file
    * was modified on disk. If @p on is @e true the editor will show warning
    * dialogs.
    * @param on controls, whether the editor should show a warning dialog for
    *        files modified on disk
    */
   virtual void setModifiedOnDiskWarning ( bool on ) = 0;

  /*
   * These stuff is implemented as SLOTS in the real document
   */
  public:
    /**
     * Ask the user what to do, if the file was modified on disk.
     * The argument @p view is used to avoid asking again, when the editor
     * regains focus after the dialog is hidden.
     * @param view the view that should be notified of the user's decision
     * @see setModifiedOnDisk(), modifiedOnDisk()
     */
    virtual void slotModifiedOnDisk( View *view = 0 ) = 0;

  /*
   * These stuff is implemented as SIGNALS in the real document
   */
  public:
    /**
     * This signal is emitted whenever the @p document changed its
     * modified-on-disk state.
     * @param document the Document object that represents the file on disk
     * @param isModified if @e true, the file was modified rather than created
     *        or deleted
     * @param reason the reason why the signal was emitted
     * @see setModifiedOnDisk()
     */
    virtual void modifiedOnDisk (KTextEditor::Document *document,
                                 bool isModified,
                                 KTextEditor::ModificationInterface::ModifiedOnDiskReason reason) = 0;
};

}

Q_DECLARE_INTERFACE(KTextEditor::ModificationInterface, "org.kde.KTextEditor.ModificationInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

