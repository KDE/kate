/* This file is part of the KDE libraries
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

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

#ifndef KATE_UNDOMANAGER_TEST_H
#define KATE_UNDOMANAGER_TEST_H

#include <QtCore/QObject>

class KateDocument;
class KateView;

class UndoManagerTest : public QObject
{
  Q_OBJECT

  public:
    UndoManagerTest();
    virtual ~UndoManagerTest();

  private Q_SLOTS:
    void init();
    void cleanup();
    void testSimpleUndo();

  private:
    KateDocument* m_doc;
    KateView *m_view;
};

#endif
