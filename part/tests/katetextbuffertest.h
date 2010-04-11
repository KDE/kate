/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATEBUFFERTEST_H
#define KATEBUFFERTEST_H

#include <QtTest/QtTest>
#include <QtCore/QObject>

class KateTextBufferTest : public QObject
{
  Q_OBJECT

  public:
    KateTextBufferTest();
    virtual ~KateTextBufferTest();

  private Q_SLOTS:
    void basicBufferTest();
    void wrapLineTest();
    void insertRemoveTextTest();
    void cursorTest();
};

#endif // KATEBUFFERTEST_H
