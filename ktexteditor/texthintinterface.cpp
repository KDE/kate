/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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


#include "texthintinterface.h"
#include "view.h"

using namespace KTextEditor;

namespace KTextEditor
{
  class PrivateTextHintInterface
  {
    public:
      PrivateTextHintInterface() {}
      ~PrivateTextHintInterface(){}

  };

unsigned int TextHintInterface::globalTextHintInterfaceNumber = 0;

TextHintInterface::TextHintInterface()
{
  globalTextHintInterfaceNumber++;
  myTextHintInterfaceNumber = globalTextHintInterfaceNumber++;

  d = new PrivateTextHintInterface();
}

TextHintInterface::~TextHintInterface()
{
  delete d;
}

unsigned int TextHintInterface::textHintInterfaceNumber () const
{
  return myTextHintInterfaceNumber;
}


TextHintInterface *textHintInterface (View *view)
{                
  if (!view)
    return 0;

  return static_cast<TextHintInterface*>(view->qt_cast("KTextEditor::TextHintInterface"));
}


}

