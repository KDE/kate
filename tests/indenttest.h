/*
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

#ifndef INDENTTEST_H
#define INDENTTEST_H

#include <QtCore/QObject>

#include "script_test_base.h"

class IndentTest : public ScriptTestBase
{
  Q_OBJECT
private slots:
  void initTestCase();

  void testPython_data();
  void testPython();

  void testCstyle_data();
  void testCstyle();

  void testCppstyle_data();
  void testCppstyle();

  void testCMake_data();
  void testCMake();

  void testRuby_data();
  void testRuby();

  void testHaskell_data();
  void testHaskell();

  void latex_data();
  void latex();

  void testPascal_data();
  void testPascal();

  void testXml_data();
  void testXml();

  void testNormal_data();
  void testNormal();
};

#endif // INDENTTEST_H
