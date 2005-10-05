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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katefont.h"

#include <kglobalsettings.h>

#include <qfontinfo.h>

//
// KateFontMetrics implementation
//

KateFontMetrics::KateFontMetrics(const QFont& f) : QFontMetrics(f)
{
  for (int i=0; i<256; i++) warray[i]=0;
}

KateFontMetrics::~KateFontMetrics()
{
  for (int i=0; i<256; i++)
    delete[] warray[i];
}

short * KateFontMetrics::createRow (short *wa, uchar row)
{
  wa=warray[row]=new short[256];

  for (int i=0; i<256; i++) wa[i]=-1;

  return wa;
}

int KateFontMetrics::width(QChar c)
{
  uchar cell=c.cell();
  uchar row=c.row();
  short *wa=warray[row];

  if (!wa)
    wa = createRow (wa, row);

  if (wa[cell]<0) wa[cell]=(short) QFontMetrics::width(c);

  return (int)wa[cell];
}

//
// KateFontStruct implementation
//

KateFontStruct::KateFontStruct()
: myFont(KGlobalSettings::fixedFont()),
  myFontBold(KGlobalSettings::fixedFont()),
  myFontItalic(KGlobalSettings::fixedFont()),
  myFontBI(KGlobalSettings::fixedFont()),
  myFontMetrics(myFont),
  myFontMetricsBold(myFontBold),
  myFontMetricsItalic(myFontItalic),
  myFontMetricsBI(myFontBI),
  m_fixedPitch (false)
{
  updateFontData ();
}

KateFontStruct::~KateFontStruct()
{
}

void KateFontStruct::updateFontData ()
{
  int maxAscent = myFontMetrics.ascent();
  int maxDescent = myFontMetrics.descent();

  fontHeight = maxAscent + maxDescent + 1;
  fontAscent = maxAscent;
  
  m_fixedPitch = QFontInfo( myFont ).fixedPitch();
}

void KateFontStruct::setFont (const QFont & font)
{  
  QFontMetrics testFM (font);

  // no valid font tried
  if ((testFM.ascent() + testFM.descent() + 1) < 1)
    return;

  myFont = font;

  myFontBold = QFont (font);
  myFontBold.setBold (true);

  myFontItalic = QFont (font);
  myFontItalic.setItalic (true);

  myFontBI = QFont (font);
  myFontBI.setBold (true);
  myFontBI.setItalic (true);

  myFontMetrics = KateFontMetrics (myFont);
  myFontMetricsBold = KateFontMetrics (myFontBold);
  myFontMetricsItalic = KateFontMetrics (myFontItalic);
  myFontMetricsBI = KateFontMetrics (myFontBI);

  updateFontData ();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
