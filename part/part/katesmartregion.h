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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATESMARTREGION_H
#define KATESMARTREGION_H

#include <QList>

#include <ktexteditor/range.h>

class KateSmartRange;
class KateDocument;

/**
 * A class which takes several SmartRanges and produces both
 * a bounding range and a list of simplified SmartRanges
 *
 * @author Hamish Rodda \<rodda@kde.org\>
 */
class KateSmartRegion
{
  public:
    KateSmartRegion(KateDocument* document);

    void addRange(KateSmartRange* range);
    void removeRange(KateSmartRange* range);

    const KTextEditor::Range& boundingRange() const;

  private:
    void calculateBounds();

    KateSmartRange* m_bounding;
    QList<KateSmartRange*> m_source;
};

#endif
