/*
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

    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#include "variableinterface.h"
#include "document.h"

using namespace KTextEditor;

unsigned int VariableInterface::globalVariableInterfaceNumber = 0;

VariableInterface::VariableInterface()
{
  globalVariableInterfaceNumber++;
  myVariableInterfaceNumber = globalVariableInterfaceNumber++;
}

VariableInterface::~VariableInterface()
{
}

unsigned int VariableInterface::variableInterfaceNumber()
{
  return myVariableInterfaceNumber;
}

VariableInterface *KTextEditor::variableInterface( Document *doc )
{
  if ( ! doc )
    return 0;

  return static_cast<VariableInterface*>(doc->qt_cast("KTextEditor::VariableInterface"));
}
