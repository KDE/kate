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

#include "attribute_p.h"

using namespace KTextEditor;

AttributePrivate::AttributePrivate()
{
  for (int i = 0; i < Attribute::ActivateCaretIn; ++i) {
    dynamicAttributes[i] = 0L;
    ownsDynamicAttributes[i] = false;
  }
}

AttributePrivate::~AttributePrivate()
{
  for (int i = 0; i < Attribute::ActivateCaretIn; ++i) {
    if (ownsDynamicAttributes[i])
      delete dynamicAttributes[i];
  }
}

Attribute::Attribute()
  : d(new AttributePrivate())
{
}

Attribute::~Attribute()
{
}

Attribute& Attribute::operator+=(const Attribute& a)
{
  merge(a);

  d->associatedActions += a.associatedActions();

  return *this;
}

/*void Attribute::addRange( SmartRange * range )
{
  d->usingRanges.insert(range);
}

void Attribute::removeRange( SmartRange * range )
{
  d->usingRanges.remove(range);
}*/

Attribute * Attribute::dynamicAttribute(ActivationType type) const
{
  if (type < 0 || type > ActivateCaretIn)
    return 0L;

  return d->dynamicAttributes[type];
}

void Attribute::setDynamicAttribute( ActivationType type, Attribute * attribute, bool takeOwnership )
{
  if (type < 0 || type > ActivateCaretIn)
    return;

  if (d->ownsDynamicAttributes[type])
    delete d->dynamicAttributes[type];

  d->dynamicAttributes[type] = attribute;
  d->ownsDynamicAttributes[type] = takeOwnership;
}

QBrush Attribute::outline( ) const
{
  if (hasProperty(Outline))
    return qVariantValue<QBrush>(property(Outline));

  return QBrush();
}

void Attribute::setOutline( const QBrush & brush )
{
  setProperty(Outline, brush);
}

QBrush Attribute::selectedForeground( ) const
{
  if (hasProperty(SelectedForeground))
    return qVariantValue<QBrush>(property(SelectedForeground));

  return QBrush();
}

void Attribute::setSelectedForeground( const QBrush & foreground )
{
  setProperty(SelectedForeground, foreground);
}

bool Attribute::backgroundFillWhitespace( ) const
{
  if (hasProperty(BackgroundFillWhitespace))
    return boolProperty(BackgroundFillWhitespace);

  return true;
}

void Attribute::setBackgroundFillWhitespace( bool fillWhitespace )
{
  setProperty(BackgroundFillWhitespace, fillWhitespace);
}

QBrush Attribute::selectedBackground( ) const
{
  if (hasProperty(SelectedBackground))
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
  d->associatedActions.clear();
}

bool Attribute::hasAnyProperty( ) const
{
  return properties().count();
}

const QList< KAction * > & Attribute::associatedActions( ) const
{
  return d->associatedActions;
}

Attribute::Effects KTextEditor::Attribute::effects( ) const
{
  if (hasProperty(AttributeDynamicEffect))
    return static_cast<Effects>(intProperty(AttributeDynamicEffect));

  return EffectNone;
}

void KTextEditor::Attribute::setEffects( Effects effects )
{
  setProperty(AttributeDynamicEffect, QVariant(effects));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
