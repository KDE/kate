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
#include <QtCore/QStringList>
#include <QtCore/QPair>

class TestScriptEnv;
class KateDocument;
class KateView;
class KMainWindow;

class IndentTest : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();
  void cleanupTestCase();

  void cstyle_data();
  void cstyle();

  void ruby_data();
  void ruby();

  void haskell_data();
  void haskell();

  void normal_data();
  void normal();

private:
  void getTestData(const QString& indenter);

  typedef QPair<const char*, const char*> Failure;
  typedef QList< Failure > ExpectedFailures;
  void runTest(const ExpectedFailures& failures);

  TestScriptEnv* m_env;
  KateDocument* m_document;
  KMainWindow* m_toplevel;
  bool m_outputCustomised;
  QStringList m_commands;
  KateView* m_view;
};

#endif // INDENTTEST_H
