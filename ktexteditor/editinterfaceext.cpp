/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "editinterfaceext.h"
#include "document.h"

using namespace KTextEditor;

uint EditInterfaceExt::globalEditInterfaceExtNumber = 0;

EditInterfaceExt::EditInterfaceExt()
	: d(0L)
{
  globalEditInterfaceExtNumber++;
  myEditInterfaceExtNumber = globalEditInterfaceExtNumber;
}

EditInterfaceExt::~EditInterfaceExt()
{
}

uint EditInterfaceExt::editInterfaceExtNumber() const
{
  return myEditInterfaceExtNumber;
}

EditInterfaceExt *KTextEditor::editInterfaceExt (Document *doc)
{
  if (!doc)
    return 0;

  return static_cast<EditInterfaceExt*>(doc->qt_cast("KTextEditor::EditInterfaceExt"));
}

