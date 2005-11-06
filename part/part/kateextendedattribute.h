/* This file is part of the KDE libraries
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATEEXTENDEDATTRIBUTE_H
#define KATEEXTENDEDATTRIBUTE_H

#include <ktexteditor/attribute.h>

class KateExtendedAttribute;

typedef QList<KTextEditor::Attribute*> KateAttributeList;
typedef QList<KateExtendedAttribute*> KateExtendedAttributeList;

/**
 * An extension of the KTextEditor::Attribute class, with convenience functions
 * for access to extra kate-specific information, and a parent heirachy system
 * for display in the config
 */
class KateExtendedAttribute : public KTextEditor::Attribute
{
  public:
    KateExtendedAttribute(const QString& name, int defaultStyleIndex = -1);

    enum DefaultStyle {
      dsNormal,
      dsKeyword,
      dsDataType,
      dsDecVal,
      dsBaseN,
      dsFloat,
      dsChar,
      dsString,
      dsComment,
      dsOthers,
      dsAlert,
      dsFunction,
      dsRegionMarker,
      dsError
    };

    enum InternalProperties {
      AttributeName = AttributeInternalProperty,
      AttributeDefaultStyleIndex
    };

    static int indexForStyleName(const QString& name);

    QString name() const;
    void setName(const QString& name);

    bool isDefaultStyle() const;
    int defaultStyleIndex() const;
    void setDefaultStyleIndex(int index);
};

#endif
