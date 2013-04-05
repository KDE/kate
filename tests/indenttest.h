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

  void python_data();
  void python();

  void cstyle_data();
  void cstyle();

  void cppstyle_data();
  void cppstyle();

  void ruby_data();
  void ruby();

  void haskell_data();
  void haskell();

  void normal_data();
  void normal();
};

#endif // INDENTTEST_H
