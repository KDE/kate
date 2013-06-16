/* This file is part of the KDE project
   Copyright (C) 2010 David Nolden (david.nolden.kdevelop@art-master.de)

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

#ifndef KDELIBS_KTEXTEDITOR_RECOVERYINTERFACE_H
#define KDELIBS_KTEXTEDITOR_RECOVERYINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>

namespace KTextEditor
{
/**
 * \brief Document extension interface to control crash recovery.
 *
 * \ingroup kte_group_doc_extensions
 *
 * When the system or the application using the editor component crashed
 * with unsaved changes in the Document, the View notifies the user about
 * the lost data and asks, whether the data should be recovered.
 *
 * This interface gives you control over the data recovery process. Use
 * isDataRecoveryAvailable() to check for lost data. If you do not want the
 * editor component to handle the data recovery process automatically, you can
 * either trigger the data recovery by calling recoverData() or discard it
 * by discardDataRecovery().
 *
 * \since 4.6
 */
class KTEXTEDITOR_EXPORT RecoveryInterface
{
  public:
    /**
     * Constructor.
     */
    RecoveryInterface();

    /**
     * Virtual destructor.
     */
    virtual ~RecoveryInterface();

  public:

    /**
     * Returns whether a recovery is available for the current document.
     *
     * \see recoverData(), discardDataRecovery()
     */
    virtual bool isDataRecoveryAvailable() const = 0;

    /**
     * If recover data is available, calling recoverData() will trigger the
     * recovery of the data. If isDataRecoveryAvailable() returns \e false,
     * calling this function does nothing.
     *
     * \see isDataRecoveryAvailable(), discardDataRecovery()
     */
    virtual void recoverData() = 0;

    /**
     * If recover data is available, calling discardDataRecovery() will discard
     * the recover data and the recover data is lost.
     * If isDataRecoveryAvailable() returns \e false, calling this function
     * does nothing.
     *
     * \see isDataRecoveryAvailable(), recoverData()
     */
    virtual void discardDataRecovery() = 0;

    /**
     * TODO KDE5 (see SwapFile::setTrackingEnabled())
     * Enables or disables the writing of swap files.
     * @note Enabling/disabling only works when the document is \e not modified!
     */
    // void setDataRecoveryEnabled(bool enable);
    // bool dataRecoveryEnabled() const;

  private:
    class RecoveryInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::RecoveryInterface, "org.kde.KTextEditor.RecoveryInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
