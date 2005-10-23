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
{
  for (int i = 0; i < 4; ++i)
    m_dynamicAttributes[i] = 0L;
}

Attribute::~Attribute()
{
}

Attribute& Attribute::operator+=(const Attribute& a)
{
  merge(a);

  m_associatedActions += a.associatedActions();

  return *this;
}

void Attribute::addRange( SmartRange * range )
{
  m_usingRanges.append(range);
}

void Attribute::removeRange( SmartRange * range )
{
  m_usingRanges.remove(range);
}

Attribute * Attribute::dynamicAttribute( ActivationFlags activationFlags ) const
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

QBrush Attribute::outline( ) const
{
  if (properties().contains(Outline))
    return qVariantValue<QBrush>(properties()[Outline]);

  return QBrush();
}

void Attribute::setOutline( const QBrush & brush )
{
  setProperty(Outline, brush);
}

QBrush Attribute::selectedForeground( ) const
{
  if (properties().contains(SelectedForeground))
    return qVariantValue<QBrush>(properties()[SelectedForeground]);

  return QBrush();
}

void Attribute::setSelectedForeground( const QBrush & foreground )
{
  setProperty(SelectedForeground, foreground);
}

bool Attribute::backgroundFillWhitespace( ) const
{
  if (properties().contains(BackgroundFillWhitespace))
    return properties()[BackgroundFillWhitespace].toBool();

  return true;
}

void Attribute::setBackgroundFillWhitespace( bool fillWhitespace )
{
  setProperty(BackgroundFillWhitespace, fillWhitespace);
}

QBrush Attribute::selectedBackground( ) const
{
  if (properties().contains(SelectedBackground))
    return qVariantValue<QBrush>(properties()[SelectedBackground]);

  return QBrush();
}

void Attribute::setSelectedBackground( const QBrush & brush )
{
  setProperty(SelectedBackground, brush);
}

void Attribute::clear( )
{
  *this = Attribute();
}

bool Attribute::fontBold( ) const
{
  return fontWeight() == QFont::Bold;
}

void Attribute::setFontBold( bool bold )
{
  setFontWeight(bold ? QFont::Bold : 0);
}

void Attribute::clearAssociatedActions( )
{
  m_associatedActions.clear();
}

bool Attribute::hasAnyProperty( ) const
{
  return properties().count();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
