/* This file is part of the KDE libraries
   Copyright (C) 2002 Christian Couder <christian@kdevelop.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katefont.h"

#include <kglobalsettings.h>

//
// FontStruct implementation
//

FontStruct::FontStruct()
: myFont(KGlobalSettings::fixedFont()),
  myFontBold(KGlobalSettings::fixedFont()),
  myFontItalic(KGlobalSettings::fixedFont()),
  myFontBI(KGlobalSettings::fixedFont()),
  myFontMetrics(myFont),
  myFontMetricsBold(myFontBold),
  myFontMetricsItalic(myFontItalic),
  myFontMetricsBI(myFontBI)
{
  updateFontData ();
}

FontStruct::~FontStruct()
{
}

void FontStruct::updateFontData ()
{
  int maxAscent = myFontMetrics.ascent();
  int maxDescent = myFontMetrics.descent();

  fontHeight = maxAscent + maxDescent + 1;
  fontAscent = maxAscent;
}

int FontStruct::width(const QString& text, int col, bool bold, bool italic, int tabWidth) const
{
  if (text[col] == QChar('\t'))
    return tabWidth * myFontMetrics.width(' ');

  return (bold) ?
    ( (italic) ?
      myFontMetricsBI.charWidth(text, col) :
      myFontMetricsBold.charWidth(text, col) ) :
    ( (italic) ?
      myFontMetricsItalic.charWidth(text, col) :
      myFontMetrics.charWidth(text, col) );
}

int FontStruct::width(const QChar& c, bool bold, bool italic, int tabWidth) const
{
  if (c == QChar('\t'))
    return tabWidth * myFontMetrics.width(' ');

  return (bold) ?
    ( (italic) ?
      myFontMetricsBI.width(c) :
      myFontMetricsBold.width(c) ) :
    ( (italic) ?
      myFontMetricsItalic.width(c) :
      myFontMetrics.width(c) );
}

const QFont& FontStruct::font(bool bold, bool italic) const
{
  return (bold) ?
    ( (italic) ? myFontBI : myFontBold ) :
    ( (italic) ? myFontItalic : myFont );
}

void FontStruct::setFont (const QFont & font)
{
  myFont = font;

  myFontBold = QFont (font);
  myFontBold.setBold (true);

  myFontItalic = QFont (font);
  myFontItalic.setItalic (true);

  myFontBI = QFont (font);
  myFontBI.setBold (true);
  myFontBI.setItalic (true);

  myFontMetrics = QFontMetrics (myFont);
  myFontMetricsBold = QFontMetrics (myFontBold);
  myFontMetricsItalic = QFontMetrics (myFontItalic);
  myFontMetricsBI = QFontMetrics (myFontBI);

  updateFontData ();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
