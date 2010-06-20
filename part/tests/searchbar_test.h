/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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

#ifndef KATE_SEARCHBAR_TEST_H
#define KATE_SEARCHBAR_TEST_H

#include <QtCore/QObject>

class SearchBarTest : public QObject
{
  Q_OBJECT

public:
  SearchBarTest();
  ~SearchBarTest();

private Q_SLOTS:
  void testSetMatchCaseIncremental();

  void testSetMatchCasePower();

  void testSetSelectionOnlyPower();

  void testSetSearchPattern_data();
  void testSetSearchPattern();

  void testSetSelectionOnly();

  void testFindAll_data();
  void testFindAll();

  void testReplaceAll();

  void testFindSelectionForward_data();
  void testFindSelectionForward();

  void testRemoveWithSelectionForward_data();
  void testRemoveWithSelectionForward();

  void testRemoveInSelectionForward_data();
  void testRemoveInSelectionForward();

  void testReplaceWithDoubleSelecion_data();
  void testReplaceWithDoubleSelecion();

  void testReplaceDollar();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
