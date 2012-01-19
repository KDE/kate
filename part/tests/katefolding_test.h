/* This file is part of the KDE libraries
   Copyright (C) 2010 Milian Wolff <mail@milianw.de>

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

#ifndef KATE_FOLDING_TEST_H
#define KATE_FOLDING_TEST_H

#include <QtCore/QObject>
#include "katecodefolding.h"

class KateFoldingTest : public QObject
{
  //friend class KateCodeFoldingTree;

  Q_OBJECT

//public:

private Q_SLOTS:

  void testFolding_data();
  void testFolding();
  void testFoldingReload();
  void testFolding_py_lang();
  void testFolding_rb_lang();
  void testFolding_expand_collapse_level234();
  void testFolding_collapse_expand_local();
  void testFolding_collapse_dsComments_C();
  void testFolding_collapse_dsComments_XML();
  void testFindNodeForPosition();
};

#endif // KATE_FOLDING_TEST_H
