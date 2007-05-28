/* This file is part of the KDE libraries
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_NAMESPACE_H
#define KATE_NAMESPACE_H

namespace Kate
{
  enum EditSource {
    /// Editing from opening a file
    OpenFileEdit,
    /// Editing from closing a file
    CloseFileEdit,
    /// Editing performed by the user
    UserInputEdit,
    /// Editing from cutting, copying and pasting
    CutCopyPasteEdit,
    /// Editing from search + replace algorithms
    SearchReplaceEdit,
    /// Edits from Kate's internal indentation routines
    AutomaticIndentationEdit,
    /// Edits from code completion
    CodeCompletionEdit,
    /// Edits by Kate scripts
    ScriptActionEdit,
    /// Inter-process communication derived edits
    IPCEdit,
    /// Editing from the kate inbuilt command line
    CommandLineEdit,
    /// Editing from a Kate plugin
    PluginEdit,
    /// Editing from a client application, eg. kdevelop.
    ThirdPartyEdit,
    /// Other internal editing done by Kate
    InternalEdit,
    /// An edit type which means that no edit source was otherwise specified, and any preexisting type should be inherited.
    NoEditSource
  };

}

#endif // KATE_NAMESPACE_H

// kate: space-indent on; indent-width 2; replace-tabs on;
