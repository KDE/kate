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
  * An interface that allows controlling the crash recovery functionality of the
  * underlying editor component.
  * 
  * When the editor crashed previously with some modifications, and the same document
  * is opened again, a recovery will be available:
  *   RecoveryInterface::haveRecovery() will return true
  * 
  * When executing RecoveryInterface::doRecovery(), the recovery will be applied.
  * 
  * When executing RecoveryInterface::discardRecovery(), the recovery will be discarded.
  * */
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
     * */
    virtual bool haveRecovery() const = 0;
    virtual void doRecovery() = 0;
    virtual void discardRecovery() = 0;

  private:
    class RecoveryInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::RecoveryInterface, "org.kde.KTextEditor.RecoveryInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
