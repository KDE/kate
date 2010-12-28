/* This file is part of the KDE libraries
   Copyright (C) 20010 David Nolden <david.nolden.kdevelop@art-master.de>

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

#ifndef KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_V4_H
#define KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_V4_H

#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

#include "katepartprivate_export.h"

/// @todo For KDE 4.7 this interface (or a version of it) should go into interfaces/ktexteditor

namespace KTextEditor {
//BEGIN V4
///Extension of CodeCompletionModelControllerInterface3
class KATEPART_TESTS_EXPORT_DEPRECATED CodeCompletionModelControllerInterface4 : public CodeCompletionModelControllerInterface3 {
  public:

    /**
     * When multiple completion models are used at the same time, it may happen that multiple models add items with the same
     * name to the list. This option allows to hide items from this completion model when another model with higher priority
     * contains items with the same name.
     * \return Whether items of this completion model should be hidden if another completion model has items with the same name
     */
    virtual bool shouldHideItemsWithEqualNames() const;
};
//END V4
}

Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionModelControllerInterface4, "org.kde.KTextEditor.CodeCompletionModelControllerInterface4")
#endif // KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_V4_H
