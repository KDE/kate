/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann (cullmann@kde.org)
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "smartinterface.h"

#include <QMutex>

using namespace KTextEditor;

class KTextEditor::SmartInterfacePrivate
{
public:
  SmartInterfacePrivate()
    : mutex(QMutex::Recursive)
    , clearOnDocumentReload(true)
  {}

  QMutex mutex;
  bool clearOnDocumentReload;
};

SmartInterface::SmartInterface()
  : d(new SmartInterfacePrivate)
{
}

SmartInterface::~ SmartInterface( )
{
  delete d;
}

bool SmartInterface::clearOnDocumentReload() const
{
  return d->clearOnDocumentReload;
}

void SmartInterface::setClearOnDocumentReload(bool clearOnReload)
{
  QMutexLocker lock(smartMutex());
  d->clearOnDocumentReload = clearOnReload;
}

QMutex * SmartInterface::smartMutex() const
{
  return &d->mutex;
}

void SmartInterface::clearRevision()
{
  useRevision(-1);
}

Cursor SmartInterface::translateFromRevision(const Cursor& cursor, SmartCursor::InsertBehavior insertBehavior) const
{
  Q_UNUSED(insertBehavior);
  return cursor;
}

SmartCursor* SmartInterface::newSmartCursor(int line, int column, SmartCursor::InsertBehavior insertBehavior)
{
  return newSmartCursor(Cursor(line, column), insertBehavior);
}

SmartRange* SmartInterface::newSmartRange(const Cursor& startPosition,
                                          const Cursor& endPosition,
                                          SmartRange* parent,
                                          SmartRange::InsertBehaviors insertBehavior)
{
  return newSmartRange(Range(startPosition, endPosition), parent, insertBehavior);
}

SmartRange* SmartInterface::newSmartRange(int startLine, int startColumn, int endLine, int endColumn, SmartRange* parent, SmartRange::InsertBehaviors insertBehavior)
{
  return newSmartRange(Range(startLine, startColumn, endLine, endColumn), parent, insertBehavior);
}

Range SmartInterface::translateFromRevision(const Range& range, SmartRange::InsertBehaviors insertBehavior) const
{
  Q_UNUSED(insertBehavior);
  return range;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
