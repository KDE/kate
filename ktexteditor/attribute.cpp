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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "attribute.h"

using namespace KTextEditor;

class KTextEditor::AttributePrivate
{
  public:
    AttributePrivate()
    {
      dynamicAttributes.append(Attribute::Ptr());
      dynamicAttributes.append(Attribute::Ptr());
    }

    QList<KAction*> associatedActions;
    QList<Attribute::Ptr> dynamicAttributes;
};

Attribute::Attribute()
  : d(new AttributePrivate())
{
}

Attribute::Attribute( const Attribute & a )
  : QTextCharFormat(a)
  , QSharedData()
  , d(new AttributePrivate())
{
  d->associatedActions = a.d->associatedActions;
  d->dynamicAttributes = a.d->dynamicAttributes;
}

Attribute::~Attribute()
{
  delete d;
}

Attribute& Attribute::operator+=(const Attribute& a)
{
  merge(a);

  d->associatedActions += a.associatedActions();

  for (int i = 0; i < a.d->dynamicAttributes.count(); ++i)
    if (i < d->dynamicAttributes.count()) {
      if (a.d->dynamicAttributes[i])
        d->dynamicAttributes[i] = a.d->dynamicAttributes[i];
    } else {
      d->dynamicAttributes.append(a.d->dynamicAttributes[i]);
    }

  return *this;
}

Attribute::Ptr Attribute::dynamicAttribute(ActivationType type) const
{
  if (type < 0 || type >= d->dynamicAttributes.count())
    return Ptr();

  return d->dynamicAttributes[type];
}

void Attribute::setDynamicAttribute( ActivationType type, Attribute::Ptr attribute )
{
  if (type < 0 || type > ActivateCaretIn)
    return;

  d->dynamicAttributes[type] = attribute;
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
  static_cast<QTextCharFormat>(*this) = QTextCharFormat();

  d->associatedActions.clear();
  d->dynamicAttributes.clear();
  d->dynamicAttributes.append(Ptr());
  d->dynamicAttributes.append(Ptr());
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

Attribute & KTextEditor::Attribute::operator =( const Attribute & a )
{
  QTextCharFormat::operator=(a);
  Q_ASSERT(static_cast<QTextCharFormat>(*this) == a);

  d->associatedActions = a.d->associatedActions;
  d->dynamicAttributes = a.d->dynamicAttributes;

  return *this;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
