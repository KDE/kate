/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>

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

#ifndef KATEATTRIBUTE_H
#define KATEATTRIBUTE_H

#include "katefont.h"

#include <qcolor.h>

/**
 * The Attribute class incorporates all text decorations supported by Kate.
 *
 * TODO: store the actual font as well.
 * TODO: update changed mechanism - use separate bitfield
 */
class KateAttribute
{
public:
  enum items {
    Weight = 0x1,
    Bold = 0x2,
    Italic = 0x4,
    Underline = 0x8,
    StrikeOut = 0x10,
    Outline = 0x20,
    TextColor = 0x40,
    SelectedTextColor = 0x80,
    BGColor = 0x100,
    SelectedBGColor = 0x200
  };

  KateAttribute();
  virtual ~KateAttribute();

  QFont font(const QFont& ref);

  inline int width(FontStruct& fs, const QString& text, int col, int tabWidth) const
  { return fs.width(text, col, bold(), italic(), tabWidth); };

  // Non-preferred function when you have a string and you want one char's width!!
  inline int width(FontStruct& fs, const QChar& c, int tabWidth) const
  { return fs.width(c, bold(), italic(), tabWidth); };

  inline bool itemSet(int item) const
  { return item & m_itemsSet; };

  inline bool isSomethingSet() const
  { return m_itemsSet; };

  inline int itemsSet() const
  { return m_itemsSet; };

  inline void clearAttribute(int item)
  { m_itemsSet &= (~item); }

  inline int weight() const
  { return m_weight; };

  void setWeight(int weight);

  bool bold() const;
  void setBold(bool enable = true);

  bool italic() const;
  void setItalic(bool enable = true);

  bool underline() const;
  void setUnderline(bool enable = true);

  bool strikeOut() const;
  void setStrikeOut(bool enable = true);

  const QColor& outline() const;
  void setOutline(const QColor& color);

  const QColor& textColor() const;
  void setTextColor(const QColor& color);

  const QColor& selectedTextColor() const;
  void setSelectedTextColor(const QColor& color);

  const QColor& bgColor() const;
  void setBGColor(const QColor& color);

  const QColor& selectedBGColor() const;
  void setSelectedBGColor(const QColor& color);

  KateAttribute& operator+=(const KateAttribute& a);

  friend bool operator ==(const KateAttribute& h1, const KateAttribute& h2);
  friend bool operator !=(const KateAttribute& h1, const KateAttribute& h2);

  virtual void changed() { m_changed = true; };
  bool isChanged() { bool ret = m_changed; m_changed = false; return ret; };

  void clear();

private:
  int m_weight;
  bool m_italic, m_underline, m_strikeout, m_changed;
  QColor m_outline, m_textColor, m_selectedTextColor, m_bgColor, m_selectedBGColor;
  int m_itemsSet;
};

#endif
