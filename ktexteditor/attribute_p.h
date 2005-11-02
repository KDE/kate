/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_ATTRIBUTE_P_H
#define KDELIBS_KTEXTEDITOR_ATTRIBUTE_P_H

#include <QList>
//#include <QSet>

#include "attribute.h"

class KAction;

namespace KTextEditor
{

class AttributePrivate
{
  public:
    AttributePrivate();
    ~AttributePrivate();

    //QSet<SmartRange*> usingRanges;
    QList<KAction*> associatedActions;
    Attribute* dynamicAttributes[2];
    bool ownsDynamicAttributes[2];
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
