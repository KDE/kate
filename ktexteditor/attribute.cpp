/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

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

#include "attribute.h"

using namespace KTextEditor;

Attribute::Attribute()
  : m_weight(QFont::Normal)
  , m_italic(false)
  , m_underline(false)
  , m_overline(false)
  , m_strikeout(false)
  , m_bgColorFillWhitespace(false)
  , m_itemsSet(0)
{
  for (int i = 0; i < 4; ++i)
    m_dynamicAttributes[i] = 0L;
}

Attribute::~Attribute()
{
}

const QTextCharFormat & Attribute::toFormat() const
{
  return m_format;
}

void Attribute::clear()
{
  m_itemsSet=0;
}

Attribute& Attribute::operator+=(const Attribute& a)
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
    setBGColor(a.bgColor(), a.bgColorFillWhitespace());

  if (a.itemSet(SelectedBGColor))
    setSelectedBGColor(a.selectedBGColor());

  m_associatedActions += a.associatedActions();

  return *this;
}

QFont Attribute::font(const QFont& ref)
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

void Attribute::setWeight(int weight)
{
  if (!(m_itemsSet & Weight) || m_weight != weight)
  {
    m_itemsSet |= Weight;

    m_weight = weight;
    m_format.setFontWeight(m_weight);

    changed();
  }
}

void Attribute::setBold(bool enable)
{
  setWeight(enable ? QFont::Bold : QFont::Normal);
}

void Attribute::setItalic(bool enable)
{
  if (!(m_itemsSet & Italic) || m_italic != enable)
  {
    m_itemsSet |= Italic;

    m_italic = enable;
    m_format.setFontItalic(m_italic);

    changed();
  }
}

void Attribute::setUnderline(bool enable)
{
  if (!(m_itemsSet & Underline) || m_underline != enable)
  {
    m_itemsSet |= Underline;

    m_underline = enable;
    m_format.setFontUnderline(m_underline);

    changed();
  }
}

void Attribute::setOverline(bool enable)
{
  if (!(m_itemsSet & Overline) || m_overline != enable)
  {
    m_itemsSet |= Overline;

    m_overline = enable;
    m_format.setFontOverline(m_overline);

    changed();
  }
}

void Attribute::setStrikeOut(bool enable)
{
  if (!(m_itemsSet & StrikeOut) || m_strikeout != enable)
  {
    m_itemsSet |= StrikeOut;

    m_strikeout = enable;
    m_format.setFontStrikeOut(m_strikeout);

    changed();
  }
}

void Attribute::setOutline(const QColor& color)
{
  if (!(m_itemsSet & Outline) || m_outline != color)
  {
    m_itemsSet |= Outline;

    m_outline = color;
    //m_format.setFontOutline(m_outline);

    changed();
  }
}

void Attribute::setTextColor(const QColor& color)
{
  if (!(m_itemsSet & TextColor) || m_textColor != color)
  {
    m_itemsSet |= TextColor;

    m_textColor = color;
    m_format.setForeground(m_textColor);

    changed();
  }
}

void Attribute::setSelectedTextColor(const QColor& color)
{
  if (!(m_itemsSet & SelectedTextColor) || m_selectedTextColor != color)
  {
    m_itemsSet |= SelectedTextColor;

    m_selectedTextColor = color;

    changed();
  }
}

void Attribute::setBGColor(const QColor& color, bool fillWhitespace)
{
  if (!(m_itemsSet & BGColor) || m_bgColor != color || m_bgColorFillWhitespace != fillWhitespace)
  {
    m_itemsSet |= BGColor;

    m_bgColor = color;
    m_bgColorFillWhitespace = fillWhitespace;
    m_format.setBackground(color);

    changed();
  }
}

void Attribute::setSelectedBGColor(const QColor& color)
{
  if (!(m_itemsSet & SelectedBGColor) || m_selectedBGColor != color)
  {
    m_itemsSet |= SelectedBGColor;

    m_selectedBGColor = color;

    changed();
  }
}

bool operator !=(const Attribute& h1, const Attribute& h2)
{
  return !(h1 == h2);
}

void KTextEditor::Attribute::addRange( SmartRange * range )
{
  m_usingRanges.append(range);
}

void KTextEditor::Attribute::removeRange( SmartRange * range )
{
  m_usingRanges.remove(range);
}

Attribute * KTextEditor::Attribute::dynamicAttribute( ActivationFlags activationFlags ) const
{
  switch (activationFlags) {
    case ActivateCaretIn:
      return m_dynamicAttributes[0];
    case ActivateCaretOut:
      return m_dynamicAttributes[1];
    case ActivateMouseIn:
      return m_dynamicAttributes[2];
    case ActivateMouseOut:
      return m_dynamicAttributes[3];
    default:
      return 0L;
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
