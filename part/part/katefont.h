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

#ifndef __kate_font_h__
#define __kate_font_h__

#include <qfont.h>
#include <qfontmetrics.h>

//
// KateFontMetrics implementation
//

class KateFontMetrics : public QFontMetrics
{
  public:
    KateFontMetrics(const QFont& f);
    ~KateFontMetrics();

    int width(QChar c);

    int width(QString s) { return QFontMetrics::width(s); }

  private:
    short *createRow (short *wa, uchar row);

  private:
    short *warray[256];
};

//
// FontStruct definition
//

class FontStruct
{
  public:
    FontStruct();
    ~FontStruct();

    int width(const QString& text, int col, bool bold, bool italic, int tabWidth);
    int width(const QChar& c, bool bold, bool italic, int tabWidth);

    inline const QFont& font(bool bold, bool italic) const
    {
      return (bold) ?
        ( (italic) ? myFontBI : myFontBold ) :
        ( (italic) ? myFontItalic : myFont );
    }

    void setFont(const QFont & font);

  private:
     void updateFontData ();

  public:
    QFont myFont, myFontBold, myFontItalic, myFontBI;

    KateFontMetrics myFontMetrics, myFontMetricsBold, myFontMetricsItalic, myFontMetricsBI;

    int fontHeight;
    int fontAscent;
};

#endif
