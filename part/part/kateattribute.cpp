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

#include "kateattribute.h"

KateAttribute::KateAttribute()
  : m_weight(QFont::Normal)
  , m_italic(false)
  , m_underline(false)
  , m_overline(false)
  , m_strikeout(false)
  , m_itemsSet(0)

{
}

KateAttribute::~KateAttribute()
{
}

void KateAttribute::clear()
{
  m_itemsSet=0;
}

KateAttribute& KateAttribute::operator+=(const KateAttribute& a)
{
  if (a.itemSet(Weight))
    setWeight(a.weight());

  if (a.itemSet(Italic))
    setItalic(a.italic());

  if (a.itemSet(Underline))
    setUnderline(a.underline());

  if (a.itemSet(Overline))
    setOverline(a.overline());

  if (a.itemSet(StrikeOut))
    setStrikeOut(a.strikeOut());

  if (a.itemSet(Outline))
    setOutline(a.outline());

  if (a.itemSet(TextColor))
    setTextColor(a.textColor());

  if (a.itemSet(SelectedTextColor))
    setSelectedTextColor(a.selectedTextColor());

  if (a.itemSet(BGColor))
    setBGColor(a.bgColor());

  if (a.itemSet(SelectedBGColor))
    setSelectedBGColor(a.selectedBGColor());

  return *this;
}

QFont KateAttribute::font(const QFont& ref)
{
  QFont ret = ref;

  if (itemSet(Weight))
    ret.setWeight(weight());
  if (itemSet(Italic))
    ret.setItalic(italic());
  if (itemSet(Underline))
    ret.setUnderline(underline());
   if (itemSet(Overline))
    ret.setOverline(overline());
  if (itemSet(StrikeOut))
    ret.setStrikeOut(strikeOut());

  return ret;
}

void KateAttribute::setWeight(int weight)
{
  if (!(m_itemsSet & Weight) || m_weight != weight)
  {
    m_itemsSet |= Weight;

    m_weight = weight;

    changed();
  }
}

void KateAttribute::setBold(bool enable)
{
  setWeight(enable ? QFont::Bold : QFont::Normal);
}

void KateAttribute::setItalic(bool enable)
{
  if (!(m_itemsSet & Italic) || m_italic != enable)
  {
    m_itemsSet |= Italic;

    m_italic = enable;

    changed();
  }
}

void KateAttribute::setUnderline(bool enable)
{
  if (!(m_itemsSet & Underline) || m_underline != enable)
  {
    m_itemsSet |= Underline;

    m_underline = enable;

    changed();
  }
}

void KateAttribute::setOverline(bool enable)
{
  if (!(m_itemsSet & Overline) || m_overline != enable)
  {
    m_itemsSet |= Overline;

    m_overline = enable;

    changed();
  }
}

void KateAttribute::setStrikeOut(bool enable)
{
  if (!(m_itemsSet & StrikeOut) || m_strikeout != enable)
  {
    m_itemsSet |= StrikeOut;

    m_strikeout = enable;

    changed();
  }
}

void KateAttribute::setOutline(const QColor& color)
{
  if (!(m_itemsSet & Outline) || m_outline != color)
  {
    m_itemsSet |= Outline;

    m_outline = color;

    changed();
  }
}

void KateAttribute::setTextColor(const QColor& color)
{
  if (!(m_itemsSet & TextColor) || m_textColor != color)
  {
    m_itemsSet |= TextColor;

    m_textColor = color;

    changed();
  }
}

void KateAttribute::setSelectedTextColor(const QColor& color)
{
  if (!(m_itemsSet & SelectedTextColor) || m_selectedTextColor != color)
  {
    m_itemsSet |= SelectedTextColor;

    m_selectedTextColor = color;

    changed();
  }
}

void KateAttribute::setBGColor(const QColor& color)
{
  if (!(m_itemsSet & BGColor) || m_bgColor != color)
  {
    m_itemsSet |= BGColor;

    m_bgColor = color;

    changed();
  }
}

void KateAttribute::setSelectedBGColor(const QColor& color)
{
  if (!(m_itemsSet & SelectedBGColor) || m_selectedBGColor != color)
  {
    m_itemsSet |= SelectedBGColor;

    m_selectedBGColor = color;

    changed();
  }
}

bool operator ==(const KateAttribute& h1, const KateAttribute& h2)
{
  if (h1.m_itemsSet != h2.m_itemsSet)
    return false;

  if (h1.itemSet(KateAttribute::Weight))
    if (h1.m_weight != h2.m_weight)
      return false;

  if (h1.itemSet(KateAttribute::Italic))
    if (h1.m_italic != h2.m_italic)
      return false;

  if (h1.itemSet(KateAttribute::Underline))
    if (h1.m_underline != h2.m_underline)
      return false;

  if (h1.itemSet(KateAttribute::StrikeOut))
    if (h1.m_strikeout != h2.m_strikeout)
      return false;

  if (h1.itemSet(KateAttribute::Outline))
    if (h1.m_outline != h2.m_outline)
      return false;

  if (h1.itemSet(KateAttribute::TextColor))
    if (h1.m_textColor != h2.m_textColor)
      return false;

  if (h1.itemSet(KateAttribute::SelectedTextColor))
    if (h1.m_selectedTextColor != h2.m_selectedTextColor)
      return false;

  if (h1.itemSet(KateAttribute::BGColor))
    if (h1.m_bgColor != h2.m_bgColor)
      return false;

  if (h1.itemSet(KateAttribute::SelectedBGColor))
    if (h1.m_selectedBGColor != h2.m_selectedBGColor)
      return false;

  return true;
}

bool operator !=(const KateAttribute& h1, const KateAttribute& h2)
{
  return !(h1 == h2);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
